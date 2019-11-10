#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <cmath>
#include <windows.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "network.h"

#include "routing_dss.h"

//-----------------------
// Dss Core Function
//-----------------------
void InitStaticTopo(
	Node* node,
	DssData* dss)
{
	ReadISLFile(dss);

	for(int i = 0; i < TotalNodeNum; i++)
	{
		for(int j = 0; j < TotalNodeNum; j++)
		{
			if(i == j)
			{
				dss->G.arcs[i][j].weight = 0;
				dss->G.arcs[i][j].linkId = DSS_LINKID_SELFLOOP;
			}
			else
			{
				dss->G.arcs[i][j].weight = MAX_ARC_WEIGHT;
				dss->G.arcs[i][j].linkId = DSS_LINKID_NOLINK;
			}
		}
	}

	for(int i = 0; i < TotalLinkNum; i++)
	{
		DssIsl* isl = & dss->G.isl[i];

		int v[2] = {isl->endpoint[0].nodeId-1, isl->endpoint[1].nodeId-1};

		for(int j = 0; j < 2; j++)
		{
			dss->G.arcs[ v[j] ][ v[(j+1)%2] ].linkId = isl->linkId;
			dss->G.arcs[ v[j] ][ v[(j+1)%2] ].weight = isl->weight;
		}

		for(int j = 0; j < 2; j++)
		{
			if(GetNodeId(node) == v[j]+1)
			{
				int inf = isl->endpoint[j].inf;
				dss->infData[inf].linkId = isl->linkId;
				break;
			}
		}
	}
}

void InitTopoSnapshot(
	Node* node,
	DssData* dss)
{
	ifstream tin(CHANGE_TIME_FILE);
	ifstream fin(MOBILITY_FILE, ios_base::binary);

	string timeStr;
	int t1 = 0, t2;
	DssCoord satellites[TotalNodeNum],satellites_old[TotalNodeNum];

	int topoId = 1;
	while (getline(tin,timeStr))
	{
		dss->topoSnapshot[topoId].topoId = topoId;

		t2 = t1;
		t1 = atoi(timeStr.c_str())+1;
		for (int i = 0; i < TotalNodeNum; i++)
		{
			int startTime[2];
			startTime[0] = i * DATA_SIZE + (t1 - 1) % DATA_SIZE;
			startTime[1] = i * DATA_SIZE + t1 % DATA_SIZE;
			DssCoord location[2];
			for (int j = 0; j < 2; j++)
			{
				fin.seekg(sizeof(DssCoord) * startTime[j], ios_base::beg);
				fin.read((char *)(location + j), sizeof(DssCoord));
			}
			satellites[i] = location[1];
			satellites_old[i] = location[0];

			if(i+1 == GetNodeId(node))
			{
				dss->topoSnapshot[topoId].map.left.lat = location[1].lat;
				dss->topoSnapshot[topoId-1].map.right.lat = location[1].lat;

				bool isNorth = location[1].lat > location[0].lat;
				dss->topoSnapshot[topoId].map.leftIsNorth = isNorth;
				dss->topoSnapshot[topoId-1].map.rightIsNorth = isNorth;
			}
		}

		for (int i = 0; i < TotalLinkNum; i++)
		{
			dss->topoSnapshot[topoId].regularSign[i].linkId = dss->G.isl[i].linkId; // i + 1;
			dss->topoSnapshot[topoId].regularSign[i].sign = false;

			int v1 = dss->G.isl[i].endpoint[0].nodeId-1;
			int v2 = dss->G.isl[i].endpoint[1].nodeId-1;
			int orbitIdA = GetOrbitId(v1+1);
			int orbitIdB = GetOrbitId(v2+1);

			if (orbitIdA != orbitIdB)
			{
				if (abs(satellites[v1].lat) > DSS_BETA || abs(satellites[v2].lat) > DSS_BETA) // IsHigh
				{
					dss->topoSnapshot[topoId].regularSign[i].sign = true;
				}
				else //IsLow
				{
					bool isEast = orbitIdA < orbitIdB;
					bool isNorth = satellites[v1].lat > satellites_old[v1].lat;
					if( v2 != GetSideSatelliteId(v1+1, isEast, isNorth)-1 )
					{
						dss->topoSnapshot[topoId].regularSign[i].sign = true;
					}
				}
			}
		}

		topoId++;
	}

	fin.close();
	tin.close();

	// TODO: Init routingSnapshot
	for(int i = 1; i <= TotalTopoSnapshotNum; i++)
	{
		DssRoutingSnapshot* rs = & dss->topoSnapshot[i].routingSnapshot;
		ReviseRoutingSnapshot(node, dss, rs);
	}
}

