#include "common.h"
#include "cmd.h"
#include "topo.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sys/wait.h>

#define runCmd_screen(m) run_cmd(m,"")
#define runCmd_noReply(m) run_cmd(m,"1> /dev/null 2> /dev/null") 
#define runCmd_toLog(m) run_cmd(m,"1> output_normal.txt 2> output_error.txt") 

extern int errno;

void run_cmd(char *_cmd,const char *attr)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"%s %s",_cmd,attr);
	if(system(cmd) < 0){
		printf("%s error:%s\n",cmd,strerror(errno));
	}
}

void add_namespace(const char *name,int id)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns add %s%d",name,id);
	runCmd_noReply(cmd);
	sprintf(cmd,"ip netns exec %s%d ip link set dev lo up",name,id);
	runCmd_noReply(cmd);
	exchange_up(name,id);
}

void del_namespace(const char *name,int id)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns del %s%d",name,id);
	runCmd_noReply(cmd);
}

void add_bridge(const char *name,int id)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"brctl addbr %s%d",name,id);
	runCmd_noReply(cmd);
	sprintf(cmd,"brctl stp %s%d on",name,id);
	runCmd_noReply(cmd);
	sprintf(cmd,"ip link set dev %s%d up",name,id);
	runCmd_noReply(cmd);
}

void del_bridge(const char *name,int id)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip link set dev %s%d down",name,id);
	runCmd_noReply(cmd);
	sprintf(cmd,"brctl delbr %s%d",name,id);
	runCmd_noReply(cmd);
}

void addLink_NsBr(
	const char *nsName,int nsId,int nsPortId,
	const char *brName,int brId,int brPortId)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip link add %s%dp%d type veth peer name %s%dp%d",
		brName,brId,brPortId,nsName,nsId,nsPortId);
	runCmd_noReply(cmd);

	sprintf(cmd,"brctl addif %s%d %s%dp%d",
		brName,brId,brName,brId,brPortId);
	runCmd_noReply(cmd);

	sprintf(cmd,"ip link set %s%dp%d netns %s%d",
		nsName,nsId,nsPortId,nsName,nsId);
	runCmd_noReply(cmd);

	sprintf(cmd,"ip link set dev %s%dp%d up",
		brName,brId,brPortId);
	runCmd_noReply(cmd);

	sprintf(cmd,"ip netns exec %s%d ip link set dev %s%dp%d up",
		nsName,nsId,nsName,nsId,nsPortId);
	runCmd_noReply(cmd);
}

void addLink_BrBr(
	const char *name1,int id1,int portId1,
	const char *name2,int id2,int portId2)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip link add %s%dp%d type veth peer name %s%dp%d",
		name1,id1,portId1,name2,id2,portId2);
	runCmd_noReply(cmd);

	sprintf(cmd,"brctl addif %s%d %s%dp%d",
		name1,id1,name1,id1,portId1);
	runCmd_noReply(cmd);

	sprintf(cmd,"brctl addif %s%d %s%dp%d",
		name2,id2,name2,id2,portId2);
	runCmd_noReply(cmd);
}

void delLink_BrBr(const char *name,int id,int portId)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip link del %s%dp%d",name,id,portId);
	runCmd_noReply(cmd);
}

void addLink_NsNs(
	const char *nsName1,int nsId1,int pId1,
	const char *nsName2,int nsId2,int pId2)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip link add %s%dp%d type veth peer name %s%dp%d",
		nsName1,nsId1,pId1,nsName2,nsId2,pId2);
	runCmd_noReply(cmd);

	const char *name[2] = {nsName1,nsName2};
	int nsId[2] = {nsId1,nsId2};
	int pId[2] = {pId1,pId2}; 

	for(int i = 0; i < 2; i++){
		sprintf(cmd,"ip link set %s%dp%d netns %s%d",
			name[i],nsId[i],pId[i],name[i],nsId[i]);
		runCmd_noReply(cmd);

		// sprintf(cmd,"ip netns exec %s%d ip link set dev %s%dp%d up",
		// 	name[i],nsId[i],name[i],nsId[i],pId[i]);
		// runCmd_noReply(cmd);
	}	
}

