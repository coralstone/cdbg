#include "log.h"
#include <iostream>
#include <iomanip>


typedef struct _ns_entry{

	DWORD state;        // 连接状态.
	DWORD locAddr;     	// 本地计算机地址.
	DWORD locPort;      // 本地计算机端口.
	DWORD remAddr;    	// 远程计算机地址.
	DWORD remPort;     	// 远程计算机端口.
	DWORD pid;          // 进程ID.
}ns_entry;

typedef struct _ns_state{
	const char* name;
}ns_state;

typedef struct _ns_table{
	DWORD    count;
	ns_entry data[1];
}ns_table;

#define NSS(n) #n,
const char* _nss[] = {
	NSS(*)
	NSS(CLOSED)
	NSS(LISTENING)
	NSS(SYN_SENT)
	NSS(SYN_RCVD)
	NSS(ESTABLISHED)
	NSS(FIN_WAIT1)
	NSS(FIN_WAIT2)
	NSS(CLOSE_WAIT)
	NSS(CLOSING)
	NSS(LAST_ACK)
	NSS(TIME_WAIT)
	NSS(DELETE_TCB)
};


typedef enum _TCP_OPM {
  TCP_LISTENER            ,
  TCP_CONNECTIONS         ,
  TCP_ALL                 ,
  TCP_OP_LISTENER        ,
  TCP_OP_CONNECTIONS     ,
  TCP_OP_ALL             ,
  TCP_OM_LISTENER     ,
  TCP_OM_CONNECTIONS  ,
  TCP_OM_ALL
} TCP_OPM ;

typedef enum _UDP_OPM
{
    UDP_BASIC,
    UDP_OP_ALL,
    UDP_OM_ALL,
} UDP_OPM;



typedef DWORD (WINAPI *fGetExtendedTcpTable)(
                PVOID pTcpTable, //返回查询结构体指针
                PDWORD pdwSize, //第一次调用该参数会返回所需要的缓冲区大小
                BOOL bOrder, //是否排序
                ULONG ulAf, //是 AF_INET还是AF_INET6
                TCP_OPM TableClass, // 表示结构体的种类，此处设为TCP_OP_ALL
                ULONG Reserved //保留不用，设为 0
);

typedef DWORD (WINAPI *fGetExtendedUdpTable)(
                PVOID pTcpTable, //返回查询结构体指针
                PDWORD pdwSize, //第一次调用该参数会返回所需要的缓冲区大小
                BOOL bOrder, //是否排序
                ULONG ulAf, //是 AF_INET还是AF_INET6
                UDP_OPM TableClass, // 表示结构体的种类，此处设为TCP_OP_ALL
                ULONG Reserved //保留不用，设为 0
);


#define TCP_OPA TCP_TABLE_OWNER_PID_ALL
#define UTP_OPA UDP_TABLE_OWNER_PID_ALL

enum ns_type{
	NS_TCP,NS_UDP
};
fGetExtendedTcpTable _get_ext_tcp;
fGetExtendedUdpTable _get_ext_udp;
DWORD  get_ip_table(ns_table*table,DWORD size,ns_type tp){


	if(tp == NS_TCP)
	_get_ext_tcp(table,&size,TRUE,AF_INET,TCP_OP_ALL,0);
    if(tp == NS_UDP)
	_get_ext_udp(table,&size,TRUE,AF_INET,UDP_OP_ALL,0);

	return size;

}
const char* ntoa(DWORD ip,WORD port,char* buff){
	BYTE* d= (BYTE*)&ip;
	sprintf(buff,"%d.%d.%d.%d:%d",d[0],d[1],d[2],d[3],port);
    return buff;
}
WORD htos(WORD val){


	BYTE d = val>>8 & 0xff;
	val = val<<8 & 0xff00;
	val |= d;

	return val;
}
#define SET_W(n) std::setw(n) << std::setfill(' ') << std::right
#define OUT_IP_ADDR(ip,port) \
	std::cout << SET_W(20) \
	<< ntoa(ip,htos(port),(char*)alloca(32))

#include "ps.h"
void out_ip_table(ns_table* table,ns_type tp)
{ /*
    std::cout << "table size:" << table->count << std::endl;
*/
	const char* nsp = (tp==NS_TCP)?"TCP":"UDP";
	char*buff = (char*)alloca(256);
	for(int i=0;i<table->count;i++){
		std::cout << nsp << std::left;
		ns_entry& ent = table->data[i];
		if(ent.state>12)
            ent.state = 0;

		OUT_IP_ADDR(ent.locAddr,ent.locPort);
        OUT_IP_ADDR(ent.remAddr,ent.remPort);


        std::cout << SET_W(12) << _nss[ent.state] << " "; //
        get_process_name(ent.pid,buff,false);
		std::cout << SET_W(4) << ent.pid ;
		std::cout << " " << buff;

		std::cout << std::endl;
	}

}

void display_netstat(){

	HMODULE hmod = ::LoadLibrary("iphlpapi.dll");
	if(!hmod)	return;
	_get_ext_tcp = (fGetExtendedTcpTable)
	::GetProcAddress(hmod,"GetExtendedTcpTable");
	_get_ext_udp = (fGetExtendedUdpTable)
	::GetProcAddress(hmod,"GetExtendedUdpTable");

	DWORD size=0;
	ns_table* table;
	// tcp
    //get_ip_table(0,size,NS_TCP);

    size = sizeof(ns_entry)*100+4;
	table= (ns_table*) alloca(size);

	get_ip_table(table,size,NS_TCP);
	out_ip_table(table,NS_TCP);
	//udp
//	size = get_ip_table(0,NS_UDP);
//	table= (ns_table*) alloca(size);
	get_ip_table(table,size,NS_UDP);
	out_ip_table(table,NS_UDP);

	::CloseHandle(hmod);

}