bool SatelliteUpdateCurrentSnapshot(
	Node* node,
	DssData* dss)
{
	UpdateCurrentLocationAndMotion(node,dss);

	int topoId = -1;
	for(int i = 1; i <= TotalTopoSnapshotNum; i++)
	{
		if(SatelliteIsInArea(
			dss->curLocation.lat,
			dss->isNorth,
			dss->topoSnapshot[i].map.left.lat,
			dss->topoSnapshot[i].map.leftIsNorth,
			dss->topoSnapshot[i].map.right.lat,
			dss->topoSnapshot[i].map.rightIsNorth))
		{
			topoId = dss->topoSnapshot[i].topoId; // i
			break;
		}
	}

	if(topoId == -1)
	{
		ERROR_ReportError("No Valid TopoSnapshot was found!");
	}

	if(dss->curTopoId != topoId)
	{
		dss->curTopoId = topoId;
		dss->revisedRS = dss->topoSnapshot[topoId].routingSnapshot;
		return true;
	}

	return false;
}

bool SatelliteIsInArea(
	double lat,
	bool isNorth,
	double leftLat,
	bool leftIsNorth,
	double rightLat,
	bool rightIsNorth)
{
	if(leftIsNorth && rightIsNorth && isNorth)
	{
		return (lat >= leftLat && lat < rightLat);
	}
	
	if(leftIsNorth && !rightIsNorth)
	{
		return isNorth ? (lat >= leftLat) : (lat > rightLat);
	}
	
	if(!leftIsNorth && !rightIsNorth && !isNorth)
	{
		return (lat <= leftLat && lat > rightLat);
	}

	if(!leftIsNorth && rightIsNorth)
	{

		return !isNorth ? (lat <= leftLat) : (lat < rightLat);
	}

	return false;
}

void ReviseRoutingSnapshot(
	Node* node,
	DssData* dss,
	DssRoutingSnapshot* rs)
{
	DssArcNode arcs[TotalNodeNum][TotalNodeNum];
	memcpy(arcs, dss->G.arcs, sizeof(DssArcNode)*TotalNodeNum*TotalNodeNum);

	DssTopoSnapshot* topo = & dss->topoSnapshot[dss->curTopoId];
	for(int i = 0; i < TotalLinkNum; i++)
	{
		if(topo->regularSign[i].sign || dss->reviseSign[i].sign)
		{
			int v1 = dss->G.isl[i].endpoint[0].nodeId - 1;
			int v2 = dss->G.isl[i].endpoint[1].nodeId - 1;
			arcs[v1][v2].weight = MAX_ARC_WEIGHT;
			arcs[v2][v1].weight = MAX_ARC_WEIGHT;
		}
	}

	int v = GetNodeId(node) - 1;
	unsigned int distance[TotalNodeNum];
	int path[TotalNodeNum];
	
	Dijkstra(arcs, TotalNodeNum, v, distance, path);
	
	DssRoutingSnapshotRow* row = rs->routingRow;
	for(int i = 0; i < TotalNodeNum; i++)
	{
		row[i].targetNodeId = i + 1;

		int k = FindPreNode(path, v, i);
		if(k != -1)
		{
			row[i].nextHopNodeId = k + 1;
			row[i].nextLinkId = arcs[v][k].linkId;		
		}
		else
		{
			row[i].nextHopNodeId = -1;
			row[i].nextLinkId = -1;			
		}

	}
}

void DssRouterFunction(
	Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted)
{
	*packetWasRouted = FALSE;
}
// end DSS Core Function

