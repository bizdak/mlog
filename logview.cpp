#include "logview.h"
#include <stdio.h>
#include <stdlib.h>
#include <cassert>

LogView::LogView(MainUi* ui, std::shared_ptr<Window> win, std::shared_ptr<LogFile> file)
	: ui_(ui), win_(win), file_(file), tail_(true)
{
	highlights_.push_back(LogHighlight(LM_HIGHLIGHT_1, "[A-E][\\da-fA-F]{12}\\.[\\da-fA-F]{12}\\.[\\da-fA-F]{4}\\.mml", true));
	highlights_.push_back(LogHighlight(LM_HIGHLIGHT_2, "^\\w+:", true));
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT, "^------\\sCategory\\s<.*>\\sevaluation\\sresult\\s------$", true));
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT, "^------\\sEnd\\sof\\sCategory\\s<.*>\\sevaluation\\s------", true));

	// category script highlight
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT_NAME, "^------\\sCategory\\s<(.*)>\\sevaluation\\sresult\\s------$", true, 1));
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT_NAME, "^------\\sEnd\\sof\\sCategory\\s<(.*)>\\sevaluation\\s------", true, 1));
	// body higlight left border category script
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT_NAME, "^(\\s{2})\\s{2}-\\s(.*)\\:\\s\\(\\d+\\)\\s$", true, 1));
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT_NAME, "^(\\s{2})(Score:)\\s\\(\\d+\\)\\sRequired:.*", true, 1));
	highlights_.push_back(LogHighlight(LM_CATEGORY_SCRIPT_NAME, "^(\\s{2})(.*:)\\sVersion", true, 1));
	// body highlight for category script
	highlights_.push_back(LogHighlight(LM_HIGHLIGHT_1, "(^\\s{2})\\s{2}-\\s(.*)\\:\\s\\(\\d+\\)\\s$", true, 2));
	highlights_.push_back(LogHighlight(LM_HIGHLIGHT_1, "^(\\s{2})(Score:)\\s\\(\\d+\\)\\sRequired:.*", true, 2));
	highlights_.push_back(LogHighlight(LM_HIGHLIGHT_1, "^(\\s{2})(.*:)\\sVersion", true, 2));

	// highlight ruleset matches
	// match rule
	highlights_.push_back(LogHighlight(LM_HIGHLIGHT_1, "\\s([^0]\\suser)\\(s\\)\\smatch\\srule\\s-\\sContent:(.*?):(.*)", true, 1));
	// ruleset
	highlights_.push_back(LogHighlight(LM_RULESET_NAME, "\\s([^0]\\suser)\\(s\\)\\smatch\\srule\\s-\\sContent:(.*?):(.*)", true, 2));
	// rulename
	highlights_.push_back(LogHighlight(LM_RULE_NAME, "\\s([^0]\\suser)\\(s\\)\\smatch\\srule\\s-\\sContent:(.*?):(.*)", true, 3));
}

void LogView::Init()
{
	SetPosition();
}

void LogView::SetSearchPattern(const std::string& searchPattern)
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

bool LogView::Update(int c)
{
	const int pageSize = LINES - 6;
	switch (c) {
	case KEY_DOWN:
		row_ = std::min(row_ + 1, file_->NumLines() - 2);	// always leave one line on screen
		row_ = std::max(row_, 0);
		SetPosition();
		return true;

	case KEY_UP:
		row_ = std::max(0, row_ - 1);
		tail_ = false;
		file_->Pause(!tail_);
		SetPosition();
		return true;

	case KEY_NPAGE: case ' ':
		row_ = std::min(row_ + pageSize, file_->NumLines() - 2);	// always leave one line on screen
		row_ = std::max(row_, 0);
		SetPosition();
		return true;

	case KEY_PPAGE:
		row_ = std::max(row_ - pageSize, 0);
		tail_ = false;
		file_->Pause(!tail_);
		SetPosition();
		return true;

	case 'h':
		highlight_ = !highlight_;
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

	case KEY_ENTER: case '\r': case '\n': 
		tail_ = true;
		file_->Pause(!tail_);
		SetPosition();
		return true;

	case 'g':
		col_ = 0;
		SetPosition();
		return true;

	case 'G':
		row_ = std::max(0, file_->NumLines() - 2);
		return true;

	case 'o': case 'O':
		OpenLogFile(file_->Filename());
		break;
	
	case 'd': case 'D':
		displayDuration_ = !displayDuration_;
		break;
	}

	return false;
}

