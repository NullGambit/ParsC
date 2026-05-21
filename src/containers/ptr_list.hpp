#pragma once
#include <type_traits>

namespace pars
{
	// a zero allocation list that just ties pointers together
	// since most memory in the compiler is preallocated and never freed
	// this can be done safely and efficiently
	template<class T>
	class PtrList
	{
	public:
		struct PtrListNode
		{
			T *current;
			T *next;
		};

		void add(T *element)
		{

		}

	private:
		PtrListNode m_root;
		PtrListNode m_tail;
	};
}
