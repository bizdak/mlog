#include "mainui.h"
#include "logfile.h"
#include "logview.h"
#include "cfgsvclogview.h"
#include "consolidatedview.h"
#include "helpview.h"
#include "inputtext.h"
#include "utils.h"
#include "messagecollector.h"

MainUi::MainUi()
{
}

void MainUi::SetColorScheme(int idx)
{
	colorSchemeIndex_ = idx % colorSchemes_.size();
	const auto& scheme = colorSchemes_[colorSchemeIndex_];
	for (auto color : scheme) {
		init_pair(color.id, color.foreground, color.background);
	}

	statusLine_->CreateSection(0);
	statusLine_->CreateSection(85);
	statusLine_->CreateSection(40);
}

MainUi::~MainUi()
{
	views_.clear();
}

void MainUi::AddLog(std::shared_ptr<LogFile> logfile, const char* title)
{
	if (logfiles_.size() > 9)
		throw std::runtime_error("Exceeded number of supported log files");
	logfiles_.push_back(logfile);
	logTailer_.AddLogFile(logfile);
	// create the log view
	int maxy, maxx;
	getmaxyx(curscr, maxy, maxx);

	auto panel = new_panel(curscr);
	View* view = nullptr;
	
	if (logfile->IsConfigService()) {
		view = new CfgSvcLogView(this,
			// sub-windows crash when resizing and larger than current size
			// this could also easiliy mean that I don't understand ncurses enough
			//win_->MakeSubWindow(120, 100, 0, 0),
			std::make_shared<Window>(newwin(maxy - 4, maxx, 2, 0)),
			logfile);
	}
	else {
		view = new LogView(this,
			// sub-windows crash when resizing and larger than current size
			// this could also easiliy mean that I don't understand ncurses enough
			//win_->MakeSubWindow(120, 100, 0, 0),
			std::make_shared<Window>(newwin(maxy - 4, maxx, 2, 0)),
			logfile);
	}
	ViewPtr logView(view);
	logView->SetTitle(title);
	hotKeyViews_.push_back(logView);
	views_.push_back(logView);
}

void MainUi::CreateConsolidatedView()
{
	int maxy, maxx;
	getmaxyx(curscr, maxy, maxx);
	auto view = new ConsolidatedLogView(this,
		// sub-windows crash when resizing and larger than current size
		// this could also easiliy mean that I don't understand ncurses enough  
		//win_->MakeSubWindow(120, 100, 0, 0),
		std::make_shared<Window>(newwin(maxy - 4, maxx, 2, 0)),
		logfiles_);
	ViewPtr logView(view);
	logView->SetTitle("Consolidated");
	hotKeyViews_.push_back(logView);
	views_.push_back(logView);
}

void MainUi::CreateHelpView()
{
	int maxy, maxx;
	getmaxyx(curscr, maxy, maxx);
	auto view = new HelpView(this,
		// sub-windows crash when resizing and larger than current size
		// this could also easiliy mean that I don't understand ncurses enough  
		//win_->MakeSubWindow(120, 100, 0, 0),
		std::make_shared<Window>(newwin(maxy - 4, maxx, 2, 0)));
	helpView_ = ViewPtr(view);
	views_.push_back(helpView_);
}

