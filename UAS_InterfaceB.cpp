#include "UAS_InterfaceB.h"
#include <iostream>


UAS_InterfaceB::UAS_InterfaceB(int port)
	:je(NULL), ack(NULL), invite(NULL), response(NULL), remote_sdp(NULL)
{
	LOG(INFO) << "B�ӿڳ�ʼ��...";
	excontext = eXosip_malloc();
	this->port = port;
}


UAS_InterfaceB::~UAS_InterfaceB()
{
	LOG(INFO) << "B�ӿ��������߳��˳�...";
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
	// ��ʼ�� sip
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
	LOG(INFO) << "SIP�̼߳�����...";
	for (;;)
	{
		// �����Ƿ�����Ϣ����
		je = eXosip_event_wait(excontext,0, 50);
		// Э��ջ���д����,��������δ֪
		eXosip_lock(excontext);
		eXosip_default_action(excontext,je);
		eXosip_automatic_action(excontext);
		eXosip_unlock(excontext);


		if (je == NULL) // û�н��յ���Ϣ������
		{
			continue;
		}

		switch (je->type)
		{
		case EXOSIP_MESSAGE_NEW: // �µ���Ϣ����
			std::cout << "\n\t*** EXOSIP_MESSAGE_NEW!\n" << std::endl;
			if (MSG_IS_MESSAGE(je->request)) // ������յ�����Ϣ������ MESSAGE
			{
				{
					osip_body_t *body;
					osip_message_get_body(je->request, 0, &body);
					std::cout << "I get the msg is: " << body->body << std::endl;
				}

				// ���չ�����Ҫ�ظ� OK ��Ϣ
				eXosip_message_build_answer(excontext,je->tid, 200, &response);
				eXosip_message_send_answer(excontext,je->tid, 200, response);
			}
			break;

		case EXOSIP_CALL_INVITE: // INVITE ������Ϣ
				// �õ����յ���Ϣ�ľ�����Ϣ
			std::cout << "\n\tReceived a INVITE msg from " << je->request->req_uri->host
				<< " : " << je->request->req_uri->port
				<< ", username is " << je->request->req_uri->username << std::endl;
			// �õ���Ϣ��,��Ϊ����Ϣ���� SDP ��ʽ.
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

				// ���ûظ���SDP��Ϣ��,��һ���ƻ�������Ϣ��
				// û�з�����Ϣ�壬ֱ�ӻظ�ԭ������Ϣ����һ�����Ĳ��á�
				osip_message_set_body(response, tmp, strlen(tmp));
				osip_message_set_content_type(response, "application/sdp");

				eXosip_call_send_answer(excontext,je->tid, 200, response);

				std::cout << "\n\t--> send 200 over!" << std::endl;
			}

			eXosip_unlock(excontext);
			// ��ʾ���� sdp ��Ϣ���е� attribute ������,����ƻ�������ǵ���Ϣ
			std::cout << "\n\t--> The INFO is :\n";

			while (!osip_list_eol(&(remote_sdp->a_attributes), pos))
			{
				sdp_attribute_t *at;
				//���������Ϊʲô��SDP��Ϣ��������a�����ű���������
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
			if (MSG_IS_INFO(je->request)) // ���������� INFO ����
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
