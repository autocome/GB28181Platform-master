// Main\MainThread.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MainThread.h"
#include "Module.h"
#include "SipTask/InviteTask.h"
#include "LoginHandler.h"
#include "SipTask/QueryTask.h"

TASK_DESC	task_array[] = {
							{DEVICE_INFO_QUERY, "Ŀ¼��ѯ"},
							{PLAYBACK, "�طŲ���"},
							{PLAYBACK_CTRL, "�طſ��Ʋ���"},
							{PLAY_VIDEO, "���Ų���"}
							};

static TASKID InitTask();

// MainThread
UINT AFX_CDECL pfnMainThreadProc(LPVOID lParam)
{
	TASKID				tid;
	CModule				* pModule;
	HANDLE				hEvents[1];
	DWORD				dWaitObject;

	//auto				bSet	= false;
	char				ip[32]	= "";
	int					port;
	CLoginHandler		loginHandler;
	SOCKADDR_IN			tSockAddr;
	SOCKET				sock;

	// ����socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("�޷�����socket���߳��˳�\n");
		return -1;
	}
	printf("����IP��");
	scanf_s("%s", ip, 32);
	//strcpy_s(ip, 32, "10.10.124.174");
	printf("\n");
	printf("���ض˿ڣ�50601");
	//scanf_s("%d", &port);
	port = 50601;
	printf("\n");

	// ���ñ���IP�Ͷ˿�
	tSockAddr.sin_family = AF_INET;
	tSockAddr.sin_addr.S_un.S_addr = inet_addr(ip);
	tSockAddr.sin_port = htons(port);
	if(0 != bind(sock, reinterpret_cast<struct sockaddr*>(&tSockAddr), sizeof(tSockAddr)))
	{
		DWORD dwErr = WSAGetLastError();
		return dwErr;
	}
	//printf("�ȴ��ͻ��˵�½����\n\n");
	//loginHandler.Init(sock);
	//loginHandler.WaitLogin(tSockAddr);
	//printf("�ͻ��˵�½�ɹ�\n\n");

	// ���������߳�
	CTask::CreateRecvThread(sock);
	printf("�����߳�����\n\n");

	// ����Զ��IP�Ͷ˿�
	tSockAddr.sin_family = AF_INET;
	tSockAddr.sin_addr.S_un.S_addr = inet_addr("10.10.124.75");
	tSockAddr.sin_port = htons(50601);

	// �����������ѭ��
	while (1)
	{
		// �û���������
		tid = InitTask();
		if (tid < DEVICE_INFO_QUERY)
		{
			// �û�ѡ���˳�
			return 0;
		}

		//// ���û����
		//if (!bSet && tid != SETIPPORT)
		//{
		//	printf("��������Ŀ��IP�Ͷ˿ڣ�\n\n");
		//	continue;
		//}

		// ��������ģ��
		switch (tid)
		{
		case DEVICE_INFO_QUERY:
			pModule = new CQueryTask(sock, tSockAddr);
			break;
		case PLAYBACK:
			pModule = new CInviteTask(sock, tSockAddr);
			break;
		case PLAYBACK_CTRL:
			continue;
		case PLAY_VIDEO:
			pModule= new CInviteTask(sock,tSockAddr);
			break;
		//case SETIPPORT:
		//	bSet = true;
		//	printf("����IP��");
		//	scanf_s("%s", ip);
		//	printf("\n");
		//	printf("���ض˿ڣ�");
		//	scanf_s("%d", &port);
		//	printf("\n");
		//	break;
		default:
			return 0;
		}

		// ��������ģ��
		pModule->Init();
		pModule->Startup();

		// �������
		hEvents[0] = pModule->m_hNotice;
		dWaitObject = WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, FALSE, INFINITE);

		// ����ģ����Ϣ
		// ģ���ڲ�Ӧ���г��Իָ�����
		// ��һ��ģ���˳�������Ϊ���󲻿ɻָ�
		// ����֪ͨ����ģ���˳�
		if (dWaitObject == WAIT_FAILED)
		{
			break;
		}

		// �ͷŹ���ģ�� 
		pModule->Cleanup();
		delete pModule;
	}
	closesocket(sock);
	return 0;
}

TASKID InitTask()
{
	TASKID	n;

	// ��ʾ�˵�
	printf("----------------------------------------\n\n");
	printf("����ƽ̨ģ����\n");
	printf("----------------------------------------\n\n");
	printf("��ѡ���������\n\n");
	for (auto i = 0; i < ARRAYSIZE(task_array); i++)
	{
		printf("\t%d:\t%s\n", task_array[i].id, task_array[i].name);
	}
	//printf("\t9:\t���ñ���IP�Ͷ˿�\n");
	printf("\t0:\t�˳�\n");

	// ��ȡ����
	printf("������룺");
	scanf_s("%d", &n);
	printf("\n");

	return n;
}