//--------------------
// Networklayer API
//--------------------
void UpdateRoutingTable(Node* node)
{
	DssData* dss = (DssData *) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSS);

	NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_DSS);

	DssTopoSnapshot* topo = & dss->topoSnapshot[dss->curTopoId];
	int nodeId = GetNodeId(node);

	for(int i = 0; i < TotalLinkNum; i++)
	{
		int v[2], inf[2];
		NodeAddress addr[2];
		for(int k = 0; k < 2; k++)
		{
			v[k] = dss->G.isl[i].endpoint[k].nodeId - 1;
			inf[k] = dss->G.isl[i].endpoint[k].inf;
			addr[k] = dss->G.isl[i].endpoint[k].ip;
		}

		if(topo->regularSign[i].sign || dss->reviseSign[i].sign)
		{
			for(int j = 0; j < 2; j++)
			{
				if(nodeId != v[j]+1)
				{
					NodeAddress dstAddr = addr[j];
					NodeAddress netMask = 0xffffffff;
					NodeAddress nextHopAddr;
					int interfaceIndex;
					int cost;

					FindNextHopByTargetNode(node, dss, v[j], nextHopAddr, interfaceIndex, cost);

					if(interfaceIndex != -1)
					{
						NetworkUpdateForwardingTable(
							node,
							dstAddr,
							netMask,
							nextHopAddr,
							interfaceIndex,
							cost,
							ROUTING_PROTOCOL_DSS);
					}
				}
			}
		}
		else
		{
			NodeAddress dstAddr = dss->G.isl[i].subnetIp;
			NodeAddress netMask = 0xffffff00;
			NodeAddress nextHopAddr;
			int interfaceIndex;
			int cost;

			bool isMySubnet = false;
			for(int j = 0; j < 2; j++)
			{
				if(nodeId == v[j]+1)
				{
					nextHopAddr = addr[(j+1)%2];
					interfaceIndex = inf[j];
					cost = dss->G.arcs[ v[j] ][ v[(j+1)%2] ].weight;

					isMySubnet = true;
					break;					
				}
			}

			if(!isMySubnet)
			{
				int dstNode = (nodeId <= v[0]+1 || GetOrbitId(nodeId) == GetOrbitId(v[0]+1)) ? v[0] : v[1];
				FindNextHopByTargetNode(node, dss, dstNode, nextHopAddr, interfaceIndex, cost);				
			}

			if(interfaceIndex != -1)
			{
				NetworkUpdateForwardingTable(
					node,
					dstAddr,
					netMask,
					nextHopAddr,
					interfaceIndex,
					cost,
					ROUTING_PROTOCOL_DSS);		
			}			
		}
	}	
}

void FindNextHopByTargetNode(
	Node* node,
	DssData* dss,
	int dstNode,
	NodeAddress &nextHopAddr,
	int &interfaceIndex,
	int &cost)
{
	int nextHopNodeId = dss->revisedRS.routingRow[dstNode].nextHopNodeId;
	if(nextHopNodeId == -1)
	{
		interfaceIndex = -1;
		return;
	}

	int nextLinkId = dss->revisedRS.routingRow[dstNode].nextLinkId;

	cost = dss->G.arcs[GetNodeId(node)-1][nextHopNodeId-1].weight;

	DssIslNode* endpoint = dss->G.isl[nextLinkId-1].endpoint;
	for(int i = 0; i < 2; i++)
	{
		if(nextHopNodeId == endpoint[i].nodeId)
		{
			nextHopAddr = endpoint[i].ip;
			interfaceIndex = endpoint[(i+1)%2].inf;	
			break;	
		}
	}
}
// End NetworkLayer API

//------------------------
// EXata Process Function
//------------------------
void DssInit(
	Node* node,
	DssData** dssPtr,
	const NodeInput* nodeInput,
	int interfaceIndex,
	NetworkRoutingProtocolType dssProtocolType)
{
	// create the dss struct to use
	NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
	DssData* dss = (DssData *) MEM_malloc(sizeof(DssData));
	(*dssPtr) = dss;

	// Initial most data
	memset(dss, 0, sizeof(DssData));

	DssInitializeConfigurableParameters(
		node,
		nodeInput,
		dss,
		interfaceIndex);

	InitStaticTopo(node, dss);

	InitTopoSnapshot(node, dss);

	// Set the router function
	NetworkIpSetRouterFunction(
		node,
		&DssRouterFunction,
		interfaceIndex);

	if(dss->enableMapping)
	{
		NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_DEFAULT);

		DssSendNewEvent(node, MSG_DSS_LocationMapping, 0);
	}

	if(dss->enableDynamicRoute)
	{
		// Init dss->infData[]
		for(int i = 0; i < MaxInfNum; i++)
		{
			dss->infData[i].counter = dss->keepAliveTTL;
			dss->infData[i].linkChangeStat = DSS_NOCHANGE;
		}

		DssSendNewEvent(node, MSG_DSS_SendHello, 0);
	}
}

