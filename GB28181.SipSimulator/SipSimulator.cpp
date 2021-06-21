// SipSimulator.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "MainThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

// Ψһ��Ӧ�ó������

//CWinApp theApp;



int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	auto nRetCode = 0;
	CWinThread	* p_mt;
	DWORD		dwWaitObject;
	WSADATA		wsaData;

	auto hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// ��ʼ�� MFC ����ʧ��ʱ��ʾ����
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: ���Ĵ�������Է���������Ҫ
			_tprintf(_T("����: MFC ��ʼ��ʧ��\n"));
			nRetCode = 1;
		}
		else
		{
			// ��ʼ��winsock2.2
			WSAStartup(MAKEWORD(2,2), &wsaData);

			// ���������߳�
			p_mt = AfxBeginThread(pfnMainThreadProc, nullptr);
			if (p_mt == nullptr)
			{
				_tprintf(_T("���󣺴������߳�ʧ��\n"));
				nRetCode = 1;
			}
			else
			{
				// �ȴ������߳��˳�
			dwWaitObject=  WaitForSingleObject(p_mt->m_hThread, INFINITE);
			if(dwWaitObject== WAIT_FAILED)
			{
				//TODO...

			}
			//Exit App...
			WSACleanup();
			
			}
		}
	}
	else
	{
		// TODO: ���Ĵ�������Է���������Ҫ
		_tprintf(_T("����: GetModuleHandle ʧ��\n"));
		nRetCode = 1;
	}

	return nRetCode;
}