void LogView::Resize()
{
	wresize(*win_, LINES - 4, COLS);
}

void LogView::Render()
{
	LockBuffer lock(file_);
	int maxy, maxx;
	getmaxyx(*win_, maxy, maxx);
	int maxEntries = std::min(maxy, file_->NumLines());

	int tsWidth = 20;
	int margin = 2;
	int bodyWidth = maxx - tsWidth - margin;

	if (tail_) 
		row_ = file_->NumLines() < maxy ? 0 : file_->NumLines() - maxy;
	int start = row_;

	std::string lastTimestamp;
	int tid = 0;
	int tidCtr = 0;
	int lastNoneEmptyLine = 0;
	int lastMarker = 0;

	for (int i = 0; i < maxy; i++, start++) {
		if (start < file_->NumLines()) {
			auto entry = file_->GetEntry(start);

			if (entry->type == MessageType::system) {
				win_->AttrOn(COLOR_PAIR(LM_SYSTEM_MESSAGE));
				win_->MvAddch(i, 1, ACS_DIAMOND);
				win_->PrintF(i, margin, (int)entry->body.size() + 1, " %s", entry->body.c_str());
				win_->AttrOff(COLOR_PAIR(LM_SYSTEM_MESSAGE));
				win_->ClearToEol();
				continue;
			}
			else if (entry->type == MessageType::normal) {
				win_->AttrOn(COLOR_PAIR(LM_THREAD_ID));
				win_->PrintF(i, margin, 6, "%05d ", entry->tid);
				win_->AttrOff(COLOR_PAIR(LM_THREAD_ID));
				auto ts = entry->time.substr(0, 8);
				bool resetBold = false;
				if (!displayDuration_ && !lastTimestamp.empty() && ts != lastTimestamp) {
					// highlight the start of a new second, but don't do it for the 1st line, otherwise, that's always bold
					win_->AttrOn(A_BOLD);
					resetBold = true;
				}
				win_->AttrOn(COLOR_PAIR(LM_TIMESTAMP));
				if (displayDuration_) {
					win_->Move(i, margin + 6);

					if (!resetBold && entry->duration.hours() > 0) win_->AttrOn(A_BOLD), resetBold = true;
					win_->PrintF("%02d:", entry->duration.hours());

					if (!resetBold && entry->duration.minutes() > 0) win_->AttrOn(A_BOLD), resetBold = true;
					win_->PrintF("%02d:", entry->duration.minutes());

					if (!resetBold && entry->duration.seconds() > 0) win_->AttrOn(A_BOLD), resetBold = true;
					win_->PrintF("%02d.", entry->duration.seconds());
					
					if (!resetBold && entry->duration.fractional_seconds() / 1000 > 0) win_->AttrOn(A_BOLD), resetBold = true;
					win_->PrintF("%03d", entry->duration.fractional_seconds() / 1000);
				}
				else
					win_->PrintF(i, margin + 6, tsWidth - 6, "%s", entry->time.c_str());
				win_->AttrOff(COLOR_PAIR(LM_TIMESTAMP));
				lastTimestamp = ts;
				if (resetBold)
					win_->AttrOff(A_BOLD);
			}
			else {
				win_->PrintF(i, margin, tsWidth, "");	// clear out the timestamp section
			}

			if (col_ > 0)
				win_->MvAddch(i, margin + tsWidth - 1, ACS_VLINE);

			if (col_ < (int)entry->body.size())
				RenderBody(i, tsWidth + margin, bodyWidth, entry->body.c_str(), col_);
			else
				win_->PrintF(i, tsWidth + margin, bodyWidth, "");
			lastNoneEmptyLine = i;

			// draw line markers
			// thread the tid's, we always draw the current line 
			if (tid == entry->tid && tid != 0) {
				if (tidCtr++ == 1) {
					// we have the same tid as the previous line, and the previous line was the first one
					win_->MvAddch(i - 1, 1, lastMarker = ACS_ULCORNER);
				}
				win_->MvAddch(i , 1, lastMarker = ACS_VLINE);
			}
			else if (tid != entry->tid && entry->tid != 0) {
				// we have a new tid and the previous one wasn't unknown
				if (tid != 0) {
					if (tidCtr == 1)
						win_->MvAddch(i - 1, 1, lastMarker = ACS_DIAMOND);
					else if (tidCtr > 1)
						win_->MvAddch(i - 1, 1, lastMarker = ACS_LLCORNER);
				}
				// start accumulating again
				tidCtr = 1;
				tid = entry->tid;
				win_->MvAddch(i, 1, lastMarker = ACS_ULCORNER);
			}
			else if (tid != 0 && entry->tid == 0) {
				// this line doesn't have tid, consider it as part of the previous tid
				tidCtr++;
				win_->MvAddch(i, 1, lastMarker = ACS_VLINE);
			}
			else
				win_->MvAddch(i, 1, ' ');
			// end of line markers
		}
		else {
			win_->Move(i, 0);
			win_->ClearToEol();
		}
	}

	// update the last line to a more appropriate thread marker
	if (lastMarker == ACS_VLINE)
		win_->MvAddch(lastNoneEmptyLine, 1, tidCtr <= 1 ? ACS_DIAMOND : ACS_LLCORNER);
	
	SetPosition();
	win_->Touch();
	win_->Refresh();
}

