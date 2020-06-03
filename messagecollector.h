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
	time_t touchTime = 0;

	std::string rxTime;			// time message was received
	std::string engTimeFirst;	// earliest time message was seen in engine
	std::string engTimeLatest;	// latest time seen, may be reprocessed or different edition
	std::string txTimeFirst;
	std::string txTimeLatest;

	int spamProfilerScore = 0;
	int spamProfilerRescan = 0;
	int spamProfilerBulk = 0;

	MessageInfo() : rxLogs(10), engLogs(10), txLogs(10) {}
};

typedef std::shared_ptr<MessageInfo> MessageInfoPtr;

class MessageCollector : public std::enable_shared_from_this<MessageCollector>
{
	std::map<std::string, MessageInfoPtr> messages_;
	friend class LockCollector;
	mutable std::atomic_flag lock_;

public:
	enum class LogType { receiver, engine, sender };

	MessageCollector();

	void AddLogFile(LogType logType, LogFilePtr logFile);

	const void FillBuffer(std::vector<MessageInfoPtr>& buf) const;

private:
	void ParseReceiverLog(LogFilePtr file, LogEntryPtr entry);
	void ParseEngineLog(LogFilePtr file, LogEntryPtr entry);
	void ParseSenderLog(LogFilePtr file, LogEntryPtr entry);
	MessageInfoPtr CreateMessageInfo(const std::string msgName);
};

typedef std::shared_ptr<MessageCollector> MessageCollectorPtr;


