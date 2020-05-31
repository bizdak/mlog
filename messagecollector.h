#pragma once

#include <string>
#include <boost/circular_buffer.hpp>
#include "logfile.h"
#include <map>

// collects information about messages from various sources
// specifically: MMReceiver, MMEngine and MMSender

struct MessageInfo
{
	std::string messageName;
	boost::circular_buffer<LogEntryPtr> rxLogs;
	boost::circular_buffer<LogEntryPtr> engLogs;
	boost::circular_buffer<LogEntryPtr> txLogs;

	MessageInfo() : rxLogs(10), engLogs(10), txLogs(10) {}
};

class MessageCollector 
{
	std::map<std::string, MessageInfo> messages_;

public:
	enum class LogType { receiver, engine, sender };

	MessageCollector();

	void AddLogFile(LogType logType, LogFilePtr logFile);

private:
	void ParseReceiverLog(LogFilePtr file, LogEntryPtr entry);
	void ParseEngineLog(LogFilePtr file, LogEntryPtr entry);
	void ParseSenderLog(LogFilePtr file, LogEntryPtr entry);
};

typedef std::shared_ptr<MessageCollector> MessageCollectorPtr;

