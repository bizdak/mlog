#pragma once

#include "view.h"
#include "messagecollector.h"

class MessageView : public View
{
	MessageCollectorPtr msgCollector_;
public:
	MessageView();
	~MessageView();

	virtual void Init();
	virtual bool Update(int c);
	virtual void Render();
	virtual void Resize();
};