void LogView::RenderBody(int y, int x, int width, const std::string& body, int offset)
{
	// display body first, then colorise each match
	win_->PrintF(y, x, width, body.c_str() + offset);
	win_->ClearToEol();

	if (highlight_) {
		for (auto highlight : highlights_) {
			if (highlight.bold)
				win_->AttrOn(A_BOLD);
			ColorizeMatch(y, x, width, body, offset, highlight.pattern, highlight.color, highlight.group);
			if (highlight.bold)
				win_->AttrOff(A_BOLD);
		}

		win_->AttrOn(A_BOLD);
		ColorizeMatch(y, x, width, body, offset, highlighPattern_, LM_SEARCH_HIGHLIGHT, 0);
		win_->AttrOff(A_BOLD);
	}
}

void LogView::ColorizeMatch(int y, int x, int width, const std::string & body, int offset, const boost::regex& pattern, int color, int group)
{
	boost::match_results<std::string::const_iterator> match;
	if (pattern != boost::regex() && boost::regex_search(body, match, pattern, boost::match_default)) {

		auto p = body.begin() + offset;
		for (int i = 0; i < match.size(); i++) {
			if (group != 0 && group != i)
				continue;

			auto m = match[i];
			win_->Move(y, x);
			while (p < m.begin() && width-- > 0)
				x++, p++;

			win_->AttrOn(COLOR_PAIR(color));
			while (p < m.end() && width-- > 0)
				win_->MvAddch(y, x++, *p++);
			win_->AttrOff(COLOR_PAIR(color));

			if (width <= 0)
				break;
		}
	}
}

void LogView::SetPosition()
{
	if (file_->NumLines() == 0)
		return;


	ui_->SetStatus(0, 0, "");
	ui_->SetStatus(1, LM_STATUS_BAR, "  %-s", file_->Filename().string().c_str());
	ui_->SetStatus(2, tail_ ? LM_STATUS_TAIL : LM_STATUS_PAUSED, " [%s] %s", tail_ ? "Tail" : "Paused", !tail_ ? " press enter to continue " : "");
}
