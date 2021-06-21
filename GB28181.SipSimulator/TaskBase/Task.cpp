#include "./../StdAfx.h"
#include "Task.h"
#include "./../Common/SipParser.h"

bool CTask::m_bIsExit = false;

//�����¼�����������׽������顣  
WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];
SOCKET socketArray[WSA_MAXIMUM_WAIT_EVENTS];
DWORD totalEvent;//�¼�����������

CTask::CTask(SOCKET sock, SOCKADDR_IN tSockAddr)
	: nConc(1), nMaxPerSec(1),
	nMaxCount(1),
	m_sock(sock),
	m_tSockAddr(tSockAddr)
{
}


CTask::~CTask(void)
{
}

void CTask::InitParam()
{
	// ��ʼ�����Բ���
	printf("�����߳�����1");
	//scanf_s("%d", &nConc);
	nConc = 1;
	printf("\n");
	printf("���߳�ÿ������ʹ����������޷��ﵽ����");
	scanf_s("%d", &nMaxPerSec);
	printf("\n");
	printf("�ܹ����Ͷ��ٴ�����������");
	scanf_s("%d", &nMaxCount);
	printf("\n");

}

void CTask::CreateRecvThread(SOCKET sock)
{
	AfxBeginThread(pfnRecvProc, reinterpret_cast<LPVOID>(sock));
}

UINT AFX_CDECL CTask::pfnRecvProc( LPVOID lParam )
{
	auto sock = reinterpret_cast<SOCKET>(lParam);;
	//fd_set		read_set;
	//timeval		timeout;
	char		buf[4096]={0};
	SOCKADDR_IN	sAddr;
	int			nRecv = sizeof(SOCKADDR_IN);
	//timeout.tv_sec = 5;
	//timeout.tv_usec = 0;
	socketArray[totalEvent] = sock;
	eventArray[totalEvent++]=  WSACreateEvent();

	auto retSelect = WSAEventSelect(sock,eventArray,FD_READ|FD_CLOSE);

	if(!retSelect)
	{
		auto error= WSAGetLastError();
		return error;
	}

	while(!m_bIsExit)
	{
		// �ȴ���¼Ӧ��
		//FD_ZERO(&read_set);
		//FD_SET(sock, &read_set);
		//auto ret = select(0, &read_set, nullptr, nullptr, &timeout);
		auto dwIndex = WSAWaitForMultipleEvents(totalEvent,eventArray,false,WSA_INFINITE,false);
		if (dwIndex == WSA_WAIT_FAILED)
		{
			printf("�ȴ���¼����Ӧ��ͨ�Ź��ϣ������¿�ʼ�¼��ȴ�����\n");
			continue;
		}
			//�������¼�������  
			WSANETWORKEVENTS wsanetwork;  
			auto s=socketArray[dwIndex-WSA_WAIT_EVENT_0];
			auto e=eventArray[dwIndex-WSA_WAIT_EVENT_0];

			auto ret = WSAEnumNetworkEvents(s,e,&wsanetwork);

			if(ret==SOCKET_ERROR)//��������ʧ�ܡ�  
			{  
				break;  
			}
			if(wsanetwork.lNetworkEvents&FD_READ)
			{
				// ���յ�¼Ӧ��
				auto nRet = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<SOCKADDR *>(&sAddr), &nRecv);

				if(0 < nRet)
				{
					buf[nRet] = 0;
					CString strMsg = buf;
					// ���������
					if(0 == memcmp(buf, "MESSAGE", 4))
					{
						// ����Ӧ��
						CString strAnswer;
						const char resFormat[] = "SIP/2.0 200 OK\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\nContent-Length: 0\r\n\r\n";

						// ȡ��Via��
						CString strVia;
						CSipParser::get_line("Via", strMsg, strVia);

						// ȡ��From��
						CString strFrom;
						CSipParser::get_line("From", strMsg, strFrom);

						// ȡ��To�� 
						CString strTo;
						CSipParser::get_line("To", strMsg, strTo);

						// ȡ��Call-ID��
						CString strCallID;
						CSipParser::get_line("Call-ID", strMsg, strCallID);

						// ȡ��CSeq��
						CString strCSeq;
						CSipParser::get_line("CSeq", strMsg, strCSeq);

						strAnswer.Format(resFormat,  strVia, strFrom, strTo, strCallID, strCSeq);
						sendto(sock, strAnswer.GetString(), strAnswer.GetLength(), 0, reinterpret_cast<SOCKADDR *>(&sAddr), sizeof(sAddr));
					}
				}				

			}
		
	}

	return 0;
}