void DssInitializeConfigurableParameters(
	Node* node,
	const NodeInput* nodeInput,
	DssData* dss,
	int interfaceIndex)
{
	BOOL wasFound;
	char buf[MAX_STRING_LENGTH];
	UInt32 nodeId = GetNodeId(node);

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"DSS-ENABLE-MAPPING",
		&wasFound,
		buf);

	if (wasFound)
	{
        dss->enableMapping = (strcmp(buf, "YES") == 0) ? TRUE : FALSE;
	}
	else
	{
		dss->enableMapping = DSS_DEFAULT_ENABLE_MAPPING;
	}

	IO_ReadTime(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"DSS-MAPPING-INTERVAL",
		&wasFound,
		&dss->mappingInterval);

	if (!wasFound)
	{
		dss->mappingInterval = DSS_DEFAULT_MAPPING_INTERVAL;
	}

	IO_ReadString(
		node, 
		nodeId, 
		interfaceIndex, 
		nodeInput, 
		"DSS-ENABLE-DYNAMICROUTE", 
		&wasFound, 
		buf);

	if (wasFound)
	{
        dss->enableDynamicRoute = (strcmp(buf, "YES") == 0) ? TRUE : FALSE; 
	}
	else
	{
		dss->enableDynamicRoute  = DSS_DEFAULT_ENABLE_DYNAMICROUTE;
	}

    IO_ReadTime(
    	node, 
    	nodeId, 
    	interfaceIndex, 
    	nodeInput, 
    	"DSS-HELLO-INTERVAL", 
    	&wasFound, 
    	&dss->helloInterval);

	if(!wasFound)
	{ 
		dss->helloInterval = DSS_DEFAULT_HELLO_INTERVAL;
	}

    IO_ReadInt(
    	node, 
    	nodeId, 
    	interfaceIndex, 
    	nodeInput, 
    	"DSS-KEEPALIVE-TTL", 
    	&wasFound, 
    	&dss->keepAliveTTL);

	if(!wasFound)
	{ 
		dss->keepAliveTTL = DSS_DEFAULT_KEEPALIVE_TTL;
	}

	DssInitInterfaceData(node, nodeInput, interfaceIndex, dss);
}

void DssInitInterfaceData(
	Node* node,
	const NodeInput* nodeInput,
	int interfaceIndex,
	DssData* dss)
{
	BOOL wasFound;
	char buf[MAX_STRING_LENGTH];
	UInt32 nodeId = GetNodeId(node);

	for(int i = 0; i < MaxInfNum; i++)
	{
		dss->infData[i].isEnabled = DSS_DEFAULT_ENABLE_PORT;
		dss->infData[i].startTime = DSS_DEFAULT_PORT_START_TIME;
		dss->infData[i].endTime = DSS_DEFAULT_PORT_END_TIME;
	}

	IO_ReadString(
		node,
		nodeId,
		interfaceIndex,
		nodeInput,
		"DSS-ENABLE-PORT-STATUS-CHANGE",
		&wasFound,
		buf);

	if (wasFound && (strcmp(buf, "YES") == 0))
	{
		for(int i = 0; i < MaxInfNum; i++)
		{
			string forwardString = "DSS-PORT";
			string mediumString;
			char t[MAX_STRING_LENGTH];
			sprintf(t,"%d",i);
			string backwardString = t;

			forwardString += backwardString;
			IO_ReadString(
				node,
				nodeId,
				interfaceIndex,
				nodeInput,
				forwardString.c_str(),
				&wasFound,
				buf);

			dss->infData[i].isEnabled = wasFound ? (strcmp(buf, "YES") == 0) : DSS_DEFAULT_ENABLE_PORT;
			mediumString = (dss->infData[i].isEnabled) ? "ENABLE" : "DISABLE";

			forwardString = "DSS-" + mediumString + "-START-TIME-PORT" + backwardString;
		    IO_ReadTime(
				node,
				nodeId,
				interfaceIndex,
				nodeInput,
				forwardString.c_str(),
				&wasFound,
				&dss->infData[i].startTime);
			if (!wasFound)
			{
				dss->infData[i].startTime = DSS_DEFAULT_PORT_START_TIME;
			}

			forwardString = "DSS-" + mediumString + "-END-TIME-PORT" + backwardString;
		    IO_ReadTime(
				node,
				nodeId,
				interfaceIndex,
				nodeInput,
				forwardString.c_str(),
				&wasFound,
				&dss->infData[i].endTime);
			if (!wasFound)
			{
				dss->infData[i].endTime = DSS_DEFAULT_PORT_END_TIME;
			}				
		}     
    }
}

