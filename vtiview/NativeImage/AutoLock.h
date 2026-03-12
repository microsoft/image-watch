#pragma once

#include "Namespace.h"

using namespace System;

BEGIN_NI_NAMESPACE;

public ref class AutoLock 
{
private:
	bool hasLock_;
	Object^ object_;

public:
	AutoLock(Object^ obj, int timeOutMillis) : object_(obj) 
	{
		hasLock_ = System::Threading::Monitor::TryEnter(obj, timeOutMillis);
	}

	AutoLock(Object^ obj) : object_(obj) 
	{
		System::Threading::Monitor::Enter(obj);
		hasLock_ = true;
	}

	~AutoLock() 
	{
		if (hasLock_)
			System::Threading::Monitor::Exit(object_);
	}

	bool HasLock()
	{
		return hasLock_;
	}
};

END_NI_NAMESPACE;