
/*
MIT License

Copyright (c) 2020 Risto Lankinen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <assert.h>
#include <iostream>
#include <type_traits>

//**********************************************************************************************************************

#define FAIL(why) do { std::cerr << std::endl << "Function '" __FUNCTION__ "(...)' failed: " why "." << std::endl; abort(); } while(false)
#define PROFILER do { static auto f=__FUNCTION__; static auto l=__LINE__; static struct S { int n{}; ~S() { std::cerr << "PROFILER: Function '" << f << "(...)' line " << l << " was invoked " << n << " times." << std::endl; } } s; ++s.n; } while (false)
#define TODO do { throw "TODO: Function '" __FUNCTION__ "(...)'."; } while(false)
#define UNREACHABLE do { std::cerr << std::endl << "Executing code that was thought to be unreachable at '" __FUNCTION__ "(...)' line " << __LINE__ << "." << std::endl; abort(); } while(false)
#if !defined(_DEBUG)
#define WARN(why)
#else
#define WARN(why) do { static auto f=__FUNCTION__; static auto l=__LINE__; static struct S { ~S() { std::cerr << "WARNING: " why " in function '" << f << "(...)' line " << l << "." << std::endl; } } s; } while (false)
#endif

/***********************************************************************************************************************
*** Shared
***********************************************************************************************************************/

struct Shared
{
	template <typename T, typename = typename std::enable_if<std::is_base_of<Shared, T>::value>::type> static inline T const* Clone(T const* p) noexcept { if (p) ++p->nShared; return p; }
	template <typename T, typename = typename std::enable_if<std::is_base_of<Shared, T>::value>::type> static inline T const* Clone(T const& r) noexcept { ++r.nShared; return &r; }
	template <typename T, typename = typename std::enable_if<std::is_base_of<Shared, T>::value>::type> static inline T* Clone(T* p) noexcept { if (p) ++p->nShared; return p; }
	template <typename T, typename = typename std::enable_if<std::is_base_of<Shared, T>::value>::type> static inline T* Clone(T& r) noexcept { ++r.nShared; return &r; }
	static inline void Erase(Shared const* p) noexcept { if (p && !--p->nShared) delete p; }

protected:
	Shared() noexcept : nShared(1) { }
	Shared(Shared const&) noexcept : nShared(1) { }
	virtual ~Shared() noexcept = 0;

	bool IsShared() const noexcept { return nShared > 1; }

private:
	mutable size_t nShared;

	Shared(Shared&&) = delete;
	Shared& operator=(Shared&&) = delete;
	Shared& operator=(Shared const&) = delete;
};

inline Shared::~Shared()
{
	assert(!IsShared());
}

/***********************************************************************************************************************
*** Saved
***********************************************************************************************************************/

template <typename T> struct Saved final
{
	Saved() = delete;
	Saved(Saved const& r) : item(r.reference), reference(r.reference) { }
	Saved(T& r) : item(r), reference(r) { }
	Saved(T&& r) : item(r), reference(r) { }
	~Saved() { reference = item; }

	operator T& () noexcept { return reference; }
	Saved& operator=(T const& r) { reference = r; return *this; }

	void* operator new(size_t) = delete;

private:
	T const item;
	T& reference;
};

/***********************************************************************************************************************
*** Objectguard
***********************************************************************************************************************/

#if !defined(_DEBUG) && !defined(VERBOSE)
template <typename = void> struct ObjectGuard { };
#else

#pragma intrinsic(memcpy)

namespace
{
	struct Guarded
	{
		mutable Guarded const* pNext;
		mutable Guarded const* pPrev;
		size_t nSerialNumber;

		static size_t nCreationCount;

		Guarded() noexcept : pNext(this), pPrev(this), nSerialNumber(++nCreationCount)
		{
		}

		Guarded(Guarded const& r) noexcept : nSerialNumber(++nCreationCount), pNext(&r), pPrev(pNext->pPrev)
		{
			pNext->pPrev = this;
			pPrev->pNext = this;
		}

		virtual ~Guarded() noexcept
		{
			pNext->pPrev = pPrev;
			pPrev->pNext = pNext;
		}

		Guarded& operator=(Guarded const&) = delete;
	};

	size_t Guarded::nCreationCount = 0;
}

template <typename T> struct ObjectGuard : public Guarded
{
	static struct data final
	{
		Guarded root;
		char const* szClassName;
		unsigned int nCreated;
		unsigned int nDeleted;
		unsigned int nHighest;

		data() : root(), szClassName(nullptr), nCreated(0), nDeleted(0), nHighest(0)
		{
			size_t const PREFIX = sizeof "ObjectGuard<" - 1;
			size_t const SUFFIX = sizeof ">::data::data" - 1;
			size_t const size = sizeof __FUNCTION__ - PREFIX - SUFFIX;
			static char buffer[size];

			memcpy(buffer, __FUNCTION__ + PREFIX, size);
			buffer[size - 1] = '\0';
			szClassName = buffer;
		}

		~data()
		{
			int limit = 8;
			switch (nCreated - nDeleted)
			{
			case 0:
#if defined(VERBOSE)
				std::cerr << "Created and deleted " << nCreated << " objects of type <" << szClassName << "> (of which at most " << nHighest << " existed simultaneously)" << std::endl;
#endif
				break;

			case 1:
				std::cerr << "Object Guard: 1 object of type <" << szClassName << "> was leaked." << std::endl << "   Serial number of the leaked object is " << root.pNext->nSerialNumber << ")." << std::endl << std::endl;
				break;

			default:
				std::cerr << "Object Guard: " << nCreated - nDeleted << " objects of type <" << szClassName << "> were leaked." << std::endl << "   Serial numbers of the leaked objects are ";
				for (Guarded const* p = root.pNext; p != &root; p = p->pNext)
				{
					if (--limit < 0) { std::cerr << "... and more"; break; }
					if (p->pNext == &root) { std::cerr << " and "; ++limit; }
					else if (p != root.pNext) { std::cerr << ", "; }
					std::cerr << p->nSerialNumber;
				}
				std::cerr << "." << std::endl << std::endl;
				break;
			}
		}

		data& operator++() { if (++nCreated > nHighest + nDeleted) { ++nHighest; } return *this; }
		data& operator--() { ++nDeleted; return *this; }

	} &instance() { static data d; return d; };

	ObjectGuard() : Guarded(instance().root) { ++instance(); }
	ObjectGuard(ObjectGuard const&) : Guarded(instance().root) { ++instance(); }

protected:
	~ObjectGuard() { --instance(); }
};

#endif

//**********************************************************************************************************************