void MainUi::Init()
{
	initscr();
	win_ = std::make_shared<Window>(curscr);
	statusLine_ = std::make_shared<StatusLine>(win_, LM_STATUS_BAR);
	keypad(stdscr, TRUE);	// enable f1 keys, etc...
	nonl();		// do not translate \n to \r\\n
	cbreak();	// raw() but generate signals for ctrl+c/ctrl+z
	noecho();
	nodelay(stdscr, TRUE);
	curs_set(0);	// no cursor

	// init colors
	if (!has_colors()) {
		endwin();
		printf("Terminal has no color support\n");
		exit(1);
	}
	start_color();

	InitColorSchemes();
	SetColorScheme(0);

	namespace fs = boost::filesystem;
	fs::path mmdir = GetMailMarshalInstallDirectory();
	fs::path logging = mmdir / fs::path("logging");
	fs::path cfgSvcLogging = mmdir / fs::path("Config Service") / fs::path("logging");

	AddLog(std::make_shared<LogFile>(logging, "MMArrayManager"), "AM");
	AddLog(std::make_shared<LogFile>(logging, "MMController"), "Controller");

	auto rxLog = std::make_shared<LogFile>(logging, "MMReceiver");
	auto engLog = std::make_shared<LogFile>(logging, "MMEngine");
	auto txLog = std::make_shared<LogFile>(logging, "MMSender");

	AddLog(rxLog, "Receiver");
	AddLog(engLog, "Engine");
	AddLog(txLog, "Sender");
	AddLog(std::make_shared<LogFile>(logging, "MMPop3"), "Pop3");
	AddLog(std::make_shared<LogFile>(cfgSvcLogging, "segcfgapi", true), "Cfg Service");

	auto msgCollector = std::make_shared<MessageCollector>();
	msgCollector->AddLogFile(MessageCollector::LogType::receiver, rxLog);
	msgCollector->AddLogFile(MessageCollector::LogType::engine, engLog);
	msgCollector->AddLogFile(MessageCollector::LogType::sender, txLog);

	CreateConsolidatedView();
	CreateHelpView();

	if (!activeView_)
		activeView_ = hotKeyViews_[0];
}

void MainUi::InitColorSchemes()
{
	colorSchemes_.push_back(ColorScheme{
		Color(LM_STATUS_BAR, COLOR_BLACK, COLOR_WHITE),
		Color(LM_STATUS_TAIL, COLOR_WHITE, COLOR_GREEN),
		Color(LM_STATUS_PAUSED, COLOR_WHITE, COLOR_RED),
		Color(LM_TIMESTAMP, COLOR_YELLOW, COLOR_BLACK),
		Color(LM_ACTIVE, COLOR_YELLOW, COLOR_BLUE),
		Color(LM_THREAD_ID, COLOR_CYAN, COLOR_BLACK),
		Color(LM_SYSTEM_MESSAGE, COLOR_GREEN, COLOR_BLACK),
		Color(LM_HIGHLIGHT, COLOR_GREEN, COLOR_BLACK),
		Color(LM_SEARCH_HIGHLIGHT, COLOR_YELLOW, COLOR_BLUE),
		Color(LM_HIGHLIGHT_1, COLOR_GREEN, COLOR_BLACK),
		Color(LM_HIGHLIGHT_2, COLOR_CYAN, COLOR_BLACK),
		Color(LM_HIGHLIGHT_3, COLOR_RED, COLOR_BLACK),
		Color(LM_HIGHLIGHT_4, COLOR_BLUE, COLOR_BLACK),
		Color(LM_HIGHLIGHT_5, COLOR_YELLOW, COLOR_BLACK),
		});

	colorSchemes_.push_back(ColorScheme{
		Color(LM_STATUS_BAR, COLOR_BLACK, COLOR_WHITE),
		Color(LM_TIMESTAMP, COLOR_GREEN, COLOR_BLACK),
		Color(LM_ACTIVE, COLOR_YELLOW, COLOR_BLUE),
		Color(LM_THREAD_ID, COLOR_CYAN, COLOR_BLACK),
		Color(LM_SYSTEM_MESSAGE, COLOR_GREEN, COLOR_BLACK),
		Color(LM_HIGHLIGHT, COLOR_BLACK, COLOR_WHITE),
		Color(LM_SEARCH_HIGHLIGHT, COLOR_YELLOW, COLOR_BLUE),
		Color(LM_HIGHLIGHT_1, COLOR_GREEN, COLOR_BLACK),
		Color(LM_HIGHLIGHT_2, COLOR_CYAN, COLOR_BLACK),
		Color(LM_HIGHLIGHT_3, COLOR_RED, COLOR_BLACK),
		Color(LM_HIGHLIGHT_4, COLOR_BLUE, COLOR_BLACK),
		Color(LM_HIGHLIGHT_5, COLOR_YELLOW, COLOR_BLACK),
		});
}

