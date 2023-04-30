#pragma once

#ifdef DEBUG_MEMORY_NAME

template<class T, class...TArgs>
IC T* xr_new(TArgs&&... args)
{
	T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new(ptr) T(std::forward<TArgs>(args)...);
}

#else

template<class T, class...TArgs>
IC T* xr_new(TArgs&&... args)
{
	protectXrNew_.Enter();
	T* ptr = (T*)Memory.mem_alloc(sizeof(T));
	protectXrNew_.Leave();

	T* newly_created = new(ptr)T(std::forward<TArgs>(args)...);

	return newly_created;
}
#endif

template <bool _is_pm, typename T>
struct xr_special_free
{
	IC void operator()(T*& ptr)
	{
		void* _real_ptr = dynamic_cast<void*>(ptr);
		ptr->~T();
		xr_free(_real_ptr);
	}

	IC void operator()(T* const & ptr)
	{
		void* _real_ptr = dynamic_cast<void*>(ptr);
		ptr->~T();
		xr_free(_real_ptr);
	}
};

template <typename T>
struct xr_special_free < false, T >
{
	IC void operator()(T*& ptr)
	{
		ptr->~T();
		xr_free(ptr);
	}
};

template <class T>
IC void xr_delete(T*& ptr)
{
	if (ptr)
	{
		xr_special_free<is_polymorphic<T>::result, T>()(ptr);
		ptr = NULL;
	}
}

template <class T>
IC void xr_delete(T* const& ptr)
{
	if (ptr)
	{
		xr_special_free<::is_polymorphic<T>::result, T>()(ptr);
		((T*&)ptr) = NULL;
	}
}

#ifdef DEBUG_MEMORY_MANAGER
	void XRCORE_API mem_alloc_gather_stats				(const bool &value);
	void XRCORE_API mem_alloc_gather_stats_frequency	(const float &value);
	void XRCORE_API mem_alloc_show_stats				();
	void XRCORE_API mem_alloc_clear_stats				();
#endif