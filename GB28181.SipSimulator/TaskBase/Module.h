#pragma once
#include <afxmt.h>

class CModule : public CObject
{
public:
	CModule(void);
	~CModule(void);

	virtual void Init() = 0;
	virtual void Cleanup() = 0;

	virtual void Startup();

	virtual const char * GetModuleID()
	{
		static const char	m_szModule[20] = "MODULE";
		return reinterpret_cast<const char *>(m_szModule);
	};

	HANDLE	m_hNotice;
	bool	m_bIsExit;			// ֪ͨģ�������߳��˳���־λ
protected:
	typedef UINT (*THREAD_PROC)(LPVOID);
	class CProcDesc
	{
	public:
		THREAD_PROC		pfn;
		LPVOID			lParam;
		int				count;
	} ;

	CEvent			m_objNotice;		// ����֪ͨģ���ܿ��߳�ģ�鴦���˳�
	int				m_nLogIdx;			// ģ����־��
	CWinThread		* m_pMainThread;	// ģ�������̶߳���
	THREAD_PROC		m_pMainCtrlProc;	// ģ�������̺߳���ָ��
	CPtrArray		m_vThreads;			// �洢���й����̵߳Ķ���ָ��
	CPtrArray		m_vThreadProcs;		// �洢���й����̵߳�ִ�к�����Ϣ

	// ע�Ṥ���̺߳���
	void RegisterProc(THREAD_PROC pfn, LPVOID lParam, int count);

	// ע�������̺߳���
	void RegisterMainProc(THREAD_PROC pfn);

	// ģ�������߳�ģ��
	static UINT AFX_CDECL pfnMainCtrlProc(LPVOID lParam);
};