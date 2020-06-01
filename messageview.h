#pragma once

#include "view.h"
#include "messagecollector.h"
#include "mainui.h"

class MessageView : public View
{
	MainUi* ui_;
	std::shared_ptr<Window> win_;
	MessageCollectorPtr collector_;
	boost::circular_buffer<MessageInfoPtr> buffer_;

public:
	MessageView(MainUi* ui, std::shared_ptr<Window> win, MessageCollectorPtr collector);
	~MessageView();

	virtual void Init();
	virtual bool Update(int c);
	virtual void Render();
	virtual void Resize();

private:
	void RefreshBuffer();
	void SetPosition();

	int msgBegY_;	// screen position of beggining of row
	int msgEndY_;	// use to see if selected message is visible
	int startRow_ = 0;	// the index in the buffer that is rendered as the beginning row
	int selectedIdx_ = 0;
	bool tail_ = true;

	void RenderMessageView(int starty, int maxy);
	void RenderDetailView(int starty, int maxy);
	int RenderDetailLogView(int starty, int maxy, const char* title, const boost::circular_buffer<LogEntryPtr>& logs);
	void EnsureVisible();
	void CalculateMessageViewSize();
};
