
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <system_error>
#include <curses.h>
#include <varargs.h>
#include <cassert>
#include "LogFile.h"
#include "wincurses.h"
#include "mainui.h"
#include "MessageCollector.h"
#include <Windows.h>



int main()
{
	try {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);

		MainUi ui;
		ui.Run();
	}
	catch (std::exception& e) {
		OutputDebugString(e.what());
		printf("Error: %s\n", e.what());
	}
	return 0;
}
