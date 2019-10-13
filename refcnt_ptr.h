// Copyright (c) 2004, Eugene Gershnik
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef HEADER_REFCNT_PTR_H
#define HEADER_REFCNT_PTR_H

//
// refcnt_ptr 
// An intrusive reference counting smart pointer class template
// see http://www.gershnik.com/articles/refcnt_ptr.asp for details
//

template<typename T>
struct attachment_data
{ T * p; };

template<typename T>
struct reference_data
{ T * p; };

template<typename T>
attachment_data<T> noref(T * p)
{ 
	const attachment_data<T> ret = {p};
	return ret;
}

template<typename T>
reference_data<T> ref(T * p)
{ 
	const reference_data<T> ret = {p};
	return ret;
}

template<typename T>
class refcnt_ptr
{
private:
	struct dummy { int i; };
	typedef int dummy::*_bool;
public:
	refcnt_ptr() : m_p(0)
		{}

	refcnt_ptr(const refcnt_ptr<T> & src) : m_p(src.m_p)
		{ this->do_add_ref(m_p); }
	refcnt_ptr<T> & operator=(const refcnt_ptr<T> & src)
	{ 
		refcnt_ptr<T> temp(src);
		this->swap(temp);
		return *this;
	}

	template<typename Y>
	refcnt_ptr(const refcnt_ptr<Y> & src) : m_p(src.c_ptr())
		{ this->do_add_ref(m_p); }
	template<typename Y>
	refcnt_ptr<T> & operator=(const refcnt_ptr<Y> & src)
	{ 
		refcnt_ptr<T> temp(src);
		this->swap(temp);
		return *this;
	}


	//Attachment and reference to a raw pointer
	template<typename Y>
	refcnt_ptr(const attachment_data<Y> & src) : m_p(src.p)
	{ 
	}
	template<typename Y>
	refcnt_ptr<T> & operator=(const attachment_data<Y> & src)
	{ 
		refcnt_ptr<T> temp(src);
		this->swap(temp);
		return *this;
	}

	template<typename Y>
	refcnt_ptr(const reference_data<Y> & src) : m_p(src.p)
	{ 
		this->do_add_ref(m_p); 
	}

	template<typename Y>
	refcnt_ptr<T> & operator=(const reference_data<Y> & src)
	{ 
		refcnt_ptr<T> temp(src);
		this->swap(temp);
		return *this;
	}

	~refcnt_ptr()
		{ this->reset(); }

	T * operator->() const
		{ return m_p; }
	T & operator*() const
		{ return *m_p; }
	T * c_ptr() const
		{ return m_p; }
	
	operator _bool() const
		{ return m_p ? &dummy::i: 0; }

	T * release()
	{ 
		T * p = m_p;
		m_p = 0;
		return p;
	}
	void reset()
	{ 
		this->do_sub_ref(m_p); 
		m_p = 0;
	}

	void swap(refcnt_ptr<T> & other)
	{
		T * temp = m_p;
		m_p = other.m_p;
		other.m_p = temp;
	}

private:
	static void do_add_ref(T * p)
	{
		typedef char dummy[sizeof(T)];
		if (p) refcnt_add_ref(*p); 
	}
	static void do_sub_ref(T * p)
	{ 
		typedef char dummy[sizeof(T)];
		if (p) refcnt_sub_ref(*p); 
	}
private:
	T * m_p;
};

template<typename T1, typename T2>
bool operator==(const refcnt_ptr<T1> & lhs, const refcnt_ptr<T2> & rhs)
	{ return lhs.c_ptr() == rhs.c_ptr(); }
template<typename T, typename Y>
bool operator==(const refcnt_ptr<T> & lhs, const Y * rhs)
	{ return lhs.c_ptr() == rhs; }
template<typename T, typename Y>
bool operator==(const Y * lhs, const refcnt_ptr<T> & rhs)
	{ return lhs == rhs.c_ptr(); }

template<typename T1, typename T2>
bool operator!=(const refcnt_ptr<T1> & lhs, const refcnt_ptr<T2> & rhs)
	{ return !(lhs == rhs); }
template<typename T, typename Y>
bool operator!=(const refcnt_ptr<T> & lhs, const Y * rhs)
	{ return !(lhs == rhs); }
template<typename T, typename Y>
bool operator!=(const Y * lhs, const refcnt_ptr<T> & rhs)
	{ return !(lhs == rhs); }

template<typename T1, typename T2>
bool operator<(const refcnt_ptr<T1> & lhs, const refcnt_ptr<T2> & rhs)
	{ return lhs.c_ptr() < rhs.c_ptr(); }
template<typename T, typename Y>
bool operator<(const refcnt_ptr<T> & lhs, const Y * rhs)
	{ return lhs.c_ptr() < rhs; }
template<typename T, typename Y>
bool operator<(const Y * lhs, const refcnt_ptr<T> & rhs)
	{ return lhs < rhs.c_ptr(); }

template<typename T1, typename T2>
bool operator<=(const refcnt_ptr<T1> & lhs, const refcnt_ptr<T2> & rhs)
	{ return lhs.c_ptr() <= rhs.c_ptr(); }
template<typename T, typename Y>
bool operator<=(const refcnt_ptr<T> & lhs, const Y * rhs)
	{ return lhs.c_ptr() <= rhs; }
template<typename T, typename Y>
bool operator<=(const Y * lhs, const refcnt_ptr<T> & rhs)
	{ return lhs <= rhs.c_ptr(); }

template<typename T1, typename T2>
bool operator>(const refcnt_ptr<T1> & lhs, const refcnt_ptr<T2> & rhs)
	{ return !(lhs <= rhs); }
template<typename T, typename Y>
bool operator>(const refcnt_ptr<T> & lhs, const Y * rhs)
	{ return !(lhs <= rhs); }
template<typename T, typename Y>
bool operator>(const Y * lhs, const refcnt_ptr<T> & rhs)
	{ return !(lhs <= rhs); }


template<typename T1, typename T2>
bool operator>=(const refcnt_ptr<T1> & lhs, const refcnt_ptr<T2> & rhs)
	{ return !(lhs < rhs); }
template<typename T, typename Y>
bool operator>=(const refcnt_ptr<T> & lhs, const Y * rhs)
	{ return !(lhs < rhs); }
template<typename T, typename Y>
bool operator>=(const Y * lhs, const refcnt_ptr<T> & rhs)
	{ return !(lhs < rhs); }
template<typename Dest, typename Src>
inline
refcnt_ptr<Dest> refcnt_const_cast(const refcnt_ptr<Src> & p)
{
	return refcnt_ptr<Dest>(ref(const_cast<Dest *>(p.c_ptr())));
}

template<typename Dest, typename Src>
inline
refcnt_ptr<Dest> refcnt_dynamic_cast(const refcnt_ptr<Src> & p)
{
	return refcnt_ptr<Dest>(ref(dynamic_cast<Dest *>(p.c_ptr())));
}

template<typename Dest, typename Src>
inline
refcnt_ptr<Dest> refcnt_reinterpret_cast(const refcnt_ptr<Src> & p)
{
	return refcnt_ptr<Dest>(ref(reinterpret_cast<Dest *>(p.c_ptr())));
}

template<typename Dest, typename Src>
inline
refcnt_ptr<Dest> refcnt_static_cast(const refcnt_ptr<Src> & p)
{
	return refcnt_ptr<Dest>(ref(static_cast<Dest *>(p.c_ptr())));
}

#endif //HEADER_REFCNT_PTR_H