UINT AFX_CDECL CTask::pfnSendProc( LPVOID lParam )
{
	auto pModule = reinterpret_cast<CTask *>(lParam);
	auto nMaxPerSec = pModule->nMaxPerSec;
	auto nMaxCount = pModule->nMaxCount;
	//auto nSent = 0;
	//auto nRecv = 0;
	auto sAddr = pModule->m_tSockAddr;
	SOCKET			sock;

	char			CALL_ID[64];
	char			TAG[128];
	char			BRANCH[128];
	//char			RTAG[128];
	//char			buf[4096];
	LARGE_INTEGER	liPerfFreq	= {0};
	LARGE_INTEGER	liPerfCur	= {0};
	auto nOweSum		= 0;
	auto msSum		= nMaxPerSec / 1000.0;				//ÿ���뷢�͵ĸ���
	auto avgSum		= 1000.0 / nMaxPerSec;				//ÿ����ʱ ����
	CString			strQueryMsg;
	CString			strQueryHead;
	CString			strQueryBody;

	sock = pModule->m_sock;

	// ��ʱ��ʼ
	QueryPerformanceFrequency(&liPerfFreq); 

	// ���뷢��ѭ��
	for (auto i = 0; i < nMaxCount; i++)
	{
		QueryPerformanceCounter(&liPerfCur);

		// ��ʼ��ͨ�Ų���
		CSipParser::gen_call_id(CALL_ID);
		CSipParser::gen_tag(TAG);
		CSipParser::gen_branch(BRANCH);
		//printf(CALL_ID);printf("\n");
		//printf(TAG);printf("\n");
		//printf(BRANCH);printf("\n\n");

		auto nStartTime = (liPerfCur.QuadPart * 10000 / liPerfFreq.QuadPart);

		// ��ʼ������������Ϣ
		strQueryBody.Format(pModule->m_strHead, i);
		strQueryHead.Format(pModule->m_strBody, BRANCH, TAG, CALL_ID, i, strQueryBody.GetLength());

		strQueryMsg = strQueryHead + strQueryBody;

		// ������Ϣ
		sendto(sock, strQueryMsg.GetString(), strQueryMsg.GetLength(), 0, reinterpret_cast<const SOCKADDR *>(&sAddr), sizeof(sAddr));

		QueryPerformanceCounter(&liPerfCur);
		auto nEndTime = (liPerfCur.QuadPart * 10000 / liPerfFreq.QuadPart);

		if(0 == nOweSum && avgSum*10 > nEndTime - nStartTime)
		{
			// ��Ҫ����ʱ��
			auto dbTime = (avgSum*10-(nEndTime - nStartTime)) / 10.0;
			auto nTime = static_cast<int>(dbTime + 0.5);
			Sleep(nTime);

			QueryPerformanceCounter(&liPerfCur);
			auto nCurTime = (liPerfCur.QuadPart* 10000 / liPerfFreq.QuadPart);

			// Sleep(1)ʵ������ʱ��ᳬ��1����
			// ȡ��ʵ�ʵ�����ʱ�䣬�����ʱ����Ӧ�ù����Ĵ���
			nOweSum = static_cast<int>((nCurTime - nEndTime - nTime) * msSum / 10.0);
		}
		else
		{
			nOweSum--;
			nOweSum = 0 > nOweSum ? 0 : nOweSum;

		}
	}

	return 0;
}

void CTask::ExitRecvProc()
{
	m_bIsExit = true;
}

