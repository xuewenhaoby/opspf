#include "common.h"
#include "route.h"
#include "cmd.h"
#include "timer.h"
#include "topo.h"
#include "task.h"

#include <iostream>
#include <algorithm>
using namespace std;

/******************************class SatNode*****************************/
SatNode::SatNode()
{
	name = SAT_NAME;
	br_name = SAT_BR_NAME;
}

void SatNode::nodeInitialize()
{
	if(ENABLE_PHYSICAL){
		int addr = id * 256*256*256 + 1;
		add_node_namespace(name.c_str(),br_name.c_str(),id,addr,8);
		//print_VethIp(name.c_str(),id,6);
	}
	setPos(ReadLocFile(id,0));
}

void SatNode::nodeFinalize()
{
	if(ENABLE_PHYSICAL){
		del_node_namespace(name.c_str(),br_name.c_str(),id);
	}
}

void SatNode::linkInitialize()
{
	if(ENABLE_PHYSICAL){
		for(int i = 0; i < SINF_NUM; i++){
			int linkId = infData[i].linkId;
			Int_DB[id-1].linkId[i]=infData[i].linkId;
			IslNode *nodes = G.isl[linkId-1].endpoint;
			if(linkId != -1 && id == nodes[0].nodeId){
				int nIds[2] = {id, nodes[1].nodeId};
				int pIds[2] = {i, nodes[1].inf};
				int addrs[2] = {nodes[0].ip,nodes[1].ip};
				//add_linkId
				add_satsat_link(name.c_str(),nIds,pIds,addrs,24,linkId);
			}
			//Int_DB initial 
		    if(linkId!=-1)
		    {
			    if(G.isl[infData[i].linkId-1].endpoint[0].nodeId==id)
			    {    
		            Int_DB[id-1].dst_portid[i]=G.isl[infData[i].linkId-1].endpoint[1].inf;
				    Int_DB[id-1].dst_satid[i]=G.isl[infData[i].linkId-1].endpoint[1].nodeId;
			    }
			    else
			    {
			    Int_DB[id-1].dst_portid[i]=G.isl[infData[i].linkId-1].endpoint[0].inf;
				Int_DB[id-1].dst_satid[i]=G.isl[infData[i].linkId-1].endpoint[0].nodeId;
			    }
		    }
		}
	}
}

void SatNode::updateLink()
{
	//todo: check infs if avalible
	for(int i = 0; i < SINF_NUM; i++){
		bool curStat = getInfStat(i);
		if(infData[i].stat != curStat)
		{
			infData[i].stat = curStat;

			string buf = curStat ? " [UP] " : " [DOWN] ";
			cout << buf << id << " inf:" << i << endl;
			if(ENABLE_PHYSICAL){
				change_LinkState(name.c_str(),id,i,curStat);
			}
		}
		//int_DB_update
		Int_DB[id-1].port_stat[i]=curStat;
	}
	// if(GetTime()>10 && id==1)
	// {
	// Routing_table(id,G,Int_DB);
	// }
	Routing_table(id,G,Int_DB);
}
//
void SatNode::Routing_table(
    int src_id,
    StaticTopo topo,
    ISDB Int_DB[]
)
{
    int path[SAT_NUM],next_hop[SAT_NUM];
    unsigned int distance[SAT_NUM];
    //variable for route_write
    int src_portid,dst_id,gw_portid,linkid,gw_id;
    //support array_weight_update
    for(int i=0;i<SAT_NUM;i++)
    {
        for(int j=0;j<SAT_NUM;j++)
        {
			int temp_link = topo.arcs[i][j].linkId;
			int k =0;
			if(temp_link!=-1&&temp_link!=0)
			{
			if(topo.isl[temp_link-1].endpoint[0].nodeId==i+1)
			    k=topo.isl[temp_link-1].endpoint[0].inf;

			else
			    k=topo.isl[temp_link-1].endpoint[1].inf;
            if(Int_DB[i].port_stat[k]==1)
            sup_array[i][j]=topo.arcs[i][j].weight;
			else 
			sup_array[i][j]=MAX_ARC_WEIGHT;
			}
			else
			sup_array[i][j]=MAX_ARC_WEIGHT;       
        }
    }

    Dijkstra(sup_array,src_id,distance,path);

    for(int i=0;i<SAT_NUM;i++)
    {
		//find next_hop
		next_hop[i]=FindPreNode(path,src_id,i+1);
        //thread
		struct arg_route args[SAT_NUM];

        if(i!=src_id-1 && next_hop[i]!=-1)
        {
            int temp=next_hop[i];
            gw_id=temp+1;
            dst_id=i+1;
            linkid=topo.arcs[src_id-1][temp].linkId;
            if(linkid != -1)
            {
                if(topo.isl[linkid-1].endpoint[0].nodeId==src_id)
                {
                     src_portid=topo.isl[linkid-1].endpoint[0].inf;
                     gw_portid=topo.isl[linkid-1].endpoint[1].inf;
                }
                else
                {
                     src_portid=topo.isl[linkid-1].endpoint[1].inf;
                     gw_portid=topo.isl[linkid-1].endpoint[0].inf;
                }
				args[i].src_id=src_id;
				args[i].gw_id=gw_id;
				args[i].dst_id=dst_id;
				args[i].src_portid=src_portid;
				args[i].gw_portid=gw_portid;
				args[i].linkid= linkid;
				RouteTask *rt=new RouteTask((void*)&args[i]);
				route_pool.addTask(rt);
				LinkRouteTask *lrt=new LinkRouteTask((void*)&args[i]);
				link_route_pool.addTask(lrt);
            }
        }
    }
}
//