void MainUi::Run()
{
	Init();

	// force a terminal reset
	resize_term(0, 0);
	curs_set(0);	// no cursor - this seems to get reset on resize
	for (auto view : views_)
		view->Resize();
	win_->Refresh();

	logTailer_.Run();
	Dispatch();
	logTailer_.Shutdown();
	endwin();
}

void MainUi::Dispatch()
{
	using namespace std::chrono_literals;
	terminate_ = false;
	clock_t lastRender = 0;
	while (!terminate_) {
		std::this_thread::sleep_for(20ms);
		if (!Update() && lastRender + CLOCKS_PER_SEC > clock())
			continue;
		Render();
		win_->Refresh();
		lastRender = clock();
	}
}

void MainUi::DoSearch()
{
	InputText inputText(win_, win_->GetMaxY() - 1, 0, " Regex Search: ");

	auto activeView = GetActiveView();
	inputText.SetText(activeView->GetCurrentSearchPattern());

	statusLine_->SetVisible(false);
	int res;
	while ((res = inputText.GetInput()) == 0) {
		Render();
		inputText.Render();
	}
	statusLine_->SetVisible(true);
	if (res == 1)
		activeView->SetSearchPattern(inputText.GetText());
}

bool MainUi::Update()
{
	int c = getch();
	// give view first rights to keyboard
	if (activeView_->Update(c))
		return true;

	switch (c) {
	case 'x': case 'X': case 'q': case 'Q':
		terminate_ = true;
		return false;
	case KEY_RESIZE: case 'r': case 'R':
		resize_term(0, 0);
		curs_set(0);	// no cursor - this seems to get reset on resize
		for (auto view : views_)
			view->Resize();
		return true;

	case 'c':
		SetColorScheme(colorSchemeIndex_ + 1);
		return true;

	case '?':
		if (activeView_ != helpView_) {
			viewStack_.push_back(activeView_);
			activeView_ = helpView_;
			return true;
		}
		break;

	case 27: // esc
		if (viewStack_.size() > 0) {
			activeView_ = viewStack_[viewStack_.size() - 1];
			viewStack_.pop_back();
			return true;
		}
		break;

	case '/':
		DoSearch();
		return true;
	}

	if (c >= '1' && c <= '9') {
		int idx = c - '1';
		if (idx < (int)hotKeyViews_.size()) {
			auto view = hotKeyViews_[idx];
			activeView_ = view;
			return true;
		}
	}

	return false;
}

void MainUi::Render()
{
	activeView_->Render();
	RenderMenu();
	statusLine_->Render();
}

void MainUi::RenderMenu()
{
	if (activeView_ == helpView_) {
		win_->Move(0, 0);
		win_->ClearToEol();
		return;
	}

	int x = 2;
	int idx = 0;
	for (auto view : hotKeyViews_) {
		if (view == activeView_) {
			win_->AttrOn(COLOR_PAIR(LM_ACTIVE));
			win_->AttrOn(A_BOLD);
		}
		idx++;
		win_->PrintF(0, x, (int)view->Title().size() + 5, "%d. %s", idx, view->Title().c_str());
		x += (int)view->Title().size() + 4;
		if (view == activeView_) {
			win_->AttrOff(A_BOLD);
			win_->AttrOff(COLOR_PAIR(LM_ACTIVE));
		}
		win_->PrintF(0, x, 3, " | ");
		x += 3;
	}
}

void MainUi::SetStatus(int section, chtype color, const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	status_ = buf;
	va_end(args);
	statusLine_->SetStatus(section, color, buf);
}
