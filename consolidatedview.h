#pragma once

#pragma once

#include "view.h"
#include "mainui.h"
#include "logfile.h"

struct LogEntryEx : LogEntry
{
	LogEntryEx& operator=(const LogEntry& rhs) {
		id = rhs.id;
		id = rhs.severity;
		tid = rhs.tid;
		time = rhs.time;
		timestamp = rhs.timestamp;
		type = rhs.type;
		body = rhs.body;
		return *this;
	}
	std::string filename;
};

class ConsolidatedLogView : public View
{
	MainUi* ui_;
	std::shared_ptr<Window> win_;
	int row_ = 0;
	int col_ = 0;
	int id_;
	boost::regex highlighPattern_;

	boost::circular_buffer<std::shared_ptr<LogEntryEx>> buffer_;
	std::vector<std::shared_ptr<LogFile>> files_;

public:
	ConsolidatedLogView(MainUi* ui, std::shared_ptr<Window> win, std::vector<std::shared_ptr<LogFile>> files);
	virtual void Init();
	virtual bool Update(int c);
	virtual void Resize();
	virtual void Render();
	virtual void SetSearchPattern(const std::string& searchPattern);

private:
	void RefreshBuffer();
	void RenderBody(int y, int x, int width, const std::string& body, int offset);
	void SetPosition();
};
