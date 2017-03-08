#ifndef HJOB_SYSTEM
#define HJOB_SYSTEM


//	Two ways to write a job system: 
//	1:
//		a depends on x & y
//  2: 
//		x permits a
//		y permits a

// expanded on here : http://cbloomrants.blogspot.se/2011/12/12-03-11-worker-thread-system-with.html

// this uses the second alterntive
// for now everything is linked lists, that seems fine to me.

// Concurrent linked queue things:

/*
Assumes 
struct Node
{
	Node *next;
	.. 
	stuff
	..
};
*/
void *next(void *node)
{
	return *(void **)node;
}
// thread unsafe logic
void push(void **first, void **trailing, void *node)
{
	if (!*first)
	{
		*first = node;
		*trailing = node;
	}
	else
	{
		next(*trailing) = node;
		trailing = node;
	}
}

void *pop(void **first, void **trailing)
{
	if (*first == *trailing)
	{
		*first = 0;
		*trailing = 0;
		return 0;
	}
	else
	{
		void *ret = *first;
		*first = next(first);
		return ret;
	}
}




#include "stdint.h"
#define NUM_THREADS 2


void semaphore_post();

struct TYPE;
struct Node
{
	Node *next;
	TYPE *task;
};

struct TYPE
{
	union
	{
		uint64_t num_dependencies; // if waiting for something
		void *next_rtr; // if in ready to run state
	};
	void(*work)();
	Node *head_permits;
	bool is_done;
	bool can_be_released;
};

TYPE *create_task(void (*work)(), TYPE *dependencies, int num_dependencies)
{
	TYPE *task = (TYPE *)malloc(sizeof(TYPE));
	*task = {};
	task->work = work;
	task->num_dependencies = num_dependencies;
	for (int i = 0; i < num_dependencies; i++)
	{
		if (dependencies[i].is_done)
		{
			int deps = InterlockedDecrement(task->num_dependencies);
			if (!deps)
			{

			}
		}
		else
		{
			Node node = (Node *)malloc(Node);
			node.next = dependencies[i].head_permits;
			dependencies[i].head_permits = node;
		}
	}
}




void release_task(TYPE *task)
{
	if (task->is_done) free(task);
	else task->can_be_released = true;
}

static TYPE *ReadyToRun;


void push_queue(TYPE task)
{

}

TYPE pop_queue()
{

}
void do_task(TYPE task);


#ifdef _WIN32
#include "windows.h"
static HANDLE semaphore = CreateSemaphore(0, 0, 10000, 0);

void semaphore_post()
{
	ReleaseSemaphore(semaphore, 1, 0);
}

void worker_loop()
{
	WaitForSingleObject(semaphore, INFINITE);
	do_task(pop_queue());
}


#endif

#endif