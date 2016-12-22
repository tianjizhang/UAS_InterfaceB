#include "UAS_InterfaceB.h"
#include <iostream>


UAS_InterfaceB::UAS_InterfaceB(int port)
	:je(NULL), ack(NULL), invite(NULL), response(NULL), remote_sdp(NULL)
{
	LOG(INFO) << "B接口初始化...";
	excontext = eXosip_malloc();
	this->port = port;
}


UAS_InterfaceB::~UAS_InterfaceB()
{
	LOG(INFO) << "B接口析构，线程退出...";
	if (je != NULL)
	{
		delete je;
	}
	if (ack != NULL)
	{
		delete ack;
	}
	if (invite != NULL)
	{
		delete invite;
	}
	if (response != NULL)
	{
		delete response;
	}
	if (excontext != NULL)
	{
		delete excontext;
	}

	this->Kill();
}

int UAS_InterfaceB::sip_init()
{
	// 初始化 sip
	int ret = eXosip_init(excontext);
	if (ret != 0)
	{
		LOG(INFO) << "\n\t--> Can't initialize eXosip!\n";
		return -1;
	}
	ret = eXosip_listen_addr(excontext, IPPROTO_UDP, NULL, port, AF_INET, 0);
	if (ret != 0)
	{
		eXosip_quit(excontext);
		LOG(INFO) << "\n\t--> eXosip_listen_addr error! Couldn't initialize transport layer!\n";
		return -1;
	}

	LOG(INFO) << "\n\t--> eXosip start listening at port: "<<port<<"!\n";
	return 0;
}

void * UAS_InterfaceB::Thread()
{
	int call_id ;
	int dialog_id ;
	int ret;
	int pos = 0;
	char tmp[4096];
	LOG(INFO) << "SIP线程监听中...";
	for (;;)
	{
		// 侦听是否有消息到来
		je = eXosip_event_wait(excontext,0, 50);
		// 协议栈带有此语句,具体作用未知
		eXosip_lock(excontext);
		eXosip_default_action(excontext,je);
		eXosip_automatic_action(excontext);
		eXosip_unlock(excontext);


		if (je == NULL) // 没有接收到消息，继续
		{
			continue;
		}

		switch (je->type)
		{
		case EXOSIP_MESSAGE_NEW: // 新的消息到来
			std::cout << "\n\t*** EXOSIP_MESSAGE_NEW!\n" << std::endl;
			if (MSG_IS_MESSAGE(je->request)) // 如果接收到的消息类型是 MESSAGE
			{
				{
					osip_body_t *body;
					osip_message_get_body(je->request, 0, &body);
					std::cout << "I get the msg is: " << body->body << std::endl;
				}

				// 按照规则，需要回复 OK 信息
				eXosip_message_build_answer(excontext,je->tid, 200, &response);
				eXosip_message_send_answer(excontext,je->tid, 200, response);
			}
			break;

		case EXOSIP_CALL_INVITE: // INVITE 请求消息
				// 得到接收到消息的具体信息
			std::cout << "\n\tReceived a INVITE msg from " << je->request->req_uri->host
				<< " : " << je->request->req_uri->port
				<< ", username is " << je->request->req_uri->username << std::endl;
			// 得到消息体,认为该消息就是 SDP 格式.
			remote_sdp = eXosip_get_remote_sdp(excontext,je->did);
			call_id = je->cid;
			dialog_id = je->did;
			eXosip_lock(excontext);
			eXosip_call_send_answer(excontext,je->tid, 180, NULL);
			ret = eXosip_call_build_answer(excontext,je->tid, 200, &response);
			if (ret != 0)
			{
				std::cout << "\n\t--> This request msg is invalid! Cann't response!\n" << std::endl;
				eXosip_call_send_answer(excontext,je->tid, 400, NULL);
			}
			else
			{
				snprintf(tmp, 4096,
					"v=0\r\n"
					"o=anonymous 0 0 IN IP4 0.0.0.0\r\n"
					"t=1 10\r\n"
					"a=username:rainfish\r\n"
					"a=password:123\r\n");

				// 设置回复的SDP消息体,下一步计划分析消息体
				// 没有分析消息体，直接回复原来的消息，这一块做的不好。
				osip_message_set_body(response, tmp, strlen(tmp));
				osip_message_set_content_type(response, "application/sdp");

				eXosip_call_send_answer(excontext,je->tid, 200, response);

				std::cout << "\n\t--> send 200 over!" << std::endl;
			}

			eXosip_unlock(excontext);
			// 显示出在 sdp 消息体中的 attribute 的内容,里面计划存放我们的信息
			std::cout << "\n\t--> The INFO is :\n";

			while (!osip_list_eol(&(remote_sdp->a_attributes), pos))
			{
				sdp_attribute_t *at;
				//这里解释了为什么在SDP消息体中属性a里面存放必须是两列
				at = (sdp_attribute_t *)osip_list_get(&remote_sdp->a_attributes, pos);
				std::cout << "\n\t" << at->a_att_field
					<< " : " << at->a_att_value << std::endl;
				pos++;
			}
			break;
		case EXOSIP_CALL_ACK:
			std::cout << "\n\t--> ACK recieved!\n" << std::endl;
			// printf ("the cid is %s, did is %s\n", je->did, je->cid); 
			break;
		case EXOSIP_CALL_CLOSED:
			std::cout << "\n\t--> the remote hold the session!\n" << std::endl;

			// eXosip_call_build_ack(dialog_id, &ack);

			// eXosip_call_send_ack(dialog_id, ack); 

			ret = eXosip_call_build_answer(excontext,je->tid, 200, &response);

			if (ret != 0)
			{
				printf("This request msg is invalid!Cann't response!\n");
				eXosip_call_send_answer(excontext,je->tid, 400, NULL);
			}
			else
			{
				eXosip_call_send_answer(excontext,je->tid, 200, response);
				std::cout << "\n\t--> bye send 200 over!\n";
			}
			break;
		case EXOSIP_CALL_MESSAGE_NEW:
			std::cout << "\n\t*** EXOSIP_CALL_MESSAGE_NEW\n" << std::endl;
			if (MSG_IS_INFO(je->request)) // 如果传输的是 INFO 方法
			{
				eXosip_lock(excontext);

				ret = eXosip_call_build_answer(excontext,je->tid, 200, &response);

				if (ret == 0)
				{
					eXosip_call_send_answer(excontext,je->tid, 200, response);
				}

				eXosip_unlock(excontext);
				{
					osip_body_t *body;
					osip_message_get_body(je->request, 0, &body);
					std::cout << "the body is " << body->body << std::endl;
				}
			}
			break;
		default:
			std::cout << "\n\t--> Could not parse the msg!\n" << std::endl;
		}
	}
	return 0;
}
