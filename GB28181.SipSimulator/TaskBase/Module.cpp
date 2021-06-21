#include "./../stdafx.h"
#include "Module.h"

// ģ�����߳�
UINT AFX_CDECL CModule::pfnMainCtrlProc(LPVOID lParam)
{
	auto			pModule = static_cast<CModule *>(lParam);
	auto			nThrds = pModule->m_vThreads.GetCount();
	auto			hEvents = static_cast<HANDLE *>(malloc(sizeof(HANDLE) * nThrds));
	DWORD			dWaitObject;
	DWORD			dWaitTm = MAKEWORD(1000, 0);

	// ��ʼ�������������
	for (auto i = 0; i < nThrds; i++)
	{
		hEvents[i] = static_cast<CWinThread *>(pModule->m_vThreads[i])->m_hThread;
	}

	// �������ѭ��
	while(!pModule->m_bIsExit)
	{
		// �������߳��źż�ģ�鴦��
		dWaitObject = WaitForMultipleObjects(nThrds, hEvents, FALSE, WSA_INFINITE);

		// �ȴ�ʧ�ܣ���ӡ������־
		if (dWaitObject == WAIT_FAILED)
		{
			// TODO
			printf("�ȴ��ȴ��¼�ʧ�ܣ����¿�ʼ�ȴ���\n");
			continue;
		}
		//TODO.....
		//���¼���Ҫ����...

		// �߳��˳�
		pModule->m_objNotice.SetEvent();
		break;
	}

	return 0;
}

CModule::CModule(void) : m_bIsExit(false),
	m_nLogIdx(0),m_pMainThread(nullptr),m_pMainCtrlProc(pfnMainCtrlProc)
{
	m_hNotice = m_objNotice.m_hObject;
}


CModule::~CModule(void)
{
	CProcDesc	* pDesc;

	// �ͷ��̺߳�������
	for (auto i = 0; i < static_cast<int>(m_vThreadProcs.GetCount()); i ++)
	{
		pDesc = reinterpret_cast<CProcDesc *>(m_vThreadProcs[i]);
		delete pDesc;
	}
}

// ���������߳�
void CModule::Startup()
{
	// �������й����߳�
	for (auto i = 0; i < static_cast<int>(m_vThreadProcs.GetCount()); i ++)
	{
		auto pDesc = reinterpret_cast<CProcDesc *>(m_vThreadProcs[i]);
		CWinThread	* pThread;
		for (auto j = 0; j < pDesc->count; j ++)
		{
			// �����߳�
			pThread = AfxBeginThread(pDesc->pfn, pDesc->lParam,	0, 0, 0, nullptr);
			m_vThreads.Add(pThread);

		}
	}

	// ���������߳�
	m_pMainThread = AfxBeginThread(m_pMainCtrlProc, static_cast<LPVOID>(this), 0, 0, 0, nullptr);
}

// ע�Ṥ���̺߳���
void CModule::RegisterProc( THREAD_PROC pfn, LPVOID lParam, int count )
{
	CProcDesc	 * pDesc = new CProcDesc();

	pDesc->pfn = pfn;
	pDesc->lParam = lParam;
	pDesc->count =count;

	m_vThreadProcs.Add(reinterpret_cast<void *>(pDesc));
}

// ע�������̺߳���
void CModule::RegisterMainProc( THREAD_PROC pfn )
{
	m_pMainCtrlProc = pfn;
}
