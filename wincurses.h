#pragma once

#include <curses.h>
#include <varargs.h>
#include <cassert>
#include <memory>

class Window
{
	WINDOW* win_;

public:
	Window(WINDOW* win) : win_(win) {
		assert(win_ != nullptr);
	}
	~Window() {
		if (win_) {
			delwin(win_);
		}
	}

	operator WINDOW* () const { return win_; }

	std::shared_ptr<Window> MakeSubWindow(int nlines, int ncols, int begy, int begx)
	{
		return std::make_shared<Window>(subwin(win_, nlines, ncols, begy, begx));
	}

	int GetMaxX() const { return getmaxx(win_); }
	int GetMaxY() const { return getmaxy(win_); }
	void PrintF(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf) - 1, fmt, args);
		for (const char* p = buf; *p; p++)
			waddch(win_, *p);
		va_end(args);
	}
	void PrintF(int y, int x, int n, const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf) - 1, fmt, args);
		for (const char* p = buf; *p && n > 0; p++, n--, x++) {
			mvwaddch(win_, y, x, *p);
			//if (*p == '\t') {
			//	for (int i = 0; i < 3 && n > 0; i++, n--, x++)
			//		mvwaddch(win_, y, x, ' ');
			//}
		}
		while (n > 0) {
			mvwaddch(win_, y, x++, ' ');
			n--;
		}
		va_end(args);
	}
	void Move(int y, int x) { wmove(win_, y, x); }
	void ClearToEol() { wclrtoeol(win_); }

	void Touch() { touchwin(win_); }
	void Refresh() { wrefresh(win_); }

	void AttrOn(chtype attr)
	{
		wattron(win_, attr);
	}

	void AttrOff(chtype attr)
	{
		wattroff(win_, attr);
	}

	void Addch(chtype ch)
	{
		waddch(win_, ch);
	}

	void MvAddch(int y, int x, chtype ch)
	{
		mvwaddch(win_, y, x, ch);
	}
};

