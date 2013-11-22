//
//  main.mm
//  TrashManTester
//
//  Created by Nathan Rausch on 10/22/13.
//  Copyright (c) 2013 Nathan Rausch. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <assert.h>

#include "../inc/TrashMan.h"


int gNumObjs[3] = { 0, 0, 0 };

BEGIN_TRASH_DATA_CLASS(Trash)
private:
	char mBlah[1024];
public:
	Trash()
	{
		++gNumObjs[0];
		memset(mBlah, 0, 1024);
	}

	virtual bool Finalize()
	{
		--gNumObjs[0];
		return true;
	}
END_TRASH_DATA_CLASS

BEGIN_TRASH_CLASS(TrashBag)
	Trash *mTrash[1024];
public:
	TrashBag()
	{
		++gNumObjs[1];
		for (int i = 0; i < 1024; ++i)
		{
			mTrash[i] = new Trash();
		}
	}

	virtual bool Finalize()
	{
		--gNumObjs[1];
		::memset(&mTrash, 0, sizeof(Trash*) * 1024);
		return true;
	}
END_TRASH_CLASS

BEGIN_TRASH_CLASS(TrashWorld)
public:
	char blah[1024 * 1024];

	TrashWorld()
	{
		++gNumObjs[2];
	}

	virtual bool Finalize()
	{
		--gNumObjs[2];
		return true;
	}

	void Execute()
	{
		for (int i = 0; i < 2048 * 1; ++i)
		{
			SSTK_PUSH();
			TrashBag **test = SSTK_ALLOC(TrashBag*);
			*test = new TrashBag;
			*test = 0;
			SSTK_POP();
		}
	}
END_TRASH_CLASS

void Run()
{
	SSTK_PUSH();
	TrashWorld **tw = SSTK_ALLOC(TrashWorld*);
	*tw = new TrashWorld;
	(*tw)->Execute();
	(*tw) = 0;
	SSTK_POP();
}

int main(int argc, const char * argv[])
{
	TrashMan::Init();
	SSTK_INIT(1024 * 1024 * 8);
	SSTK_PUSH();
	Run();
	SSTK_POP();
	SSTK_SHUTDOWN();
	TrashMan::Collect();
	TrashMan::Destroy();
	
	assert(gNumObjs[0] == gNumObjs[1] == gNumObjs[2] == 0);
	return 0;
}
