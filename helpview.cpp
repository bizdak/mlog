#include "helpview.h"
#include <algorithm>

HelpView::HelpView(MainUi* ui, std::shared_ptr<Window> win)
	: ui_(ui), win_(win)
{
	buffer_ = {
		"Lloyd Macrohon's MailMarshal Logs Viewer",
		"",
		"Keys",
		"  b       - open baretail",
		"  c       - cycle colours",
		"  d       - toggle timestamp or duration (certain views only)",
		"  e       - open explorer",
		"  f       - filter certain entries (available in CfgService view only)",
		"  h       - toggle highlight",
		"  o       - open log file in editor",
		"  q       - quit viewer",
		"  /       - search for string",
		"  esc     - close help"
	};
}

void HelpView::Init()
{
}

bool HelpView::Update(int c)
{
	const int pageSize = LINES - 6;
	switch (c) {
	case KEY_DOWN:
		row_ = std::min(row_ + 1, (int)buffer_.size() - 1);
		row_ = std::max(row_, 0);
		return true;

	case KEY_UP:
		row_ = std::max(0, row_ - 1);
		return true;

	case KEY_NPAGE: case ' ':
		row_ = std::min(row_ + pageSize, (int)buffer_.size() - 2);	// always leave one line on screen
		row_ = std::max(row_, 0);
		return true;

	case KEY_PPAGE:
		row_ = std::max(row_ - pageSize, 0);
		return true;

	case KEY_LEFT:
		col_ = std::max(col_ - 1, 0);
		return true;

	case KEY_RIGHT:
		col_++;
		return true;

	case KEY_HOME:
		col_ = 0;
		return true;

	case 'g':
		col_ = 0;
		return true;

	case 'G':
		row_ = std::max(0, (int)buffer_.size() - 1);
		return true;

	case 'q':	// override, do not quit app
		return true;
	}

	return false;
}

void HelpView::Resize()
{
	wresize(*win_, LINES - 4, COLS);
}

void HelpView::Render()
{
	int maxy = win_->GetMaxY();
	int maxx = win_->GetMaxX();

	int start = row_;
	for (int i = 0; i < maxy; i++, start++) {
		if (start < (int)buffer_.size()) {
			const auto& line = buffer_[start];
			win_->PrintF(i, 2, (int)line.size() + 2, "%s", line.c_str());
			win_->ClearToEol();
		}
		else {
			win_->Move(i, 0);
			win_->ClearToEol();
		}
	}
	win_->Touch();
	win_->Refresh();
}
