#include "MessageCollector.h"

MessageCollector::MessageCollector()
{

}

const void MessageCollector::FillBuffer(boost::circular_buffer<MessageInfoPtr>& buf) const
{
	Spinlock lock(lock_);
	buf.clear();
	for (auto it : messages_) 
		buf.push_back(it.second);
}

void MessageCollector::AddLogFile(LogType logType, LogFilePtr logFile)
{
	auto collector = shared_from_this();
	switch (logType)
	{
	case LogType::receiver:
		logFile->GetSignal().connect([collector](LogFilePtr file, LogEntryPtr entry) {
			collector->ParseReceiverLog(file, entry);
			});
		break;

	case LogType::engine:
		logFile->GetSignal().connect([collector](LogFilePtr file, LogEntryPtr entry) {
			collector->ParseEngineLog(file, entry);
			});
		break;

	case LogType::sender:
		logFile->GetSignal().connect([collector](LogFilePtr file, LogEntryPtr entry) {
			collector->ParseSenderLog(file, entry);
			});
		break;
	}
}

MessageInfoPtr MessageCollector::CreateMessageInfo(const std::string msgName)
{
	auto it = messages_.find(msgName);
	if (it != messages_.end()) {
		it->second->touchTime = time(nullptr);
		return it->second;
	}

#ifdef _DEBUG
	const int Threshold = 10;
	const int EntriesToRemove = 3;
#else
	const int Threshold = 1000;
	const int EntriesToRemove = 100;
#endif

	if (messages_.size() > Threshold) {
		DLog("Expiring entries out of the message collector\n");
		std::vector<MessageInfoPtr> sorted(messages_.size());
		for (auto tmp : messages_) 
			sorted.push_back(tmp.second);

		std::sort(sorted.begin(), sorted.end(), [](MessageInfoPtr lhs, MessageInfoPtr rhs) { return lhs->touchTime < rhs->touchTime; });
		for (int i = 0; i < EntriesToRemove; i++) 
			messages_.erase(messages_.find(sorted[i]->messageName));
		DLog("Removed %d entries from message collector\n", EntriesToRemove);
	}

	auto ptr = std::make_shared<MessageInfo>();
	ptr->touchTime = time(nullptr);
	ptr->messageName = msgName;
	messages_[msgName] = ptr;
	return ptr;
}

void MessageCollector::ParseReceiverLog(LogFilePtr file, LogEntryPtr entry)
{
	if (entry->type != MessageType::normal)
		return;

	static boost::regex arrivePattern = boost::regex("^TX:\\s<250\\s(\\w+)\\sMessage\\saccepted\\sfor\\sdelivery>$");
	static boost::regex msgPattern = boost::regex("([A-E][\\da-fA-F]{12})\\.[\\da-fA-F]{12}\\.[\\da-fA-F]{4}\\.mml");
	static boost::regex spamProfilerPattern = boost::regex("^([A-E][\\da-fA-F]{12})\\.[\\da-fA-F]{12}\\.[\\da-fA-F]{4}\\.mml\\sSpamProfiler\\sscore:\\s(\\d+),\\srescan:\\s(\\d+),\\sbulk:\\s(\\d+)");
	
	boost::match_results<std::string::const_iterator> match;
	if (boost::regex_search(entry->body, match, arrivePattern, boost::match_default)) {
		assert(match.length() > 1);
		auto msgName = std::string(match[1].begin(), match[1].end());
		DLog("Recv: <%s>\n", msgName.c_str());
		auto msgInfo = CreateMessageInfo(msgName);
		msgInfo->rxTime = entry->time;
		msgInfo->rxLogs.push_back(entry);
	}
	else if (boost::regex_search(entry->body, match, spamProfilerPattern, boost::match_default)) {
		assert(match.length() > 5);
		auto msgName = std::string(match[1].begin(), match[1].end());
		auto score = std::string(match[2].begin(), match[2].end());
		auto rescan = std::string(match[3].begin(), match[3].end());
		auto bulk = std::string(match[4].begin(), match[4].end());
		auto msgInfo = CreateMessageInfo(msgName);
		msgInfo->spamProfilerScore = atoi(score.c_str());
		msgInfo->spamProfilerRescan = atoi(rescan.c_str());
		msgInfo->spamProfilerBulk = atoi(bulk.c_str());
		msgInfo->rxLogs.push_back(entry);
	}
	else if (boost::regex_search(entry->body, match, msgPattern, boost::match_default)) {
		assert(match.length() > 2);
		auto msgName = std::string(match[1].begin(), match[1].end());
		DLog("Recv: %s\n", entry->body.c_str());
		auto msgInfo = CreateMessageInfo(msgName);
		msgInfo->rxLogs.push_back(entry);
	}
}

void MessageCollector::ParseEngineLog(LogFilePtr file, LogEntryPtr entry)
{
	static boost::regex msgPattern = boost::regex("([A-E][\\da-fA-F]{12})\\.[\\da-fA-F]{12}\\.[\\da-fA-F]{4}\\.mml");

	boost::match_results<std::string::const_iterator> match;
	if (boost::regex_search(entry->body, match, msgPattern, boost::match_default)) {
		assert(match.length() > 1);
		auto msgName = std::string(match[1].begin(), match[1].end());
		DLog("Eng: %s\n", entry->body.c_str());
		auto msgInfo = CreateMessageInfo(msgName);
		if (msgInfo->engTimeFirst.empty())
			msgInfo->engTimeFirst = entry->time;
		msgInfo->engTimeLatest = entry->time;
		msgInfo->engLogs.push_back(entry);
	}

}

void MessageCollector::ParseSenderLog(LogFilePtr file, LogEntryPtr entry)
{
	static boost::regex msgPattern = boost::regex("([A-E][\\da-fA-F]{12})\\.[\\da-fA-F]{12}\\.[\\da-fA-F]{4}\\.mml");

	boost::match_results<std::string::const_iterator> match;
	if (boost::regex_search(entry->body, match, msgPattern, boost::match_default)) {
		assert(match.length() > 1);
		auto msgName = std::string(match[1].begin(), match[1].end());
		DLog("Send: %s\n", entry->body.c_str());
		auto msgInfo = CreateMessageInfo(msgName);
		if (msgInfo->txTimeFirst.empty())
			msgInfo->txTimeFirst = entry->time;
		msgInfo->txTimeLatest = entry->time;
		msgInfo->txLogs.push_back(entry);
	}
}