void change_LinkState(
	const char *name,int id,
	int portId,bool state)
{
	char stat[BUF_STR_SIZE];
	if(state){
		sprintf(stat,"up");
	}
	else{
		sprintf(stat,"down");
	}

	char cmd[BUF_STR_SIZE];
	if(strcmp(name,HOST_BR_NAME) == 0 || 
		strcmp(name,SAT_BR_NAME) == 0)
	{
		sprintf(cmd,"ip link set dev %s%dp%d %s",name,id,portId,stat);
	}
	else if(strcmp(name,SAT_NAME) == 0 || 
		strcmp(name,HOST_NAME) == 0)
	{
		sprintf(cmd,"ip netns exec %s%d ip link set dev %s%dp%d %s",
			name,id,name,id,portId,stat);
	}
	else{
		printf("Wrong name!\n");
	}
	
	runCmd_noReply(cmd);
}

void set_VethIp(
	const char *name,int id,int portId,
	const char *addr,const char *brAddr)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns exec %s%d ip addr add %s broadcast %s dev %s%dp%d",
		name,id,addr,brAddr,name,id,portId);
	runCmd_noReply(cmd);	
}


//ipv6_set
void set_VethIP_6(
	const char *name,int id,int portId,
	const char *addr)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns exec %s%d ip -6 addr add %s dev %s%dp%d",
		name,id,addr,name,id,portId);
	runCmd_noReply(cmd);	
}
//addr_clear
void clear_6(
	const char *name,int id,int portId)
{
    char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns exec %s%d ip -6 addr flush %s%dp%d",
		name,id,name,id,portId);
	runCmd_noReply(cmd);	
}
//


void print_LinkState(const char *name,int id,int portId)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip link | grep %s%dp%d@",name,id,portId);
	runCmd_noReply(cmd);
}

void print_VethIp(const char *name,int id,int portId)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns exec %s%d ip addr | grep 'global %s%dp%d'"
		,name,id,name,id,portId);
	runCmd_noReply(cmd);	
}

void add_node_namespace(
	const char *name,const char *br_name,
	int id,int addr,int maskSize)
{
	int portId = 0;;
	if(strcmp(name,SAT_NAME) == 0) portId = 6;

	add_namespace(name,id);
	add_bridge(br_name,id);
	addLink_NsBr(name,id,portId,br_name,id,0);

	char ipStr[BUF_STR_SIZE], bIpStr[BUF_STR_SIZE];
	//xry_add
	char ipStr_6[BUF_STR_SIZE];
	IpStr_node_6(ipStr_6,id,64);
	set_VethIP_6(name,id,portId,ipStr_6);
	//
	IpStr(ipStr,addr,maskSize);
	BIpStr(bIpStr,addr,maskSize);
	set_VethIp(name,id,portId,ipStr,bIpStr);
}

void add_satsat_link(
	const char *sname,int ids[],int pIds[],
	int addrs[],int maskSize,int linkId)
{
	addLink_NsNs(sname,ids[0],pIds[0],sname,ids[1],pIds[1]);

	char ipStr[BUF_STR_SIZE], bIpStr[BUF_STR_SIZE];
	//xry_add
	char ipStr_6[BUF_STR_SIZE];
	//
	for(int i = 0; i < 2; i++){
		IpStr(ipStr,addrs[i],maskSize);
		BIpStr(bIpStr,addrs[i],maskSize);
		set_VethIp(sname,ids[i],pIds[i],ipStr,bIpStr);
		//xry_add
		IpStr_port_6(ipStr_6,ids[i],pIds[i],64,linkId);
		set_VethIP_6(sname,ids[i],pIds[i],ipStr_6);
	}
}

void add_sathost_link(
	const char *name1,int id1,int portId1,
	const char *name2,int id2,int portId2)
{
	addLink_BrBr(name1,id1,portId1,name2,id2,portId2);
}

void del_node_namespace(const char *name,const char *br_name,int id)
{
	del_namespace(name,id);
	del_bridge(br_name,id);	
}