void DssSendNewEvent(
	Node* node,
	int eventType,
	clocktype delay)
{
	Message* newMsg = MESSAGE_Alloc(
		node,
		NETWORK_LAYER,
		ROUTING_PROTOCOL_DSS,
		eventType);

	MESSAGE_Send(node, newMsg, delay);
}

void DssHandleRoutingProtocolEvent(
	Node* node,
	Message* msg)
{
	DssData* dss = (DssData *) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSS);

	switch(MESSAGE_GetEvent(msg))
	{
		case MSG_DSS_LocationMapping:
		{
			if(SatelliteUpdateCurrentSnapshot(node, dss))
			{
				UpdateRoutingTable(node);
			}

			MESSAGE_Send(node, msg, dss->mappingInterval);
			break;
		}
		case MSG_DSS_SendHello:
		{
			DssSendHello(node, dss);

			MESSAGE_Send(node, msg, dss->helloInterval);
			break;
		}
		case MSG_DSS_SendLsu:
		{
			DssSendLsu(node, dss);

			MESSAGE_Free(node, msg);
			break;
		}

		default:
		{
			MESSAGE_Free(node, msg);
			break;
		}
	}
}

void DssSendHello(
	Node* node,
	DssData* dss)
{
	for(int i = 0; i < MaxInfNum; i++)
	{
		int linkId = dss->infData[i].linkId;
		DssLinkStatType status = GetLinkStatus(linkId, dss);

		if(status != DSS_LINK_UNREACHABLE)
		{
        	if(0 == dss->infData[i].counter--)
        	{
        		dss->reviseSign[linkId-1].sign = true;
				ReviseRoutingSnapshot(node, dss, &dss->revisedRS);
				UpdateRoutingTable(node);
        		// Send LSU packet from other prot, set flag and active the event.
        		dss->infData[i].linkChangeStat = DSS_LINKDOWN;
        		DssSendNewEvent(node, MSG_DSS_SendLsu, 0);
        	}
        	else
        	{
				DssSendProtocolPacket(
					node, 
					dss,
					i, 
					ANY_ADDRESS, 
					DSS_HELLO_PACKET);
        	}
		}
	}
}

void DssSendLsu(
	Node* node,
	DssData* dss)
{
	for(int i = 0; i < MaxInfNum; i++)
	{
		// find the ports whose info changed, then send the LSU info via all alive ports
		if(dss->infData[i].linkChangeStat != DSS_NOCHANGE)
		{
			// save the lsu data
			dss->lsuData.linkId = dss->infData[i].linkId;
			dss->lsuData.linkChangeStat = dss->infData[i].linkChangeStat;
			dss->lsuData.txTime = node->getNodeTime();
			
			// send the LSU info via all alive ports
			for(int j = 0; j < MaxInfNum; j++)
			{
				int linkId = dss->infData[j].linkId;
				if(GetLinkStatus(linkId, dss) == DSS_LINK_AVALIBLE)
				{
					DssSendProtocolPacket(
						node, 
						dss,
						j, 
						ANY_ADDRESS, 
						DSS_LSU_PACKET);
				}
				// Reset the change stat
				dss->infData[i].linkChangeStat = DSS_NOCHANGE;
			}
		}
	}
}

