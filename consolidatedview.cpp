#include "consolidatedview.h"
#include <stdio.h>
#include <stdlib.h>
#include <cassert>

ConsolidatedLogView::ConsolidatedLogView(MainUi* ui, std::shared_ptr<Window> win, std::vector<std::shared_ptr<LogFile>> files)
	: ui_(ui), win_(win), files_(std::move(files)), buffer_(200)
{
}

void ConsolidatedLogView::Init()
{
	SetPosition();
}

void ConsolidatedLogView::SetSearchPattern(const std::string& searchPattern)
{
	View::SetSearchPattern(searchPattern);
	try {
		if (searchPattern.empty())
			highlighPattern_ = boost::regex();
		else
			highlighPattern_ = boost::regex(searchPattern);
	}
	catch (boost::exception&) {

	}
}

bool ConsolidatedLogView::Update(int c)
{
	const int pageSize = LINES - 6;
	switch (c) {
	case KEY_DOWN: case 'j':
		row_ = std::min(row_ + 1, (int)buffer_.size() - 2);	// always leave one line on screen
		row_ = std::max(row_, 0);
		SetPosition();
		return true;

	case KEY_UP: case 'k':
		row_ = std::max(0, row_ - 1);
		SetPosition();
		return true;

	case KEY_NPAGE: case ' ':
		row_ = std::min(row_ + pageSize, (int)buffer_.size() - 2);	// always leave one line on screen
		row_ = std::max(row_, 0);
		SetPosition();
		return true;

	case KEY_PPAGE:
		row_ = std::max(row_ - pageSize, 0);
		SetPosition();
		return true;

	case KEY_LEFT: case 'h':
		col_ = std::max(col_ - 1, 0);
		return true;

	case KEY_RIGHT: case 'l':
		col_++;
		return true;

	case KEY_HOME:
		col_ = 0;
		return true;

	case KEY_ENTER: case 't': case '\r': case '\n':
		SetPosition();
		return true;

	case 'g':
		col_ = 0;
		SetPosition();
		return true;

	case 'G':
		row_ = std::max(0, (int)buffer_.size() - 2);
		return true;

	}

	return false;
}

void ConsolidatedLogView::Resize()
{
	wresize(*win_, LINES - 4, COLS);
}

void ConsolidatedLogView::RefreshBuffer()
{
	// grab n entries per log file
	int N = 10;
	std::vector<std::shared_ptr<LogEntryEx>> tmp;
	for (int i = 0; i < (int)files_.size(); i++) {
		auto file = files_[i];
		LockBuffer lock(file);
		int j = std::max(file->NumLines() - N - 1, 0);
		for (; j < file->NumLines(); j++) {
			auto entry = std::make_shared<LogEntryEx>();
			*entry = *file->GetEntry(j);
			entry->filename = file->Filename().filename().string();
			tmp.push_back(entry);
		}

		if (file->NumLines() > 0) {
			// add an empty entry per log file to separate them
			auto entry = std::make_shared<LogEntryEx>();
			entry->type = MessageType::separator;
			tmp.push_back(entry);
		}
	}

	//std::sort(tmp.begin(), tmp.end(), [](std::shared_ptr<LogEntryEx> lhs, std::shared_ptr<LogEntryEx> rhs) -> bool {
	//	return rhs->timestamp > lhs->timestamp;
	//	});

	buffer_.clear();
	for (auto t : tmp) {
		buffer_.push_back(t);
	}
}

void ConsolidatedLogView::Render()
{
	RefreshBuffer();
	int maxy, maxx;
	getmaxyx(*win_, maxy, maxx);

	int start = row_;

	for (int i = 0; i < maxy; i++, start++) {
		if (start < (int)buffer_.size()) {
			auto entry = buffer_[start];

			win_->Move(i, 0);
			win_->ClearToEol();
			win_->AttrOn(COLOR_PAIR(LM_THREAD_ID));
			win_->PrintF(i, 1, 25, "%s", entry->filename.c_str());
			win_->AttrOff(COLOR_PAIR(LM_THREAD_ID));
			win_->MvAddch(i, 26, ACS_VLINE);

			if (entry->type == MessageType::separator) {
				win_->MvAddch(i, 26, ACS_LTEE);
				for (int x = 27; x < maxx; x++)
					win_->MvAddch(i, x, ACS_HLINE);
			}
			else {
				bool highlight = entry->timestamp + 3 > time(nullptr);
				if (highlight)
					win_->AttrOn(A_BOLD);
				win_->PrintF(i, 28, 12, "%s ", entry->time.c_str());
				win_->PrintF(i, 41, (int)entry->body.size(), "%s", entry->body.c_str());
				if (highlight)
					win_->AttrOff(A_BOLD);
			}
		}
		else {
			win_->Move(i, 0);
			win_->ClearToEol();
		}
	}

	SetPosition();
	win_->Touch();
	win_->Refresh();
}

void ConsolidatedLogView::RenderBody(int y, int x, int width, const std::string& body, int offset)
{
	win_->Move(y, x);
	boost::match_results<std::string::const_iterator> match;
	const auto& pattern = highlighPattern_;
	if (pattern != boost::regex() && boost::regex_search(body, match, pattern, boost::match_default)) {

		auto p = body.begin() + offset;
		for (auto m : match) {
			while (p < m.begin() && width-- > 0)
				win_->MvAddch(y, x++, *p++);

			win_->AttrOn(COLOR_PAIR(LM_SEARCH_HIGHLIGHT));
			while (p < m.end() && width-- > 0)
				win_->MvAddch(y, x++, *p++);
			win_->AttrOff(COLOR_PAIR(LM_SEARCH_HIGHLIGHT));

			if (width <= 0)
				break;
		}
		while (p < body.end() && width-- > 0)
			win_->MvAddch(y, x++, *p++);
	}
	else
		win_->PrintF(y, x, width, "%s", body.c_str() + offset);
	win_->ClearToEol();
}

void ConsolidatedLogView::SetPosition()
{
	if (buffer_.size() == 0)
		return;

	ui_->SetStatus(0, 0, "");
	ui_->SetStatus(1, 0, "");
	ui_->SetStatus(2, 0, "");
}
