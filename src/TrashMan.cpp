#include "../inc/TrashMan.h"
#include "gc.h"

#include <string.h>
#include <assert.h>

namespace TrashMan
{

	void Init()
	{
		//GC_set_all_interior_pointers(1);
		//GC_set_full_freq(0);
		//GC_enable_incremental();
		GC_set_java_finalization(1);
		GC_INIT();
	}
	
	void Destroy()
	{
	}
	
	void Collect()
	{
		GC_gcollect();
	}
	
	void *Alloc(size_t s)
	{
		return GC_MALLOC(s);
	}
	
	void *AllocData(size_t s)
	{
		return GC_MALLOC_ATOMIC(s);
	}
	
///////////////////////////
	
	class Stack
	{
		char *mHead;
		char *mTail;
		char *mEnd;
		unsigned int mCurrentSize;
	public:
		Stack(size_t size);
		
		~Stack();
		
		void *Alloc(unsigned int size, unsigned int alignment = 4);
		
		template <class T, int ALIGN = 4>
		T *Alloc()
		{
			return (T*)Alloc(sizeof(T), ALIGN);
		}
		
		void Push();
		
		void Pop();
	};
	
	inline unsigned int RoundToM(unsigned int n, unsigned int m)
	{
		int r = n % m;
		return (r == 0) ? n : n + m - r;
	}

	Stack::Stack(size_t size)
	{
		mCurrentSize = 0;
		mTail = mHead = (char*)GC_MALLOC_UNCOLLECTABLE(size);
		mEnd = mHead + size;
	}
	
	Stack::~Stack()
	{
		if (mCurrentSize > 0)
		{
			::memset(mHead, 0, (size_t)(mEnd - mHead));
		}
		GC_FREE(mHead);
		
		mHead = 0;
		mTail = 0;
		mEnd = 0;
		
		// Force a collection to remove everything on this stack
		TrashMan::Collect();
	}
	
	void *Stack::Alloc(unsigned int size, unsigned int alignment)
	{
		mCurrentSize = RoundToM(mCurrentSize, alignment);
		assert((mTail + size + mCurrentSize) < mEnd);
		void *current = mTail + mCurrentSize;
		mCurrentSize += size;
		return current;
	}

	void Stack::Push()
	{
		mCurrentSize = RoundToM(mCurrentSize, 4);
		mTail+=mCurrentSize;
		assert((mTail + mCurrentSize + sizeof(unsigned int)) < mEnd);
		unsigned int *frameSize = (unsigned int*)mTail;
		*frameSize = mCurrentSize;
		mTail+=sizeof(unsigned int);
		mCurrentSize = 0;
	}
	
	void Stack::Pop()
	{
		assert(mTail > mHead);
		if (mCurrentSize > 0)
		{
			::memset(mTail, 0, mCurrentSize);
		}
		mTail-=sizeof(unsigned int);
		mCurrentSize = *((unsigned int *)mTail);
		*mTail = 0;
		mTail-=mCurrentSize;
		//GC_gcollect();
	}
	
///////////////////////////
	
	static void Finalization_proc(void * inObj, void *client_data)
	{
		assert((client_data == (void*)0xfefe) || (client_data == (void*)0xefef));
		if (client_data == (void*)0xfefe)
		{
			Object *obj = (Object*)inObj;
			obj->DoFinalizeChain();
		}
		else if (client_data == (void*)0xefef)
		{
			Object *obj = (Object*)inObj;
			obj->DoFinalizeChain();
		}
	}

	void *Object::operator new(size_t sz)
	{
		void *ptr = GC_MALLOC(sz);
		GC_REGISTER_FINALIZER_IGNORE_SELF(ptr, Finalization_proc, (void*)0xfefe, 0, 0);
		return ptr;
	}
	
	void *DataObject::operator new(size_t sz)
	{
		void *ptr = GC_MALLOC_ATOMIC(sz);
		GC_REGISTER_FINALIZER_IGNORE_SELF(ptr, Finalization_proc, (void*)0xfefe, 0, 0);
		return ptr;
	}
	
///////////////////////////
	
	__thread Stack *gShadowStack = 0;
	__thread int gShadowStackDepth = 0;
	static unsigned int gDefaultShadowStackSize = 1024 * 1024;
	
	void SetShadowStackSize(unsigned int newSize)
	{
		assert(!gShadowStack);
		gDefaultShadowStackSize = newSize;
	}

	unsigned int GetShadowStackSize()
	{
		return gDefaultShadowStackSize;
	}
	
	int ShadowStackPush()
	{
		if (!gShadowStack)
		{
			gShadowStack = new Stack(gDefaultShadowStackSize);
		}
		
		gShadowStack->Push();
		
		int temp = gShadowStackDepth;
		++gShadowStackDepth;
		return temp;
	}
	
	void ShadowStackPop(int prevDepth)
	{
		assert(gShadowStack);
		--gShadowStackDepth;
		assert(gShadowStackDepth == prevDepth);
		gShadowStack->Pop();
	}
	
	void *ShadowStackAlloc(unsigned int size, unsigned int alignment)
	{
		if (!gShadowStack)
		{
			gShadowStack = new Stack(gDefaultShadowStackSize);
		}
		
		return gShadowStack->Alloc(size, alignment);
	}
	
	void ShadowStackShutdown()
	{
		assert(gShadowStackDepth == 0);
		delete gShadowStack;
		gShadowStack = 0;
	}
	
}
