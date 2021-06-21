#include "stdafx.h"
#include "SubscribeList.h"
#include "Main/MainThread.h"
#include "Common/common.h"
#include "Main/UnifiedMessage.h"
#include "Log/Log.h"


CSubscribeList::CSubscribeList()
{
	m_strSysSubInfo.did = 0;
	m_strSysSubInfo.exp = 0;
}

CSubscribeList::~CSubscribeList()
{}

void CSubscribeList::Init(const char *pszGWID)
{
	m_strSysSubscribe = pszGWID;
}

// ��Ӷ��ķ�Χ������������ϵͳ��
void CSubscribeList::Add(const char *pszDeviceID, int nExp, int nDID)
{
	time_t tmCur;
	time(&tmCur);

	// ��������ϵͳĿ¼
	if (0 == m_strSysSubscribe.Compare(pszDeviceID))
	{
		m_strSysSubscribe = pszDeviceID;
		m_strSysSubInfo.did = nDID;
		m_strSysSubInfo.exp = nExp + tmCur;
		return;
	}

	SubInfo_t  subInfo;
	subInfo.exp = nExp + tmCur;
	subInfo.did = nDID;
	m_oSubscribeMap.SetAt(pszDeviceID, subInfo);
}
// ��ѯ��ǰ�豸�Ƿ����ڱ����ķ�Χ
// OUT pzsSubscribeID�����ķ�Χ��ϵͳID����������
int CSubscribeList::Find(const char *pszDeviceID, char *pzsSubscribeID, int &nDID)
{
	char tmp[ID_BUF_LEN];
	time_t tmCur;
	time(&tmCur);

	// ϵͳ���ģ������豸����Ч
	if (0 < m_strSysSubInfo.exp)
	{
		Utils::StringCpy_s(pzsSubscribeID, ID_BUF_LEN, m_strSysSubscribe.GetString());
		nDID = m_strSysSubInfo.did;
		// �ж�ϵͳ�����Ƿ����
		if (tmCur < m_strSysSubInfo.exp)
		{
			return 0;
		}
		else
		{
			Del(pzsSubscribeID);
			return 1;
		}
	}

	// ʡ���С���/�ء����㵥λ��4��Ŀ¼, ���һ�����豸ID�ж�
	for (int i = 1; i < 6; i++)
	{
		if (i < 5)
			Utils::StringCpy_s(tmp, i * 2 + 1, pszDeviceID);
		else
			Utils::StringCpy_s(tmp, ID_BUF_LEN, pszDeviceID);

		SubInfo_t subInfo;
		if (TRUE == m_oSubscribeMap.Lookup(tmp, subInfo, FALSE))
		{
			nDID = subInfo.did;
			Utils::StringCpy_s(pzsSubscribeID, ID_BUF_LEN, tmp);
			// ����δ����
			if (tmCur < subInfo.exp)
			{
				return 0;
			}
			else
			{
				CLog::Log(SIPCOM, LL_NORMAL, "[%s] Subscribe Expired", tmp);
				Del(tmp);
				return 1;
			}
		}
	}

	return -1;
}

// �Ӷ����б�ɾ��
int CSubscribeList::Del(const char *pszDeviceID)
{
	return m_oSubscribeMap.RemoveKey(pszDeviceID);
}