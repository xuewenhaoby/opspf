#ifndef __TASK_H__
#define __TASK_H__

void *func1(void *);

void *HandleNodeEvent(void *arg);
void *HandleTimerEvent(void *arg);
//xry_add
void *thread_route(void *argument_route);
void *thread_link_route(void *argument_link_route);

//

class Task{
public:
	Task() = default;
	Task(void *data):taskData(data){}
	virtual int Run() = 0;
	void setData(void *data){taskData = data;}

protected:
	void *taskData;
};

class NodeTask:public Task{
public:
	NodeTask(void *data):Task(data){}
	int Run(){
		HandleNodeEvent(taskData);
		return 0;
	}
};

class TimerTask:public Task{
public:
	TimerTask(void *data):Task(data){}
	int Run(){
		HandleTimerEvent(taskData);
		return 0;
	}
};

//xry_add
class RouteTask:public Task{
public:
    RouteTask(void *data):Task(data){}
	int Run(){
        thread_route(taskData);
		return 0;
	}
};
class LinkRouteTask:public Task{
public:
    LinkRouteTask(void *data):Task(data){}
	int Run(){
		thread_link_route(taskData);
		return 0;
	}
};
//
#endif