void DssSendProtocolPacket(
	Node* node,
	DssData* dss,
	int outInterface,
	NodeAddress sourceAddress,
	DssPacketType packetType)
{
	Message* newMsg = MESSAGE_Alloc(
		node,
		NETWORK_LAYER,
		ROUTING_PROTOCOL_DSS,
		MSG_DSS_SendHello); // The MSG_Type will be changed below.

	MESSAGE_PacketAlloc(node, newMsg, 512, TRACE_DSS);

	DssHeader* DssHdr = (DssHeader*) MESSAGE_ReturnPacket(newMsg);
	DssHdr->packetType = packetType;

	NodeAddress selfAddr = NetworkIpGetInterfaceAddress(node, outInterface);
	NodeAddress dstAddr = ANY_ADDRESS;

	switch(packetType)
	{
		case DSS_HELLO_PACKET:
		{
			MESSAGE_SetEvent(newMsg, (short) MSG_DSS_SendHello);
        	DssHelloInfo* helloInfo = (DssHelloInfo*) (MESSAGE_ReturnPacket(newMsg) + sizeof(DssHeader));

        	helloInfo->satelliteId = GetNodeId(node);
        	helloInfo->portId = outInterface;
			break;
		}
		case DSS_LSU_PACKET:
		{
			MESSAGE_SetEvent(newMsg, (short) MSG_DSS_SendLsu);
        	DssLsuInfo* lsuInfo = (DssLsuInfo*) (MESSAGE_ReturnPacket(newMsg) + sizeof(DssHeader));

        	*lsuInfo = dss->lsuData;

			break;
		}
		default:
		{
			printf("Unknown protocol packet type to send!\n");
			break;
		}
	}

	NetworkIpSendRawMessage(node, newMsg, selfAddr, dstAddr, outInterface, 0, IPPROTO_DSS, 64);
}

void DssHandleProtocolPacket(
	Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned ttl,
    int interfaceIndex)
{
	DssData* dss = (DssData*)NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSS);
	DssHeader*  dssHdr = (DssHeader*) MESSAGE_ReturnPacket(msg);

	switch(dssHdr->packetType)
	{	
		// Satellite received hello packet
		case DSS_HELLO_PACKET:
		{
			if(dss->enableDynamicRoute)
			{
				DssHandleHelloPacket(
					node, 
					msg, 
					dss, 
					sourceAddress,
					interfaceIndex);
			}

			break;
		}	
		// Satellite received LSU packet
		case DSS_LSU_PACKET:
		{
			if(dss->enableDynamicRoute)
			{
				DssHandleLsuPacket(
					node, 
					msg, 
					dss, 
					sourceAddress,
					interfaceIndex);
			}
			break;
		}		

		default: 
		{
			printf("Unknown Dss Packet Type for satellite!\n");
			break;
		}
	}

    MESSAGE_Free(node, msg);
}

void DssHandleHelloPacket(
	Node* node,
	Message* msg,
	DssData* dss,
	NodeAddress sourceAddress,
	int interfaceIndex)
{
	DssHelloInfo* helloInfo = (DssHelloInfo*) (MESSAGE_ReturnPacket(msg) + sizeof(DssHeader));
	DssInterfaceData* inf = & dss->infData[interfaceIndex];

	// Reset counter
	inf->counter = dss->keepAliveTTL;

	int linkId = inf->linkId;
	if(GetLinkStatus(linkId, dss) == DSS_LINK_DOWN)
	{
		// Update Revised Sign
		dss->reviseSign[linkId-1].sign = false;
		ReviseRoutingSnapshot(node, dss, &dss->revisedRS);
		UpdateRoutingTable(node);
		//Send lsu packet
		inf->linkChangeStat = DSS_RELINK;
		DssSendNewEvent(node, MSG_DSS_SendLsu, 0);
	}	
}

void DssHandleLsuPacket(
	Node* node,
	Message* msg,
	DssData* dss,
	NodeAddress sourceAddress,
	int interfaceIndex)
{
	DssLsuInfo* lsuInfo = (DssLsuInfo*) (MESSAGE_ReturnPacket(msg) + sizeof(DssHeader));
	int linkId = lsuInfo->linkId;

	bool flood = !(dss->reviseSign[linkId-1].sign == (lsuInfo->linkChangeStat == DSS_LINKDOWN)); // xor

	if(flood)
	{
		// Update Snapshot and Routing Table
		dss->reviseSign[linkId-1].sign = !dss->reviseSign[linkId-1].sign;
		ReviseRoutingSnapshot(node, dss, &dss->revisedRS);
		UpdateRoutingTable(node);
		// Send lsu packet via other available port
		dss->lsuData = *lsuInfo;
		for(int i = 0; i < MaxInfNum; i++)
		{
			if(i != interfaceIndex)
			{
				int linkId = dss->infData[i].linkId;
				if(GetLinkStatus(linkId, dss) == DSS_LINK_AVALIBLE)
				{
					DssSendProtocolPacket(
						node,
						dss,
						i,
						sourceAddress,
						DSS_LSU_PACKET);
				}
			}
		}
	}
}

