#include "stdafx.h"
#include "CatalogLayer.h"
#include "./log/Log.h"
#include "Main/MainThread.h"
#include "Common/common.h"
#include "Main/UnifiedMessage.h"

CCatalogLayer::CCatalogLayer()
{
}

CCatalogLayer::~CCatalogLayer()
{
}

// �����豸ID����Ŀ¼�㼶�ṹ
int CCatalogLayer::AddDevice(const char *pszDeviceID)
{
	char szProvince[3];
	char szCity[5];
	char szDistrict[7];
	char szStation[9];
	CCatalogList *pList = nullptr;
	CCatalogList *pListTmp = nullptr;
	int nLayer = 0;
	int nGrade = 0;
	int nDeviceGrade = 0;

	m_oLock.Lock();
	// �豸�Ѿ����
	if (TRUE == m_oDeviceQueue.Lookup(pszDeviceID, nGrade))
	{
		CLog::Log(DEVINFO, LL_NORMAL, "�ظ����Ŀ¼�㼶�ṹ �豸ID;%s �豸����:%d", pszDeviceID, nGrade);
		nLayer = -1;
		goto error;
	}

	Utils::StringCpy_s(szProvince, 3, pszDeviceID);
	Utils::StringCpy_s(szCity, 5, pszDeviceID);
	Utils::StringCpy_s(szDistrict, 7, pszDeviceID);
	Utils::StringCpy_s(szStation, 9, pszDeviceID);

	// ��ӵ��豸����
	m_oDeviceQueue.SetAt(pszDeviceID, nDeviceGrade);

	// ʡ
	if (FALSE == m_oProvinceCount.Lookup(szProvince, pList))
	{
		pList = new CCatalogList;
		m_oProvinceCount.SetAt(szProvince, pList);
		nLayer++;
	}
	pListTmp = pList;
	// ��
	if (FALSE == m_oCityCount.Lookup(szCity, pList))
	{
		pList = new CCatalogList;
		pListTmp->SetAt(szCity, nLayer);
		m_oCityCount.SetAt(szCity, pList);
		nLayer++;
	}
	pListTmp = pList;
	// ��/��
	if (FALSE == m_oDistrictCount.Lookup(szDistrict, pList))
	{
		pList = new CCatalogList;
		pListTmp->SetAt(szDistrict, nLayer);
		m_oDistrictCount.SetAt(szDistrict, pList);
		nLayer++;
	}
	pListTmp = pList;
	// ���㵥λ
	if (FALSE == m_oStationCount.Lookup(szStation, pList))
	{
		pList = new CCatalogList;
		pListTmp->SetAt(szStation, nLayer);
		m_oStationCount.SetAt(szStation, pList);
		nLayer++;
	}
	pList->SetAt(pszDeviceID, nLayer);
error:
	m_oLock.Unlock();
	return nLayer;
}

