#include "common.h"
#include "timer.h"
#include "message.h"
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
using namespace std;

struct itimerval itv;
struct timeval tv = {.tv_sec = 0, .tv_usec = 0};

void SetTimer(
	int interval_sec, 
    int interval_usec,
	int delay_sec, 
    int delay_usec)
{
    signal(SIGALRM, SigHandle);
    //how long to run the first time
    //can not be 0 0
    itv.it_value.tv_sec = delay_sec;
    itv.it_value.tv_usec = delay_usec;
    //how long to run the next time
    itv.it_interval.tv_sec = interval_sec;
    itv.it_interval.tv_usec = interval_usec;

    if(setitimer(ITIMER_REAL,&itv,NULL) != 0){
    	cout << "Setitimer failed." << endl;
    }
}

void SigHandle(int sig)
{
    tv.tv_usec += TIME_SCALE * itv.it_interval.tv_usec;
    tv.tv_sec += TIME_SCALE * itv.it_interval.tv_sec;
    cout << "Simulation time:" << GetTime() << " s." << endl;
    
    pool.addTask(new TimerTask(new Message(
                    NULL,MSG_TIME_Timer)));
}

int GetTime()
{
    return tv.tv_sec + tv.tv_usec/1000;
}

void StartTimer()
{
    SetTimer(1,0,1,0);
}

void EndTimer()
{
    SetTimer(0,0,0,0);
    pool.wait();
}