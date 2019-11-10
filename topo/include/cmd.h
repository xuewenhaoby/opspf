#ifndef __CMD_H__
#define __CMD_H__

void run_cmd(char *cmd,const char *attr);

void add_namespace(const char *name, int id);
void del_namespace(const char *name, int id);

void add_bridge(const char *name,int id);
void del_bridge(const char *name,int id);

void addLink_NsBr(
	const char *nsName,int nsId,int nsPortId,
	const char *brName,int brId,int brPortId);

void addLink_BrBr(
	const char *name1,int id1,int portId1,
	const char *name2,int id2,int portId2);

void delLink_BrBr(const char *name,int id,int portId);

void addLink_NsNs(
	const char *nsName1,int nsId1,int pId1,
	const char *nsName2,int nsId2,int pId2);

void change_LinkState(
	const char *name,int id,
	int portId,bool state);

void set_VethIp(
	const char *name,int id,
	int portId,const char *address);

//ipv6_set
void set_VethIP_6(
	const char *name,int id,int portId,
	const char *addr);
//addr_clear
void clear_6(
	const char *name,int id,int portId);
//


void add_node_namespace(
	const char *name,const char *br_name,
	int id,int addr,int maskSize);

void del_node_namespace(const char *name,const char *br_name,int id);

void add_satsat_link(
	const char *sname,int ids[],int pIds[],
	int addrs[],int maskSize,int linkId);

void add_sathost_link(
	const char *name1,int id1,int portId1,
	const char *name2,int id2,int portId2);

void print_LinkState(const char *name,int id,int portId);
void print_VethIp(const char *name,int id,int portId);

void IpStr(char *buf,int addr,int maskSize);
void BIpStr(char *buf,int addr,int maskSize);

//ipv6_node_addr
void IpStr_node_6(char *buf,int id,int maskSize);
//ipv6_port_addr
void IpStr_port_6(char *buf,int id,int portid,int maskSize,int linkId);
//ipv6_link_net
void IpStr_net_link(char *buf,int linkId,int masksize);
//ipv6_node_net
void IpStr_net(char *buf,int id,int maskSize);
//ipv6_gw_addr
void IpStr_gw_6(char *buf,int gw_id,int gw_portid,int linkId);
//ipv6_route_write
void route_write(
	const char *name,int src_id,int gw_id,
    int dst_id,int masksize,int src_portid,int gw_portid,int linkId) ;
//ipv6_link_route_write
void link_route_write(
	const char *name,int src_id,int gw_id,
    int dst_id,int masksize,int src_portid,int gw_portid,int linkId,ISDB Int_DB[]);
//turning up ip_exchange
void exchange_up(const char *name,int src_id);
#endif