void DssFinalize(
	Node* node,
	int interfaceIndex)
{

}
// end EXata Process Function

//---------------------
// LEO API
//---------------------
void ReadISLFile(DssData* dss)
{
	ifstream in;
	in.open(LINK_TABLE_FILE);
	string s;
	while (getline(in, s))
	{
		int p0 = s.find(",", 0);
		string sub = s.substr(1, p0);
		int id = atoi(sub.c_str());
		dss->G.isl[id - 1].linkId = id;

		int * value = (int*) dss->G.isl[id - 1].endpoint;
		for (int i = 0; i < 2; i++)
		{
			int p1 = s.find(":",p0);
			sub = s.substr(p0 + 1, p1 - p0 - 1);
			*value = atoi(sub.c_str());

			int p2 = s.find("|", p1);
			sub = s.substr(p1 + 1, p2 - p1 - 1);
			*(value+1) = atoi(sub.c_str());
			
			int p3 = s.find(",", p2);
			sub = s.substr(p2 + 1, p3 - p2 - 1);
			*(value+2) = atoi(sub.c_str());

			p0 = p3;
			value += 3;
		}
		value = NULL;

		int pos2 = s.find("]");
		sub = s.substr(p0 + 1, pos2 - p0 - 1);
		dss->G.isl[id - 1].weight = atoi(sub.c_str());

		dss->G.isl[id - 1].subnetIp = dss->G.isl[id - 1].endpoint[0].ip & 0xffffff00;
	}
	in.close();
}

// to avoid the mistake like "if(GetNodeId(node) = 1)"
int GetNodeId(Node* node)
{
	return node->nodeId;
}

int GetOrbitId(int nodeId)
{
	return (nodeId-1)/8 + 1;
}

int GetOrbitIndex(int nodeId)
{
	return (nodeId-1)%8 + 1;
}

int GetForeSatelliteId(int nodeId)
{
	int orbitId = GetOrbitId(nodeId);
	int index = GetOrbitIndex(nodeId);

	return (orbitId - 1)*8 + index % 8 + 1;
}

int GetRearSatelliteId(int nodeId)
{
	int orbitId = GetOrbitId(nodeId);
	int index = GetOrbitIndex(nodeId);
	
	return (orbitId - 1)*8 + (index + 6)%8 + 1;
}

int GetSideSatelliteId(
	int nodeId,
	bool isEast,
	bool isNorth)
{
	int orbitId = GetOrbitId(nodeId);
	int index = GetOrbitIndex(nodeId);
	int v = -1;

	if(orbitId == 1)
	{
		if(isEast)
		{
			if(isNorth)
				v = orbitId*8 + (index + 6)% 8;
			else
				v = orbitId*8 + index-1;
		}
	}
	else if(orbitId == 6)
	{
		if(!isEast)
		{
			if(isNorth)
				v = (orbitId - 2)*8 + index % 8;
			else
				v = (orbitId - 2)*8 + index-1;			
		}
	}
	else
	{
		if(isEast)
		{
			if(isNorth)
				v = orbitId*8 + (index + 6)% 8;
			else
				v = orbitId*8 + index-1;
		}
		else
		{
			if(isNorth)
				v = (orbitId - 2)*8 + index % 8;
			else
				v = (orbitId - 2)*8 + index-1;	
		}
	}

	return v + 1;
}

DssLinkStatType GetLinkStatus(
	int linkId,
	DssData* dss)
{
	if(linkId == 0)
	{
		return DSS_LINK_UNREACHABLE;
	}
	else
	{
		if(dss->reviseSign[linkId-1].sign)
		{
			return DSS_LINK_DOWN;
		}
		else
		{
			DssTopoSnapshot* topo = & dss->topoSnapshot[dss->curTopoId];
			if(topo->regularSign[linkId-1].sign) 
			{
				return DSS_LINK_UNREACHABLE;
			}			
		}
	}

	return DSS_LINK_AVALIBLE;
}

