#ifndef ROUTING_DSS_H
#define ROUTING_DSS_H

#define API_LEO_TOPO_CHANGE

#define DSS_M 6 // number of orbit
#define DSS_N 8 // number of satellite on one orbit
#define TotalNodeNum DSS_M * DSS_N
#define TotalLinkNum 128
#define TotalTopoSnapshotNum 95
#define SnapshotTableSize 100
#define MaxInfNum 6

#define DATA_SIZE 86401 //The size of mobility data : 24*60*60+1 24 hours data per second
#define LINK_TABLE_FILE "C:\\Scalable\\exata\\5.1\\scenarios\\user\\satellites\\DSS\\linktable.txt"
#define CHANGE_TIME_FILE "C:\\Scalable\\exata\\5.1\\scenarios\\user\\satellites\\DSS\\changeTime.txt"
#define MOBILITY_FILE "C:\\Scalable\\exata\\5.1\\scenarios\\user\\satellites\\mobilityFile\\orbit.bin"

#define MAX_ARC_WEIGHT 1000

#define DSS_LINKID_SELFLOOP 0
#define DSS_LINKID_NOLINK -1
#define DSS_BETA 70

#define DSS_DEFAULT_ENABLE_MAPPING true
#define DSS_DEFAULT_MAPPING_INTERVAL 1*SECOND

#define DSS_DEFAULT_ENABLE_DYNAMICROUTE FALSE
#define DSS_DEFAULT_ENABLE_PORT TRUE
#define DSS_DEFAULT_KEEPALIVE_TTL 4
#define DSS_DEFAULT_HELLO_INTERVAL 1*SECOND
#define DSS_DEFAULT_PORT_START_TIME 0*SECOND
#define DSS_DEFAULT_PORT_END_TIME 24*3600*SECOND

//--------------------
// DSS Data Structure
//--------------------
enum DssPacketType
{
	DSS_HELLO_PACKET = 1,
	DSS_LSU_PACKET = 2
};

struct DssHeader {
	DssPacketType packetType;
};

// The info structure filled in hello packet
struct DssHelloInfo
{
	int satelliteId;
	int portId;
};

enum DssLinkStatType
{
	DSS_LINK_AVALIBLE = 1,
	DSS_LINK_UNREACHABLE = 2,
	DSS_LINK_DOWN = 3,
};

enum DssLinkChangeStatType
{
	DSS_LINKDOWN = 1,
	DSS_RELINK = 2,
	DSS_NOCHANGE = 0,
};

// The info structure filled in lsu packet
struct DssLsuInfo
{
	int linkId;
	DssLinkChangeStatType linkChangeStat;

	// to measure the convergence time
	clocktype txTime;
};

struct DssIslNode
{
	int nodeId;
	int inf;
	NodeAddress ip;
};

struct DssIsl
{
	int linkId;
	NodeAddress subnetIp;
	unsigned int weight;

	DssIslNode endpoint[2];
};

struct DssArcNode
{
	int linkId;
	unsigned int weight;
};

struct DssStaticTopo
{
	DssIsl isl[TotalLinkNum];
	DssArcNode arcs[TotalNodeNum][TotalNodeNum]; // weight;
};

struct DssIslSign
{
	int linkId;
	bool sign;
};

struct DssRoutingSnapshotRow
{
	int targetNodeId;
	int nextHopNodeId;
	int nextLinkId;
};

struct DssRoutingSnapshot
{
	DssRoutingSnapshotRow routingRow[TotalNodeNum];
};

struct DssCoord
{
	double lat;
	double lon;
};

struct DssMapping
{
	DssCoord left;
	DssCoord right;
	bool leftIsNorth;
	bool rightIsNorth;
};

struct DssTopoSnapshot
{
	int topoId;

	DssMapping map;
	DssIslSign regularSign[TotalLinkNum];

	DssRoutingSnapshot routingSnapshot;
};

struct DssInterfaceData
{
	int counter; // counter for sending hello packet
	int linkId; // static link
	DssLinkChangeStatType linkChangeStat;

	// for GUI set
	bool isEnabled;
	clocktype startTime;
	clocktype endTime;
};

