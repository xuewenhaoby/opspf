#ifndef __POOL_H__
#define __POOL_H__

#include "task.h"
#include <vector>
#include <pthread.h>
using namespace std;

#define POOL_SIZE 20

class ThreadPool{
private:
	static vector<Task*> taskList;
	static int busyNum;
	static pthread_mutex_t mutex;
	static pthread_cond_t cond;
	static bool shutdown;

	int threadNum;
	pthread_t * threadArray;

protected:
	static void *ThreadFunc(void *arg);
	void create();

public:
	ThreadPool(){ThreadPool(POOL_SIZE);}
	ThreadPool(int _tNum);
	void addTask(Task *task);
	void stopAll();
	int getTaskNum(){return taskList.size();}
	void wait(){while(busyNum > 0);}
};

#endif