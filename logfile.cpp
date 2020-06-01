#pragma once
#include "logfile.h"

#include <boost/regex.hpp>
#include <boost/circular_buffer.hpp>

LogFile::LogFile(fs::path logDir, std::string namePrefix, bool cfgSvc, int bufSize) 
	: fp_(nullptr), buffer_(bufSize), exiting_(false)
	, logDir_(logDir), namePrefix_(namePrefix), cfgSvc_(cfgSvc)
{
	pattern_ = cfgSvc
		? boost::regex("^(\\d{4}-\\d{2}-\\d{2})\\s(\\d{2}:\\d{2}:\\d{2}\\.\\d{3}).*?\\[(.*?)\\]\\s\\[(.*?)\\]\\s(.*)")
		: boost::regex("^(\\d+)\\s(\\d{2}:\\d{2}:\\d{2}\\.\\d{3})\\s(.*)$");
}

LogFile::~LogFile()
{
	Close();
}

bool LogFile::Open(const char* filename)
{
	Close();
	fp_ = _fsopen(filename, "rt", _SH_DENYNO);
	if (fp_ == nullptr)
		return false;

	const int Threshold = 20 * 8192;
	struct stat st;
	fstat(_fileno(fp_), &st);
	if (st.st_size > Threshold) {
		auto bytes = std::min((int)st.st_size, Threshold);
		fseek(fp_, -bytes, SEEK_END);

		// forward to the next line
		for (int c = fgetc(fp_); c != EOF && c != '\n' && !ferror(fp_);)
			c = fgetc(fp_);
	}

	return true;
}

void LogFile::Close()
{
	if (fp_)
		fclose(fp_), fp_ = nullptr;
}

void LogFile::CheckForLogFile()
{
	// only check once every 2 seconds
	if (lastLogCheck_ + 2 > time(nullptr))
		return;

	if (!fs::exists(logDir_) || !fs::is_directory(logDir_)) {
		if (buffer_.size() == 0) {
			auto entry = std::make_shared<LogEntry>();
			entry->timestamp = time(nullptr);
			entry->type = MessageType::system;
			entry->body = "Directory does not exist or is not a directory: " + logDir_.string();
			DLog("%s\n", entry->body.c_str());
			AddLogEntry(entry);
		}
		return;
	}

	try {
		namespace fs = boost::filesystem;
		// instead of basing this on the filename, we're going to find the latest modified log file
		lastLogCheck_ = time(nullptr);
		fs::directory_entry latest;
		for (auto p : fs::directory_iterator(logDir_)) {
			if (!fs::is_regular_file(p))
				continue;
			if (_strnicmp(p.path().filename().string().c_str(), namePrefix_.c_str(), namePrefix_.size()) != 0)
				continue;

			if (latest == fs::directory_entry() || fs::last_write_time(p) > fs::last_write_time(latest))
				latest = p;
		}
		if (latest.path() == logFile_) {
			return;	// no file found or we've aleady opened the latest
		}
		if (fs::exists(latest) && Open(latest.path().string().c_str())) {
			logFile_ = latest;
			auto entry = std::make_shared<LogEntry>();
			entry->timestamp = time(nullptr);
			entry->type = MessageType::system;
			entry->body = "Opened " + logFile_.string();
			DLog("%s\n", entry->body.c_str());
			AddLogEntry(entry);
		}
		else if (buffer_.size() == 0) {
			auto entry = std::make_shared<LogEntry>();
			entry->timestamp = time(nullptr);
			entry->type = MessageType::system;
			entry->body = "Error loading log file for " + latest.path().string();
			DLog("%s\n", entry->body.c_str());
			AddLogEntry(entry);
		}
	}
	catch (std::exception& e) {
		if (buffer_.size() == 0) {
			auto entry = std::make_shared<LogEntry>();
			entry->timestamp = time(nullptr);
			entry->type = MessageType::system;
			entry->body = "Error loading log file for " + namePrefix_ + " - " + std::string(e.what());
			DLog("%s\n", entry->body.c_str());
			AddLogEntry(entry);
		}
	}
}

void LogFile::Tail()
{	
	if (paused_)
		return;
	CheckForLogFile();
	if (fp_ == nullptr)
		return;

	std::string line;
	int lineNo = 0;
	while (!exiting_) {
		int c = fgetc(fp_);
		if (c == EOF) {
			if (ferror(fp_)) {
				if (!line.empty()) {
					AddLogEntry(ParseLine(line));
					line.clear();
				}
				break;
			}
			clearerr(fp_);
			return;
		}
		else {
			if (c == '\t')
				line += "    ";
			else
				line += (char)c;
			if (c == '\n') {
				AddLogEntry(ParseLine(line));
				line.clear();
			}
		}
	}
}

std::shared_ptr<LogEntry> LogFile::ParseLine(const std::string& line)
{
	auto entry = std::make_shared<LogEntry>();
	entry->timestamp = time(nullptr);
	boost::match_results<std::string::const_iterator> match;
	if (boost::regex_search(line, match, pattern_, boost::match_default)) {
		if (!cfgSvc_) {
			entry->type = MessageType::normal;
			entry->time = std::string(match[2].first, match[2].second);
			entry->tid = atoi(std::string(match[1].first, match[1].second).c_str());
			entry->body = std::string(match[3].first, match[3].second);
		}
		else {
			entry->type = MessageType::cfgSvc;
			entry->time = std::string(match[2].first, match[2].second);
			entry->severity = std::string(match[3].first, match[3].second);
			entry->id = std::string(match[4].first, match[4].second);
			entry->body = std::string(match[5].first, match[5].second);
		}
	}
	else {
		entry->type = MessageType::continuation;
		entry->body = line;
	}
	return entry;
}

