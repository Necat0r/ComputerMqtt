#pragma once

template <class T>
class SafePtr
{
public:
	using this_type = SafePtr<T>;

	inline SafePtr()
		: m_ptr(nullptr)
	{}

	inline SafePtr(T* ptr)
		: m_ptr(ptr)
	{}

	SafePtr(const this_type&) = delete;

	inline SafePtr(this_type&& other)
	{
		auto tmp = m_ptr;
		m_ptr = other.m_ptr;
		other.m_ptr = tmp;
	}

	inline ~SafePtr()
	{
		release();
	}

	typename T& operator=(const this_type&) = delete;

	inline typename this_type& operator=(this_type&& other)
	{
		auto tmp = m_ptr;
		m_ptr = other.m_ptr;
		other.m_ptr = tmp;

		return *this;
	}

	inline typename this_type& operator=(typename T* ptr)
	{
		release();
		m_ptr = ptr;

		return *this;
	}

	inline typename T** operator&()
	{
		return &m_ptr;
	}

	inline typename T* operator->()
	{
		return m_ptr;
	}

	inline typename T& operator*()
	{
		return *m_ptr;
	}

	inline operator bool() const
	{
		return m_ptr != nullptr;
	}

	inline void release()
	{
		if (m_ptr)
			m_ptr->Release();
		m_ptr = nullptr;
	}

private:
	T* m_ptr = nullptr;
};
