#pragma once

#include "logfile.h"
#include <vector>
#include <thread>

class LogTailer
{
	std::vector<std::shared_ptr<LogFile>> logfiles_;

public:
	LogTailer();
	void AddLogFile(std::shared_ptr<LogFile> logfile);
	void Run();
	void Shutdown() { exiting_ = true; tailThread_.join(); }

private:
	bool exiting_ = false;
	std::thread tailThread_;

	void DoTail();
};

