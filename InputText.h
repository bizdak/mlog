#pragma once

#include "wincurses.h"
#include <string>

class InputText
{
	std::shared_ptr<Window> parent_;
	int x_, y_;
	std::string label_;
	std::string text_;

public:
	InputText(std::shared_ptr<Window> parent, int y, int x, const char* label)
		: parent_(parent), y_(y), x_(x), label_(label)
	{
	}

	int GetInput()
	{
		int c = getch();
		if (c == ERR)
			return 0;

		switch (c) {
		case 27: {
			text_.clear();
			return -1;
		}

		case '\n': case '\r': case KEY_ENTER:
			return 1;

		case '\b':
			if (text_.size() > 0)
				text_.resize(text_.size() - 1);
			break;

		default:
			if (isascii(c))
				text_ += c;
			break;
		}
		return 0;
	}

	std::string GetText() const { return text_; }
	void SetText(const std::string& text) { text_ = text; }

	void Render()
	{
		parent_->Move(y_, x_);
		parent_->PrintF("%s%s", label_.c_str(), text_.c_str());
		parent_->ClearToEol();
	}
};
