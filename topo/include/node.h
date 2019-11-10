#ifndef __NODE_H__
#define __NODE_H__

#include <iostream>
#include <cstdlib>
#include <vector>
#include "topo.h"
#include "route.h"
using namespace std;

#define SET(a,_a) a = _a
#define GET(a) return a
#define ACQ(a) return &a
#define RANDOM(a,b) rand()/((double)RAND_MAX/(b-a))+a 
struct arg_route
{
    int src_id;
    int gw_id;
    int dst_id;
    int src_portid;
    int gw_portid;
    int linkid;
};

class Node{
public:
	Node(){}
	~Node(){}
	// Node(int _id,NodePos _pos){SET(id,_id);SET(pos,_pos);}

	int getId(){GET(id);}
	Position getPos(){GET(pos);}

	void setId(int _id){SET(id,_id);}
	void setPos(NodePos _pos){SET(pos,_pos);}
	void setName(string _name){SET(name,_name);}
	void setBrName(string _br_name){SET(br_name,_br_name);}

	virtual void nodeInitialize() = 0;
	virtual void nodeFinalize() = 0;
	virtual void linkInitialize() = 0;
	virtual void linkFinalize() = 0;
	virtual void updateLink() = 0;

protected:
	int id;
	NodePos pos;
	string name;
	string br_name;
};

class SatNode:public Node{
public:
	SatNode();
	// SatNode(int _id,NodePos _pos);

	void nodeInitialize();
	void nodeFinalize();
	void linkInitialize();
	void linkFinalize(){}
	void updateLink();

	void updatePos();
	void setInfData(InfData data,int i){SET(infData[i],data);}
	InfData* acqInfData(int i){ACQ(infData[i]);}
	// xwh added
    int getOrbitId(int id);
    int getOrbitIndex(int id); 
    int getForeSatelliteId(int id);
    int getRearSatelliteId(int id);
 	int getSideSatelliteId(int id,bool isEast,bool isNorth);
    //end xwh
	void Routing_table(int src_id,StaticTopo topo,ISDB Int_DB[]);
    bool getInfStat(int infId);
private:
	InfData infData[6];
    unsigned int sup_array[SAT_NUM][SAT_NUM];
};

class HostNode:public Node{
public:
	HostNode();
	// HostNode(int _id,NodePos _pos);

	void nodeInitialize();
	void nodeFinalize();
	void linkInitialize();
	void linkFinalize();
	void updateLink();
private:
	LinkState link_stat[SAT_NUM];
	vector<LinkState> link_use;
	vector<LinkState> link_old;
};

bool Compare(LinkState a, LinkState b);

#endif