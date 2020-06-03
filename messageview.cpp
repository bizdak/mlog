#include "messageview.h"
#include <algorithm>

MessageView::MessageView(MainUi* ui, std::shared_ptr<Window> win, MessageCollectorPtr collector)
	: ui_(ui), win_(win), collector_(collector)
{
	buffer_.reserve(5000);
}

MessageView::~MessageView()
{

}

void MessageView::Init()
{
	SetPosition();
	CalculateMessageViewSize();
}

void MessageView::EnsureVisible()
{
	// if row_ is no longer visible increment row_
	int visibleRows = msgEndY_ - msgBegY_ - 1;
	if (selectedIdx_ >= startRow_ + visibleRows) 
		startRow_ = selectedIdx_ - visibleRows + 1;
	else if (startRow_ > selectedIdx_)
		startRow_ = selectedIdx_;

	if (buffer_.size() > 0 && startRow_ >= buffer_.size())
		startRow_ = (int)buffer_.size() - 1;
	if (startRow_ < 0)
		startRow_ = 0;
}

void MessageView::CalculateMessageViewSize()
{
	int maxy = win_->GetMaxY();
	msgBegY_ = 2;
	msgEndY_ = (int)(maxy * .4);
}

bool MessageView::Update(int c)
{
	switch (c) {
	case KEY_DOWN:
		if (selectedIdx_ < buffer_.size() - 1) {
			selectedIdx_++;
			EnsureVisible();
			return true;
		}
		break;

	case KEY_UP:
		tail_ = false;
		if (selectedIdx_ > 0) {
			selectedIdx_--;
			EnsureVisible();
			return true;
		}
		break;

	case KEY_NPAGE: case ' ': {
		if (selectedIdx_ == buffer_.size() - 1)
			return false;
		int pageSize = msgEndY_ - msgBegY_ - 1;
		startRow_ = std::min(startRow_ + pageSize, (int)buffer_.size() - 2);	// always leave one line on screen
		selectedIdx_ = startRow_ = std::max(startRow_, 0);
		SetPosition();
		return true;
	}

	case KEY_PPAGE: {
		int pageSize = msgEndY_ - msgBegY_ - 1;
		selectedIdx_ = startRow_ = std::max(startRow_ - pageSize, 0);
		tail_ = false;
		SetPosition();
		return true;
	}

	case '\n': case '\r':
		tail_ = true;
		return true;
	}

	return false;
}

void MessageView::Render()
{
	RefreshBuffer();
	int maxy = win_->GetMaxY();

	if (tail_) {
		selectedIdx_ = buffer_.size() == 0 ? 0 : (int)buffer_.size() - 1;
		EnsureVisible();
	}

	RenderMessageView(0, msgEndY_);
	RenderDetailView(msgEndY_, maxy);

	SetPosition();
	win_->Touch();
	win_->Refresh();
}

void MessageView::RenderMessageView(int starty, int maxy)
{
	int y = starty;
	int maxx = win_->GetMaxX();
	// render header
	win_->AttrOn(COLOR_PAIR(LM_STATUS_BAR));
	win_->PrintF(y, 0, maxx, "");
	win_->PrintF(y, 2, 15, "Message Name");
	win_->PrintF(y, 20, 15, "Receiver Time");
	win_->PrintF(y, 40, 15, "Engine Time");
	win_->PrintF(y, 60, 15, "Sender Time");
	win_->PrintF(y, 80, 15, "SpamProfiler");
	win_->AttrOff(COLOR_PAIR(LM_STATUS_BAR));

	y++;
	while (y < msgBegY_) {
		win_->Move(y++, 0);
		win_->ClearToEol();
	}

	int start = startRow_;
	for (int i = y; i < maxy; i++, start++) {
		if (start < (int)buffer_.size() && i < maxy - 1) {
			const auto& msg = buffer_[start];

			bool highlight = msg->touchTime + 5 > time(nullptr);
			if (highlight)
				win_->AttrOn(A_BOLD);

			// display selection mark
			if (selectedIdx_ == start)
				win_->AttrOn(COLOR_PAIR(LM_ACTIVE));
			
			// render columns
			win_->PrintF(i, 1, maxx, "");

			win_->PrintF(i, 2, 15, "%s", msg->messageName.c_str());
			win_->PrintF(i, 20, 15, "%s", msg->rxTime.c_str());
			win_->PrintF(i, 40, 15, "%s", msg->engTimeLatest.c_str());
			win_->PrintF(i, 60, 15, "%s", msg->txTimeLatest.c_str());
			win_->PrintF(i, 82, 15, "%3d", msg->spamProfilerScore);
			win_->PrintF(i, 86, 4, "%c%c", msg->spamProfilerRescan ? 'R' : '.', msg->spamProfilerBulk ? 'B' : '.');
			win_->ClearToEol();

			if (selectedIdx_ == start)
				win_->AttrOff(COLOR_PAIR(LM_ACTIVE));

			if (highlight)
				win_->AttrOff(A_BOLD);
		}
		else {
			win_->Move(i, 0);
			win_->ClearToEol();
		}
	}
}