void UpdateCurrentLocationAndMotion(
	Node* node,
	DssData* dss)
{

	ifstream fileIn(MOBILITY_FILE, ios_base::binary);

	int t[2];
	int startTime[2];
	DssCoord location[2];
	t[0] = node->getNodeTime()/SECOND;
	t[1] = t[0] + 1;
	int v = GetNodeId(node)-1;
	
	for(int i = 0; i < 2; i++)
	{
		startTime[i] = v * DATA_SIZE + t[i] % DATA_SIZE;
		fileIn.seekg(sizeof(DssCoord) * startTime[i], ios_base::beg);
		fileIn.read((char *)(location + i), sizeof(DssCoord));
	}

	memcpy(& dss->curLocation, location, sizeof(DssCoord));
	dss->isNorth = (location[0].lat < location[1].lat);

	fileIn.close();
}

bool API_DssIsLinkValid(
	Node* node,
	int interfaceIndex)
{
	if(interfaceIndex == node->numberInterfaces-1) return true;

	DssData* dss = (DssData *) NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_DSS);
	clocktype time = node->getNodeTime();
	DssInterfaceData inf = dss->infData[interfaceIndex];

	// Dynamic Set from GUI
	if( (inf.isEnabled && (time < inf.startTime || time > inf.endTime))
		|| (!inf.isEnabled && (time > inf.startTime && time < inf.endTime)))
	{
		return false;
	}

	// Regular linkdown
	int srcNodeId = GetNodeId(node);
	int dstNodeId = -1;
	DssIsl isl = dss->G.isl[inf.linkId-1];
	for(int i = 0; i < 2; i++)
	{
		if(srcNodeId == isl.endpoint[i].nodeId)
		{
			dstNodeId = isl.endpoint[(i+1)%2].nodeId;
			break;
		}
	}

	if(dstNodeId == GetForeSatelliteId(srcNodeId) ||
		dstNodeId == GetRearSatelliteId(srcNodeId))
	{
		return true;
	}
	else if(dstNodeId == GetSideSatelliteId(srcNodeId, true, dss->isNorth) ||
		dstNodeId == GetSideSatelliteId(srcNodeId, false, dss->isNorth)) 
	{
		return (abs(dss->curLocation.lat) < DSS_BETA);
	}
	else
	{
		return false;
	}
}
// end LEO API

//------------------------------
// Routing Calculation Function
//------------------------------
int FindMin(
	unsigned int D[], 
	bool S[], 
	int n)
{
	int k = 0, min = MAX_ARC_WEIGHT;
	for(int i = 0; i < n; i++)
	{
		if(!S[i] && D[i] < min)
		{
			min = D[i];
			k = i;
		}
	}

	if(min == MAX_ARC_WEIGHT)
		return -1;

	return k;
}

void Dijkstra(
	DssArcNode arcs[][TotalNodeNum],
	int vertexNum,
	int sourceVertex,
	unsigned int distance[],
	int path[])
{
	//Init helping array
	int v = sourceVertex;
	bool S[TotalNodeNum];
	for(int i = 0; i < vertexNum; i++)
	{
		S[i] = false;
		distance[i] = arcs[v][i].weight;
		if(distance[i] != MAX_ARC_WEIGHT)
			path[i] = v;
		else
			path[i] = -1;
	}

	S[v] = true;
	distance[v] = 0;
	for(int i = 0; i < vertexNum; i++)
	{
		if((v = FindMin(distance, S, vertexNum)) == -1)
			return;
		S[v] = true;

		// Update helping array before next loop
		for(int j = 0; j < vertexNum; j++)
		{
			if(!S[j] && (distance[j] > arcs[v][j].weight + distance[v]))
			{
				distance[j] = arcs[v][j].weight + distance[v];
				path[j] = v;
			}
		}
	}
}

int FindPreNode(
	int path[], 
	int srcNode,
	int dstNode)
{
	int k = dstNode;

	if(path[k] == srcNode)
		return k;
	else if(path[k] == -1) //no pre node
		return -1;
	else
		FindPreNode(path, srcNode, path[k]);
}
// end Routing Calculation Function