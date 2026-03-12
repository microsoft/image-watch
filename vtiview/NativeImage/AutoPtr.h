#pragma once

#include "Namespace.h"

using namespace System;

BEGIN_NI_NAMESPACE;

template <typename T>
ref class AutoPtr
{
public:
	AutoPtr()
		: m_ptr(0)
	{
	}

	~AutoPtr()	
	{		
		Release();
	}

	void Reset(T* ptr)
	{
		Release();
		m_ptr = ptr;
	}

	void Release()
	{
		delete m_ptr;
		m_ptr = 0;
	}

	void ReleaseNoDelete()
	{
		m_ptr = 0;
	}

	T* Get()
	{
		if (m_ptr != 0)
			return m_ptr;
		
		throw gcnew ObjectDisposedException(
			String::Format("Referenced object of type {0} has already "
			"been disposed", T::typeid));
	}

	T* operator->()
	{
		return Get();
	}

	bool IsNull()
	{
		return m_ptr == 0;
	}
	
private:
	T* m_ptr;
};

template <typename T>
ref class AutoComPtr
{
public:
	AutoComPtr()
		: m_ptr(0)
	{
	}

	~AutoComPtr()	
	{		
		Release();
	}

	void Reset(T* ptr)
	{
		Release();
		m_ptr = ptr;
	}

	void Release()
	{
		if (m_ptr != 0)
			m_ptr->Release();
		m_ptr = 0;
	}

	T* Get()
	{
		if (m_ptr != 0)
			return m_ptr;
		
		throw gcnew ObjectDisposedException(
			String::Format("Referenced object of type {0} has already "
			"been disposed", T::typeid));
	}

	T* operator->()
	{
		return Get();
	}

	bool IsNull()
	{
		return m_ptr == 0;
	}
	
private:
	T* m_ptr;
};

END_NI_NAMESPACE