// ɾ���豸���豸��Ӧ��Ŀ¼�㼶�ṹ
int CCatalogLayer::DelDevice(const char *pszDeviceID)
{
	char szProvince[3];
	char szCity[5];
	char szDistrict[7];
	char szStation[9];
	int nCount = 0;
	int nlayer = 0;
	CCatalogList *pList = nullptr;
	m_oLock.Lock();
	if (FALSE == m_oDeviceQueue.Lookup(pszDeviceID, nCount))
	{
		CLog::Log(DEVINFO, LL_NORMAL, "��ɾ����Ŀ¼������ Ŀ¼ID;%s", pszDeviceID);
		nlayer = -1;
		goto error;
	}

	m_oDeviceQueue.RemoveKey(pszDeviceID);

	Utils::StringCpy_s(szProvince, 3, pszDeviceID);
	Utils::StringCpy_s(szCity, 5, pszDeviceID);
	Utils::StringCpy_s(szDistrict, 7, pszDeviceID);
	Utils::StringCpy_s(szStation, 9, pszDeviceID);

	// ���㵥λ
	if (TRUE == m_oStationCount.Lookup(szStation, pList))
	{
		pList->RemoveKey(pszDeviceID);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oStationCount.RemoveKey(szStation);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -5;
		goto error;
	}

	// ��/��
	if (TRUE == m_oDistrictCount.Lookup(szDistrict, pList))
	{
		pList->RemoveKey(szStation);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oDistrictCount.RemoveKey(szDistrict);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -4;
		goto error;
	}

	// ��
	if (TRUE == m_oCityCount.Lookup(szCity, pList))
	{
		pList->RemoveKey(szDistrict);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oCityCount.RemoveKey(szCity);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -3;
		goto error;
	}

	// ʡ
	if (TRUE == m_oProvinceCount.Lookup(szProvince, pList))
	{
		pList->RemoveKey(szCity);
		if (!pList->IsEmpty())
			return nlayer;
		else
		{
			m_oProvinceCount.RemoveKey(szProvince);
			delete pList;
		}
		nlayer++;
	}
	else
	{
		nlayer = -2;
		goto error;
	}
error:
	m_oLock.Unlock();
	return nlayer;
}

// ��ѯĿ¼�㼶
int CCatalogLayer::FindDeviceLayer(const char *pszCivilCode, const char *pszGWID, CivilList &oCivilList)
{
	CString			strProvinceID;
	CCatalogList*	pTmp = nullptr;

	m_oLock.Lock();
	if (ID_LEN == strlen(pszCivilCode))
	{
		// ƽ̨Ŀ¼
		if (0 == _stricmp(pszGWID, pszCivilCode))
		{
			// ��������ʡ��
			POSITION pos = m_oProvinceCount.GetStartPos();
			while (pos)
			{
				m_oProvinceCount.GetNext(pos, strProvinceID, pTmp);
				oCivilList.AddTail(strProvinceID);
				ErgodicRead(strProvinceID, oCivilList);
			}
		}
		else
		{
			int nTmp;
			if (TRUE == m_oDeviceQueue.Lookup(pszCivilCode, nTmp))
				oCivilList.AddTail(pszCivilCode);
		}
	}
	else
	{
		//oCivilList.AddTail(pszCivilCode);
		ErgodicRead(pszCivilCode, oCivilList);
	}
	m_oLock.Unlock();

	return 0;
}

// ������ȡ���ṹΪ���в㼶��ϵ���б�
// 1.ʡID
// 2.	��ID
// 3.     ����ID
// 4.		  �ɳ���ID
// 5.			  �豸ID
// 6.	  	      �豸ID
// 7.		  �ɳ���ID
// 8.			  �豸ID
// 10.	  ����ID
void CCatalogLayer::ErgodicRead(const char *pszCivilCode, CivilList &oCivilList)
{
	CounterMap *pCivilMaps[4] = { &m_oProvinceCount, &m_oCityCount, &m_oDistrictCount, &m_oStationCount };
	int				nLayer = strlen(pszCivilCode) / 2 - 1;
	CCatalogList*	pList;
	CString			strDeviceID;

	if (0 > nLayer || 3 < nLayer)
		return;

	if (TRUE == pCivilMaps[nLayer]->Lookup(pszCivilCode, pList))
	{
		POSITION pos = pList->GetStartPosition();
		int nGrade = 0;
		CString	strRight2;
		while (pos)
		{
			pList->GetNextAssoc(pos, strDeviceID, nGrade);
			// ĩβ��00��Ҳ�����ţ�
//			if(0 != strDeviceID.Right(2).Compare("00"))
			oCivilList.AddTail(strDeviceID);

			// ��ǰ��ѯIDΪ�豸ID���ѵ�����ײ㣬�ݹ����
			if (3 > nLayer)
			{
				ErgodicRead(strDeviceID, oCivilList);
			}
		}
	}
}