typedef struct struct_network_dss_str
{
	DssStaticTopo G;
	DssTopoSnapshot topoSnapshot[SnapshotTableSize];

	DssIslSign reviseSign[TotalLinkNum];
	DssRoutingSnapshot revisedRS; 

	int curTopoId;
	DssCoord curLocation;
	bool isNorth;

	bool enableMapping;
	clocktype mappingInterval;

	// Dynamic Route
	bool enableDynamicRoute;
	clocktype helloInterval;
	int keepAliveTTL;
	DssInterfaceData infData[MaxInfNum];
	DssLsuInfo lsuData;
	
} DssData;
// end DSS Data Structure

//----------------------
// Dss Core Function
//----------------------
void InitStaticTopo(
	Node* node,
	DssData* dss);

void InitTopoSnapShot(
	Node* node,
	DssData* dss);

bool SatelliteUpdateCurrentSnapshot(
	Node* node,
	DssData* dss);

bool SatelliteIsInArea(
	double lat,
	bool isNorth,
	double leftLat,
	bool leftIsNorth,
	double rightLat,
	bool rightIsNorth);

void ReviseRoutingSnapshot(
	Node* node,
	DssData* dss,
	DssRoutingSnapshot* rs);

void DssRouterFunction(
	Node* node,
    Message* msg,
    NodeAddress destAddr,
    NodeAddress previousHopAddress,
    BOOL* packetWasRouted);
// end DSS Core Function

//----------------------
// Networklayer API
//----------------------
void UpdateRoutingTable(Node* node);

void FindNextHopByTargetNode(
	Node* node,
	DssData* dss,
	int dstNode,
	NodeAddress &nextHopAddr,
	int &interfaceIndex,
	int &cost);
// end NetworkLayer API

//-------------------------
// EXata Process Function
//-------------------------
void DssInit(
	Node* node,
	DssData** dssPtr,
	const NodeInput* nodeInput,
	int interfaceIndex,
	NetworkRoutingProtocolType dssProtocolType);

void DssInitializeConfigurableParameters(
	Node* node,
	const NodeInput* nodeInput,
	DssData* dss,
	int interfaceIndex);

void DssInitInterfaceData(
	Node* node,
	const NodeInput* nodeInput,
	int interfaceIndex,
	DssData* dss);

void DssSendNewEvent(
	Node* node,
	int eventType,
	clocktype delay);

void DssHandleRoutingProtocolEvent(
	Node* node,
	Message* msg);

void DssSendHello(
	Node* node,
	DssData* dss);

void DssSendLsu(
	Node* node,
	DssData* dss);

void DssSendProtocolPacket(
	Node* node,
	DssData* dss,
	int outInterface,
	NodeAddress sourceAddress,
	DssPacketType packetType);

void DssHandleProtocolPacket(
	Node* node,
    Message* msg,
    NodeAddress sourceAddress,
    NodeAddress destinationAddress,
    TosType priority,
    unsigned ttl,
    int interfaceIndex);

void DssHandleHelloPacket(
	Node* node,
	Message* msg,
	DssData* dss,
	NodeAddress sourceAddress,
	int interfaceIndex);

void DssHandleLsuPacket(
	Node* node,
	Message* msg,
	DssData* dss,
	NodeAddress sourceAddress,
	int interfaceIndex);

void DssFinalize(
	Node* node,
	int interfaceIndex);
// end EXata Process Function

//------------------
// LEO API
//------------------
void ReadISLFile(DssData* dss);

int GetNodeId(Node* node);

int GetOrbitId(int nodeId);

int GetOrbitIndex(int nodeId);

int GetForeSatelliteId(int nodeId);

int GetRearSatelliteId(int nodeId);

int GetSideSatelliteId(
	int nodeId,
	bool isEast,
	bool isNorth);

DssLinkStatType GetLinkStatus(
	int linkId,
	DssData* dss);

void UpdateCurrentLocationAndMotion(
	Node* node,
	DssData* dss);

bool API_DssIsLinkValid(
	Node* node,
	int inf);
// end LEO API

//------------------------------
// Routing Calculation Function
//------------------------------
int FindMin(
	unsigned int D[], 
	bool S[], 
	int n);

void Dijkstra(
	DssArcNode arcs[][TotalNodeNum],
	int vertexNum,
	int sourceVertex,
	unsigned int distance[],
	int path[]);

int FindPreNode(
	int path[],
	int srcNode,
	int dstNode);
// end Routing Calculation Function

#endif