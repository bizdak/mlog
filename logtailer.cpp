#include "logtailer.h"
#include <thread>

LogTailer::LogTailer()
	: exiting_(false)
{

}

void LogTailer::AddLogFile(std::shared_ptr<LogFile> logfile)
{
	logfiles_.push_back(logfile);
}

void LogTailer::Run()
{
	tailThread_ = std::thread(&LogTailer::DoTail, this);
}

void LogTailer::DoTail()
{
	using namespace std::chrono_literals;
	while (!exiting_) {
		for (auto file : logfiles_) {
			file->Tail();
		}
		std::this_thread::sleep_for(100ms);
	}
}