void MessageView::RenderDetailView(int starty, int maxy)
{
	int y = starty;
	int maxx = win_->GetMaxX();
	// render header
	win_->AttrOn(COLOR_PAIR(LM_STATUS_BAR));
	win_->PrintF(y, 0, maxx, "");
	win_->PrintF(y, 2, 20, "Message Details");
	win_->AttrOff(COLOR_PAIR(LM_STATUS_BAR));

	y++;

	win_->Move(y, 0);
	win_->ClearToEol();

	if (selectedIdx_ < buffer_.size()) {
		const auto& msg = buffer_[selectedIdx_];
		y = RenderDetailLogView(y, maxy, "Receiver", msg->rxLogs);
		y = RenderDetailLogView(y, maxy, "Engine", msg->engLogs);
		y = RenderDetailLogView(y, maxy, "Sender", msg->txLogs);
	}

	// clear out remaining lines
	while (y++ < maxy) {
		win_->Move(y, 0);
		win_->ClearToEol();
	}
}

int MessageView::RenderDetailLogView(int starty, int maxy, const char* title, const boost::circular_buffer<LogEntryPtr>& logs)
{
	int y = starty;
	if (y >= maxy)
		return y;
	int maxx = win_->GetMaxX();
	for (auto log : logs) {
		if (log->type != MessageType::normal)
			continue;
		
		if (++y > maxy)
			break;

		const int Margin = 2;
		const int FileNameWidth = 12;
		const int TimeWidth = 14;

		int width = maxx - Margin;
		int x = Margin;

		// clear it initally before we paint
		win_->Move(y, 0);
		win_->ClearToEol();

		win_->AttrOn(COLOR_PAIR(LM_THREAD_ID));
		win_->PrintF(y, x, FileNameWidth, title); x += FileNameWidth; width -= FileNameWidth;
		win_->AttrOff(COLOR_PAIR(LM_THREAD_ID));

		win_->AttrOn(COLOR_PAIR(LM_TIMESTAMP));
		win_->PrintF(y, x, TimeWidth, "%s", log->time.c_str()); x += TimeWidth; width -= TimeWidth;
		win_->AttrOff(COLOR_PAIR(LM_TIMESTAMP));

		win_->PrintF(y, x, width, "%s", log->body.c_str());
	}
	return y;
}


void MessageView::Resize()
{
	wresize(*win_, LINES - 4, COLS);
	CalculateMessageViewSize();
}

void MessageView::RefreshBuffer()
{
	if (tail_) {
		buffer_.clear();
		collector_->FillBuffer(buffer_);
		selectedIdx_ = buffer_.size() == 0 ? 0 : (int)buffer_.size() - 1;
		if (startRow_ > buffer_.size())
			startRow_ = selectedIdx_;
	}
}

void MessageView::SetPosition()
{
	ui_->SetStatus(0, 0, "");
	ui_->SetStatus(1, 0, "");
	ui_->SetStatus(2, tail_ ? LM_STATUS_TAIL : LM_STATUS_PAUSED, " [%s] %s", tail_ ? "Tail" : "Paused", !tail_ ? " press enter to continue " : "");
}
