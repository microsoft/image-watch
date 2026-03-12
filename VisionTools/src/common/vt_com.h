#pragma once

// TODO: Mac - AddRef and Release were removed I believe correct?
namespace vt
{
	template <class T>
	class _NoAddRefReleaseOnVTComPtr : public T
	{
	private:
		// hide AddRef and Release (including dummy argument to create
		// different signature required for name-hiding)
		void AddRef(int);
		void Release(int);
	};

	template <class T>
	class VTComPtr
	{
	public:
		VTComPtr() : p(NULL)
		{}

		VTComPtr(IN OUT OPTIONAL T* lp) throw()
		{
			p = lp;
#if !defined(VT_OSX)
			if (p != NULL)
				p->AddRef();
#endif
        }

		~VTComPtr()
		{
#if !defined(VT_OSX)
			if (p)
				p->Release();
#endif
		}

		void operator=(IN OUT OPTIONAL T* lp) throw()
		{
			T* tmp = p;
			p = lp;
			if (p)
			{
				p->AddRef();
			}
			if (tmp)
			{
				tmp->Release();
			}
		}

		T* Detach() throw()
		{
			T* pt = p;
			p = NULL;
			return pt;
		}

		operator T*() const throw()
		{
			return p;
		}

		T** operator&() throw()
		{
			VT_ASSERT(p == NULL);
			return &p;
		}

		_NoAddRefReleaseOnVTComPtr<T>* operator->() const throw()
		{
			VT_ASSERT(p != NULL);
			return (_NoAddRefReleaseOnVTComPtr<T>*)p;
		}

	private:
		T* p;
	};
}