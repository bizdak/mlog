#pragma once

#include "wincurses.h"
#include "logfile.h"
#include "logtailer.h"
#include "view.h"
#include "panel.h"
#include "messagecollector.h"

#define LM_STATUS_BAR 1
#define LM_TIMESTAMP 2
#define LM_ACTIVE 3
#define LM_THREAD_ID 4
#define LM_SYSTEM_MESSAGE 5
#define LM_HIGHLIGHT 6
#define LM_SEARCH_HIGHLIGHT 7
#define LM_STATUS_PAUSED 8
#define LM_STATUS_TAIL 9
#define LM_HIGHLIGHT_1 10
#define LM_HIGHLIGHT_2 11
#define LM_HIGHLIGHT_3 13
#define LM_HIGHLIGHT_4 14
#define LM_HIGHLIGHT_5 15
#define LM_CATEGORY_SCRIPT 16
#define LM_CATEGORY_SCRIPT_NAME 17
#define LM_RULESET_NAME 18
#define LM_RULE_NAME 19

struct Color
{
	int id;
	int foreground;
	int background;
	Color(int a, int b, int c) : id(a), foreground(b), background(c) {}
};


class StatusLine
{
	std::shared_ptr<Window> win_;
	chtype color_;

	struct Section
	{
		int width;
		chtype color;
		std::string text;
	};

	std::vector<Section> sections_;
	bool visible_ = true;

public:
	StatusLine(std::shared_ptr<Window> win, chtype color)
		: win_(win), color_(color)
	{

	}

	void SetVisible(bool visible) { visible_ = visible; }

	void CreateSection(int width)
	{
		Section sect;
		sect.width = width;
		sect.color = color_;
		sections_.push_back(sect);
	}

	void SetStatus(int section, chtype color, const char* text)
	{
		if (section > (int)sections_.size())
			throw std::runtime_error("invalid section id");
		auto& sect = sections_[section];
		sect.color = color;
		sect.text = text;
	}

	void Render()
	{
		if (!visible_)
			return;
		int maxy, maxx;
		getmaxyx(*win_, maxy, maxx);
		int row = maxy - 1;
		win_->Move(row, 0);
		win_->AttrOn(COLOR_PAIR(color_));
		win_->PrintF(row, 0, maxx, ""); //win_->ClearToEol();
		win_->AttrOff(COLOR_PAIR(color_));

		int col = 0;
		for (const auto& sect : sections_) {
			win_->Move(row, col);
			win_->AttrOn(COLOR_PAIR(sect.color));
			int width = sect.width == 0 ? maxx : sect.width;
			for (int i = 0; i < width && i < (int)sect.text.size(); i++)
				win_->Addch(sect.text[i]);
			win_->AttrOff(COLOR_PAIR(sect.color));
			col += sect.width;

			// if rendering full width, override everything else
			if (sect.width == 0 && sect.text.size() > 0)
				break;
		}
	}
};

class MainUi
{
public:
	MainUi();
	~MainUi();
	void AddLog(std::shared_ptr<LogFile> logfile, const char* title);
	void Run();
	void SetStatus(int section, chtype attr, const char* fmt, ...);

	ViewPtr GetActiveView() const { return activeView_; }
	void SetActiveView(ViewPtr view) { activeView_ = view; }

private:
	std::shared_ptr<Window> win_;
	std::vector<std::shared_ptr<LogFile>> logfiles_;
	std::string status_;
	bool terminate_;
	LogTailer logTailer_;
	std::shared_ptr<StatusLine> statusLine_;

	// our views
	std::vector<ViewPtr> views_;
	ViewPtr activeView_;
	ViewPtr helpView_;	// keep a pointer to help view so we can pop this up quickly
	std::vector<ViewPtr> viewStack_;
	std::vector<ViewPtr> hotKeyViews_;

	int colorSchemeIndex_ = 0;
	typedef std::vector<Color> ColorScheme;
	std::vector<ColorScheme> colorSchemes_;

	void Init();
	void InitColorSchemes();
	void CreateConsolidatedView();
	void CreateMessageView(MessageCollectorPtr msgCollector);
	void CreateHelpView();
	void SetColorScheme(int idx);
	void Dispatch();
	bool Update();
	void Render();
	void RenderMenu();
	void RenderStatusLine();
	void DoSearch();
};
