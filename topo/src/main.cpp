#include "timer.h"
#include "common.h"
#include "route.h"
#include <iostream>
#include <unistd.h>

#include "topo.h"
#include "node.h"
#include "cmd.h"

using namespace std;

int main(int argc,char *argv[])
{
	TopoInitialize();

	string s;
	cout << "Waiting for start..." << endl;
	do{
		cin >> s;
		sys_stat=s;
	}while(s != "start");

	StartTimer();

	do{
		cin >> s;
		if(s == "stop"){
			sys_stat=s;
			EndTimer();	
			cout<<"task down!"<<"\n";	
		}
		else if(s == "start"){
			sys_stat=s;
			StartTimer();
		}
	}while(s != "exit");
	sys_stat=s;
	EndTimer();

	TopoFinalize();
	
	return 0;
}
