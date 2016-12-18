#include "UAS_InterfaceB.h"

int main(int argc, char** argv)
{
	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::GLOG_INFO,"logInfo");

	UAS_InterfaceB* pUAS = new UAS_InterfaceB;


	delete pUAS;
}



