#pragma once
class CTask
{
public:
	CTask(SOCKET sock, SOCKADDR_IN tSockAddr);
	virtual ~CTask(void);

	void InitParam();

	static void CreateRecvThread(SOCKET sock);
	static UINT AFX_CDECL pfnRecvProc( LPVOID lParam );
	static UINT AFX_CDECL pfnSendProc( LPVOID lParam );
	static void ExitRecvProc();
protected:
	int		nConc;			// �����߳���
	int		nMaxPerSec;		// ���߳�ÿ������͵�¼/ע�������������޷��ﵽ��
	int		nMaxCount;		// �ܹ����Ͷ��ٴ������ĵ�¼/ע������
	CString	m_strHead;		// SIP��ͷ��ʽ����
	CString	m_strBody;		// SIP�����ʽ����

	SOCKET			m_sock;
	SOCKADDR_IN		m_tSockAddr;
	static bool		m_bIsExit;
};

