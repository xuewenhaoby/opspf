#include "common.h"
#include "topo.h"

SatNode sats[SAT_NUM];
HostNode users[HOST_NUM];
StaticTopo G;
ThreadPool pool;

ThreadPool route_pool;
ThreadPool link_route_pool;
ISDB Int_DB[SAT_NUM];
string sys_stat;

