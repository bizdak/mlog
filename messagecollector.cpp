#include "MessageCollector.h"

MessageCollector::MessageCollector()
{

}

void MessageCollector::AddLogFile(LogType logType, LogFilePtr logFile)
{
	switch (logType)
	{
	case LogType::receiver:
		logFile->GetSignal().connect([this](LogFilePtr file, LogEntryPtr entry) {
			ParseReceiverLog(file, entry);
			});
		break;

	case LogType::engine:
		logFile->GetSignal().connect([this](LogFilePtr file, LogEntryPtr entry) {
			ParseEngineLog(file, entry);
			});
		break;

	case LogType::sender:
		logFile->GetSignal().connect([this](LogFilePtr file, LogEntryPtr entry) {
			ParseSenderLog(file, entry);
			});
		break;
	}
}

void MessageCollector::ParseReceiverLog(LogFilePtr file, LogEntryPtr entry)
{
}

void MessageCollector::ParseEngineLog(LogFilePtr file, LogEntryPtr entry)
{

}

void MessageCollector::ParseSenderLog(LogFilePtr file, LogEntryPtr entry)
{

}
