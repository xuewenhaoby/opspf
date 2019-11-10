#ifndef __TOPO_H__
#define __TOPO_H__

#include <iostream>
using namespace std;

#define HOST_NUM 1

#define TOPO_M 6 // number of orbit
#define TOPO_N 8 // number of satellite on one orbit
#define SAT_NUM TOPO_M * TOPO_N
#define SLINK_NUM 128
#define SINF_NUM 6
#define MAX_ARC_WEIGHT 1000

#define TOPO_BETA 70

#define LINKID_SELFLOOP 0
#define LINKID_NOLINK -1

#define SAT_NAME "sat"
#define HOST_NAME "host"
#define SAT_BR_NAME "sbr"
#define HOST_BR_NAME "hbr"

#define MOBILITY_FILE "topofile/orbit.bin"
//The size of mobility data : 24*60*60+1 24 hours data per second
#define DATA_SIZE 86401 
#define ISL_FILE "topofile/linktable.txt"

#define PI 3.14159265358979323846264338328
#define EARTH_RADIUS 6378.137 // km
#define COVER_RADIUS 2500.0 // km

#define RADIAN(m) (m * PI / 180.0)

typedef int NodeAddress;


struct IslNode
{
	int nodeId;
	int inf;
	NodeAddress ip;
};

struct Isl
{
	int linkId;
	NodeAddress subnetIp;
	unsigned int weight;

	IslNode endpoint[2];
};

struct ArcNode
{
	int linkId;
	unsigned int weight;
};

struct StaticTopo
{
	Isl isl[SLINK_NUM];
	ArcNode arcs[SAT_NUM][SAT_NUM]; // weight;
};

struct LinkState
{
	int sateId;
	double distance;
	bool isCovered;
	bool operator==(const LinkState &b){
		return this->sateId == b.sateId 
		&& this->isCovered == b.isCovered;
	}
};

struct Coord{
	double lat;
	double lon;
};

typedef struct Position{
	Coord loc;
	bool isNorth;
}NodePos;

typedef struct InterfaceData{
	int linkId;
	bool stat;
}InfData;

//xry_add
typedef struct InterfaceDatabase{
	int linkId[SINF_NUM];
	bool port_stat[SINF_NUM];
	int dst_portid[SINF_NUM];
	int dst_satid[SINF_NUM];
}ISDB;
//

void TopoInitialize();
void TopoFinalize();
void topo_test();

void InitStaticTopo();

void ReadIslFile(string fileName);

NodePos ReadLocFile(int id,int time);
NodePos RandomPos();

double CalculateDistance(Coord x,Coord y); 

#endif