bool SatNode::getInfStat(int infId)
{
	int srcNodeId = id;
	int dstNodeId = -1;
	Isl isl = G.isl[ infData[infId].linkId-1 ];
	for(int i = 0; i < 2; i++)
	{
		if(srcNodeId == isl.endpoint[i].nodeId)
		{
			dstNodeId = isl.endpoint[(i+1)%2].nodeId;
			break;
		}
	}

	if(dstNodeId == getForeSatelliteId(srcNodeId) ||
		dstNodeId == getRearSatelliteId(srcNodeId))
	{
		return true;
	}
	else if(dstNodeId == getSideSatelliteId(srcNodeId, true, pos.isNorth) ||
		dstNodeId == getSideSatelliteId(srcNodeId, false, pos.isNorth)) 
	{
		return (abs(pos.loc.lat) < TOPO_BETA) && 
				(abs(sats[dstNodeId-1].getPos().loc.lat) < TOPO_BETA);
	}
	else
	{
		return false;
	}
}

void SatNode::updatePos()
{
	setPos(ReadLocFile(id,GetTime()));
}

int SatNode::getOrbitId(int id)
{
	return (id-1)/8 + 1;
}


int SatNode::getOrbitIndex(int id)
{
	return (id-1)%8 + 1;
}

int SatNode::getForeSatelliteId(int id)
{
	int orbitId = getOrbitId(id);
	int index = getOrbitIndex(id);
	return (orbitId - 1)*8 + index % 8 + 1;
}

int SatNode::getRearSatelliteId(int id)
{
	int orbitId = getOrbitId(id);
	int index = getOrbitIndex(id);
	
	return (orbitId - 1)*8 + (index + 6)%8 + 1;
}

int SatNode::getSideSatelliteId(int id,bool isEast,bool isNorth)
{
	int orbitId = getOrbitId(id);
	int index = getOrbitIndex(id);
	int v = -1;

	if(orbitId == 1){
		if(isEast){
			if(isNorth)
				v = orbitId*8 + (index + 6)% 8;
			else
				v = orbitId*8 + index-1;
		}
	}else if(orbitId == 6){
		if(!isEast){
			if(isNorth)
				v = (orbitId - 2)*8 + index % 8;
			else
				v = (orbitId - 2)*8 + index-1;			
		}
	}else{
		if(isEast){
			if(isNorth)
				v = orbitId*8 + (index + 6)% 8;
			else
				v = orbitId*8 + index-1;
		}else{
			if(isNorth)
				v = (orbitId - 2)*8 + index % 8;
			else
				v = (orbitId - 2)*8 + index-1;	
		}
	}

	return v + 1;
}

/******************************class HostNode*****************************/
HostNode::HostNode()
{
	name = HOST_NAME;
	br_name = HOST_BR_NAME;
}

void HostNode::nodeInitialize()
{
	if(ENABLE_PHYSICAL){
		srand((unsigned)time(NULL));
		int addr = (rand()%48+1)*256*256*256 + id + 1;
		add_node_namespace(name.c_str(),br_name.c_str(),id,addr,8);
	}
	// initialize position
	setPos(RandomPos());
	// Coord loc = {80,115};
	// NodePos pos = {loc,false};
	// setPos(pos);
}

void HostNode::nodeFinalize()
{
	if(ENABLE_PHYSICAL){
		del_node_namespace(name.c_str(),br_name.c_str(),id);
	}
}

void HostNode::linkInitialize()
{
	if(ENABLE_PHYSICAL){
		for(int i = 0; i < SAT_NUM; i++){
			add_sathost_link(SAT_BR_NAME,i+1,id,br_name.c_str(),id,i+1);
			// set dev of sat up
			change_LinkState(SAT_BR_NAME,i+1,id,true);
		}
	}	
}

void HostNode::linkFinalize()
{
	if(ENABLE_PHYSICAL){
		for(int i = 0; i < SAT_NUM; i++){
			delLink_BrBr(br_name.c_str(),id,i+1);
		}
	}
}

void HostNode::updateLink()
{
	link_old = link_use;
	link_use.clear();

	for(int i = 0; i < SAT_NUM; i++){
		link_stat[i].sateId = i+1;
		double distance = CalculateDistance(pos.loc, sats[i].getPos().loc);
		link_stat[i].distance = distance;
		link_stat[i].isCovered = distance < COVER_RADIUS;

		if(link_stat[i].isCovered){
			link_use.push_back(link_stat[i]);
			// set dev up if down and curentelly used
			vector<LinkState>::iterator it;
			it = find(link_old.begin(),link_old.end(),link_stat[i]);
			if(it == link_old.end()){
				cout << " [UP] user " << id << " link:" << link_stat[i].sateId << endl;
				if(ENABLE_PHYSICAL){
					change_LinkState(br_name.c_str(),id,link_stat[i].sateId,true);
				}
			}
		}
	}
	sort(link_use.begin(),link_use.end(),Compare);

	// set dev down if up and not currently used
	vector<LinkState>::iterator it;
	for(it = link_old.begin(); it != link_old.end();it++){
		vector<LinkState>::iterator it2;
		it2 = find(link_use.begin(),link_use.end(),*it);
		if(it2 == link_use.end()){
			cout << " [DOWN] user " << id << " link:" << it->sateId << endl;
			if(ENABLE_PHYSICAL){
				change_LinkState(br_name.c_str(),id,it->sateId,false);
			}
		}
	}
}

/******************************API Funcs***********************************/
bool Compare(LinkState a, LinkState b)
{
	return a.distance < b.distance;
}
