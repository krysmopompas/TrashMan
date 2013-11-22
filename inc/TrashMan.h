/*

 TrashMan - conservative GC for C++ for use in a hybrid environment where other memory managment schemes are in use

*/

#ifndef TRASHMAN_H
#define TRASHMAN_H

#include <assert.h>

namespace TrashMan
{
	
	extern void Init();
	extern void Destroy();
	extern void Collect();
	
	// Self is GC, interior is GC
	extern void *Alloc(size_t s);
	
	// Self is GC, interior is ignored
	extern void *AllocData(size_t s);
	
	#define SSTK_INIT(SIZE) \
		::TrashMan::SetShadowStackSize((unsigned int)SIZE) \
		/**/
		
	#define SSTK_PUSH() \
		int protected_##__FUNCTION__##_stackIndex = ::TrashMan::ShadowStackPush() \
		/**/

	#define SSTK_POP() \
		::TrashMan::ShadowStackPop(protected_##__FUNCTION__##_stackIndex); \
		protected_##__FUNCTION__##_stackIndex = -1 \
		/**/
		
	#define SSTK_ALLOC(CLASSNAME) \
		(CLASSNAME*)::TrashMan::ShadowStackAlloc(sizeof(CLASSNAME), 4) \
		/**/
	
	#define SSTK_ALLOC_ALIGNED(CLASSNAME, ALIGN) \
		(CLASSNAME*)::TrashMan::ShadowStackAlloc(sizeof(CLASSNAME), ALIGN) \
		/**/

	#define SSTK_SHUTDOWN() \
		::TrashMan::ShadowStackShutdown() \
		/**/

	// Required base class for all collectable objects
	class Object
	{
	protected:
		~Object() { }
	public:
		Object() { }
		
		void operator delete(void *p) { assert(0); }
		
		virtual bool Finalize() = 0;

		void *operator new(size_t sz);
		void *operator new[](size_t sz) { assert(0); return 0; }
		
		virtual bool DoFinalizeChain()
		{
			return Finalize();
		}
	};
	
	class DataObject : public Object
	{
	protected:
		DataObject() { }
	public:
		~DataObject() { }
		
		void operator delete(void *p) { assert(0); }
		
		virtual bool Finalize() = 0;
		
		void *operator new(size_t sz);
		void *operator new[](size_t sz) { assert(0); return 0; }
		
		virtual bool DoFinalizeChain()
		{
			return Finalize();
		}
	};

	extern void SetShadowStackSize(unsigned int newSize);
	extern unsigned int GetShadowStackSize();
	extern int ShadowStackPush();
	extern void ShadowStackPop(int prevDepth);
	extern void *ShadowStackAlloc(unsigned int size, unsigned int alignment = 4);
	extern void ShadowStackShutdown();
	
} // namespace TrashMan

#define BEGIN_TRASH_CLASS(X) \
class X : public ::TrashMan::Object \
{ \
protected:\
virtual bool DoFinalizeChain() \
{ \
return Finalize(); \
} \
protected: \
~X() { } \
/**/

#define DERIVE_TRASH_CLASS(X, PARENT) \
class X : public PARENT \
{ \
protected:\
virtual bool DoFinalizeChain() \
{ \
bool res = Finalize(); \
return (!res || !PARENT::Finalize()) ? false : true; \
} \
protected: \
~X() { } \
/**/

#define END_TRASH_CLASS \
}; \
/**/

#define BEGIN_TRASH_DATA_CLASS(X) \
class X : public ::TrashMan::DataObject \
{ \
protected:\
virtual bool DoFinalizeChain() \
{ \
return Finalize(); \
} \
protected: \
~X() { } \
/**/

#define DERIVE_TRASH_DATA_CLASS(X, PARENT) \
class X : public PARENT \
{ \
protected:\
virtual bool DoFinalizeChain() \
{ \
bool res = Finalize(); \
return (!res || !PARENT::Finalize()) ? false : true; \
} \
protected: \
~X() { } \
/**/

#define END_TRASH_DATA_CLASS \
}; \
/**/

// Private

#endif // TRASHMAN_H
