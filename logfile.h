#pragma once
#include <boost/regex.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include <boost/signals2/signal.hpp>
#include <filesystem>
#include <shared_mutex>
#include "utils.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace fs = boost::filesystem;
namespace ts = boost::posix_time;

enum class MessageType { normal, continuation, cfgSvc, system, separator };

struct LogEntry
{
	MessageType type = MessageType::normal;
	time_t timestamp = 0;
	std::string time;
	std::string body;
	int tid = 0;
	ts::time_duration duration;

	// for config service
	std::string severity;
	std::string id;
};

typedef std::shared_ptr<LogEntry> LogEntryPtr;

class LogFile;
typedef std::shared_ptr<LogFile> LogFilePtr;

class LockBuffer
{
	Spinlock lock;
	//std::unique_lock<std::shared_mutex> lock;
public:
	LockBuffer(LogFile& logfile);
	LockBuffer(LogFilePtr logfile);
};



class LogFile : public std::enable_shared_from_this<LogFile>
{
	FILE* fp_ = nullptr;
	boost::regex pattern_;			// pattern for parsing log entries
	bool cfgSvc_;
	fs::path logDir_;
	std::string namePrefix_;
	fs::path logFile_;		// current log file opened

	// we only buffer so many lines to display
	boost::circular_buffer<LogEntryPtr> buffer_;
	bool exiting_ = false;
	bool paused_ = false;

	friend class LockBuffer;
	//mutable std::shared_mutex lock_;
	mutable std::atomic_flag lock_;

	time_t lastLogCheck_ = 0;	// last time we checked for a new log

	boost::signals2::signal<void(LogFilePtr, LogEntryPtr)> subscribers_;

	// last timestamp or duration read
	ts::time_duration lastDuration_;

public:
	LogFile(fs::path logDir, std::string namePrefix, bool cfgSvc=false, int bufSize=10000);
	~LogFile();

	fs::path Filename() const { return logFile_; }
	void Close();
	void Tail();
	void Pause(bool pause) { paused_ = pause; }
	int NumLines() const { return (int)buffer_.size(); }
	std::shared_ptr<LogEntry> GetEntry(int idx) const
	{
		Spinlock spinlock(lock_);
		//std::unique_lock lock(lock_);
		return buffer_[idx];
	}
	void SetExiting(bool exiting) { exiting_ = exiting; }
	void AddLogEntry(LogEntryPtr entry)
	{
		Spinlock spinlock(lock_);
		buffer_.push_back(entry);
		subscribers_(shared_from_this(), entry);
	}
	bool IsConfigService() const { return cfgSvc_; }
	boost::signals2::signal<void(LogFilePtr, LogEntryPtr)>& GetSignal() { return subscribers_; }

private:
	void CheckForLogFile();
	bool Open(const char* filename);
	LogEntryPtr ParseLine(const std::string& line);
};

inline LockBuffer::LockBuffer(LogFile& logfile) : lock(logfile.lock_) {}
inline LockBuffer::LockBuffer(LogFilePtr logfile) : lock(logfile->lock_) {}