void IpStr(char *buf,int addr,int maskSize)
{
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	sprintf(buf,"%d.%d.%d.%d/%d",a1,a2,a3,a4,maskSize);
}

void BIpStr(char *buf,int addr,int maskSize)
{
	int tmp = 0;
	for(int i = 0; i < 32-maskSize;i++){
		tmp <<= 1;
		tmp |= 0x00000001;
	}
	addr |= tmp;
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	sprintf(buf,"%d.%d.%d.%d",a1,a2,a3,a4);
}

//ipv6_node_addr
void IpStr_node_6(char *buf,int id,int maskSize)
{
	sprintf(buf,"2003:%d::1/%d",id,maskSize);
}
//ipv6_port_addr
void IpStr_port_6(char *buf,int id,int portid,int maskSize,int linkId)
{
	int n = portid +1;
	sprintf(buf,"2003:0:%d::%d:%d/%d",linkId,id,n,maskSize);
}
//ipv6_link_net
void IpStr_net_link(char *buf,int linkId,int masksize)
{
	sprintf(buf,"2003:0:%d::/%d",linkId,masksize);
}
//ipv6_gw_addr
void IpStr_gw_6(char *buf,int gw_id,int gw_portid,int linkId)
{
	int n = gw_portid +1;
	sprintf(buf,"2003:0:%d::%d:%d",linkId,gw_id,n);
}
//ipv6_node_net
void IpStr_net(char *buf,int dst_id,int maskSize)
{
	sprintf(buf,"2003:%d::/%d",dst_id,maskSize);
}
//ipv6_route_write
void route_write(
	const char *name,int src_id,int gw_id,
    int dst_id,int masksize,int src_portid,int gw_portid,int linkId) 
{
	char dst_netStr[BUF_STR_SIZE];
	char gw_ipStr[BUF_STR_SIZE];
	IpStr_net(dst_netStr,dst_id,masksize);
	IpStr_gw_6(gw_ipStr,gw_id,gw_portid,linkId);
	char cmd[BUF_STR_SIZE],cmd_del[BUF_STR_SIZE];
	sprintf(cmd_del,"ip netns exec %s%d ip -6 route del %s",name,src_id,dst_netStr);
	runCmd_noReply(cmd_del);

	sprintf(cmd,"ip netns exec %s%d ip -6 route add %s via %s dev %s%dp%d",
		name,src_id,dst_netStr,gw_ipStr,name,src_id,src_portid);
	runCmd_noReply(cmd);

}

//ipv6_link_route_write
void link_route_write(
	const char *name,int src_id,int gw_id,
    int dst_id,int masksize,int src_portid,int gw_portid,int linkId,ISDB Int_DB[])
{
	char dstlink_netStr[BUF_STR_SIZE];
	char gw_ipStr[BUF_STR_SIZE];
	int temp_link;
	for(int p=0;p<SINF_NUM;p++)
	{
		if(Int_DB[dst_id-1].port_stat[p]==true && Int_DB[dst_id-1].dst_satid[p]!=src_id)
		{
			temp_link=Int_DB[dst_id-1].linkId[p];
			IpStr_net_link(dstlink_netStr,temp_link,masksize);
	        IpStr_gw_6(gw_ipStr,gw_id,gw_portid,linkId);
	        char cmd[BUF_STR_SIZE],cmd_del[BUF_STR_SIZE];
			sprintf(cmd_del,"ip netns exec %s%d ip -6 route del %s",name,src_id,dstlink_netStr);
			runCmd_noReply(cmd_del);
	        sprintf(cmd,"ip netns exec %s%d ip -6 route add %s via %s dev %s%dp%d",
		    name,src_id,dstlink_netStr,gw_ipStr,name,src_id,src_portid);
			runCmd_noReply(cmd);
		}
	}
}
//turning up ip_exchange
void exchange_up(const char *name,int src_id)
{
	char cmd[BUF_STR_SIZE];
	sprintf(cmd,"ip netns exec %s%d sysctl net.ipv6.conf.all.forwarding=1",name,src_id);
	runCmd_noReply(cmd);
}