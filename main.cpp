#include "UAS_InterfaceB.h"

int main(int argc, char** argv)
{
	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::GLOG_INFO,"logInfo");

	UAS_InterfaceB* pUAS = new UAS_InterfaceB(5060);
	int ret = pUAS->sip_init();
	if (ret == -1)
	{
		LOG(INFO) << "初始化失败，退出程序。";
		return -1;
	}
	LOG(INFO) << "启动线程；";
	pUAS->Start();

	delete pUAS;
	return 0;
}



