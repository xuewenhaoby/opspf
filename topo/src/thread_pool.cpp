#include "thread_pool.h"

vector<Task*> ThreadPool::taskList;
int ThreadPool::busyNum = 0;
bool ThreadPool::shutdown = false;
pthread_mutex_t ThreadPool::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::cond = PTHREAD_COND_INITIALIZER;

void *ThreadPool::ThreadFunc(void *arg)
{
	//pthread_t tid = pthread_self();
	
	while(1){
		pthread_mutex_lock(&mutex);
		while(taskList.size() == 0 && !shutdown){
			pthread_cond_wait(&cond,&mutex);
		}

		if(shutdown){
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
		}

		vector<Task*>::iterator it = taskList.begin();
		Task *task = *it;
		if(it != taskList.end()){
			task = *it;
			taskList.erase(it);
		}
		busyNum++;
		pthread_mutex_unlock(&mutex);

		task->Run();

		pthread_mutex_lock(&mutex);
		busyNum--;
		pthread_mutex_unlock(&mutex);
	}

	return (void*)0;
}

void ThreadPool::create()
{
	threadArray = new pthread_t[threadNum];
	for(int i = 0; i < threadNum; i++){
		pthread_create(&threadArray[i],NULL,ThreadFunc,NULL);
	}
}

ThreadPool::ThreadPool(int threadNum)
{
	this->threadNum = threadNum;
	create();
}

void ThreadPool::addTask(Task *task)
{
	pthread_mutex_lock(&mutex);
	taskList.push_back(task);
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
}

void ThreadPool::stopAll()
{
	// Avoid that the func be called repeatly.
	if(shutdown) return;

	shutdown = true;
	pthread_cond_broadcast(&cond);

	for(int i = 0; i < threadNum; i++){
		pthread_join(threadArray[i],NULL);
	}

	delete[] threadArray;
	threadArray = NULL;

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}
