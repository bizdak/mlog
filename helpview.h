#pragma once

#include "mainui.h"
#include "wincurses.h"

class HelpView : public View
{
public:
	HelpView(MainUi* ui, std::shared_ptr<Window> win);

	virtual void Init();
	virtual bool Update(int c);
	virtual void Resize();
	virtual void Render();

private:
	MainUi* ui_;
	std::shared_ptr<Window> win_;
	int row_ = 0;
	int col_ = 0;
	std::vector<std::string> buffer_;
};
