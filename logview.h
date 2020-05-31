#pragma once

#include "view.h"
#include "mainui.h"
#include "logfile.h"

class LogView : public View
{
	MainUi* ui_;
	std::shared_ptr<Window> win_;
	std::shared_ptr<LogFile> file_;
	int row_ = 0;
	int col_ = 0;
	int id_;
	bool tail_;
	boost::regex highlighPattern_;

	bool highlight_ = true;
	std::vector<LogHighlight> highlights_;

public:
	LogView(MainUi* ui, std::shared_ptr<Window> win, std::shared_ptr<LogFile> file);
	virtual void Init();
	virtual bool Update(int c);
	virtual void Resize();
	virtual void Render();
	virtual void SetSearchPattern(const std::string& searchPattern);
	void SetPosition();

private:
	void RenderBody(int y, int x, int width, const std::string& body, int offset);
	void ColorizeMatch(int y, int x, int width, const std::string& body, int offset, const boost::regex& pattern, int color);
};
