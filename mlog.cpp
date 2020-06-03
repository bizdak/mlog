
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <system_error>
#include <curses.h>
#include <varargs.h>
#include <cassert>
#include "logfile.h"
#include "mainui.h"
#include "messagecollector.h"
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
