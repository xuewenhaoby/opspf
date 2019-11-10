#ifndef __COMMON_H__
#define __COMMOM_H__

#include "node.h"
#include "topo.h"
#include "thread_pool.h"

#define ENABLE_PHYSICAL true
#define BUF_STR_SIZE 100

extern SatNode sats[SAT_NUM];
extern HostNode users[HOST_NUM];
extern StaticTopo G;
extern ThreadPool pool;

extern ThreadPool route_pool;
extern ThreadPool link_route_pool;
extern ISDB Int_DB[SAT_NUM];
extern string sys_stat;


#endif