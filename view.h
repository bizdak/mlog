#pragma once

#include <string>
#include <boost/regex.hpp>

struct LogHighlight
{
	int color;
	boost::regex pattern;
	bool bold;

	LogHighlight(int color, const char* pattern, bool bold = false)
	{
		this->color = color;
		this->bold = bold;
		this->pattern = boost::regex(pattern);
	}
};

class View
{
	std::string title_;
	std::string searchPattern_;

public:
	virtual ~View() {}
	
	const std::string& Title() const { return title_; }
	void SetTitle(const char* title) { title_ = title; }
	
	virtual void Init() = 0;
	virtual bool Update(int c) = 0;
	virtual void Render() = 0;
	virtual void Resize() = 0;

	virtual const std::string& GetCurrentSearchPattern() const { return searchPattern_; }
	virtual void SetSearchPattern(const std::string& searchPattern) { searchPattern_ = searchPattern; }
};

typedef std::shared_ptr<View> ViewPtr;
