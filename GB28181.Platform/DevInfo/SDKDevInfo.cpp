#include "StdAfx.h"
#include "SDKDevInfo.h"
#include "Main/MainThread.h"
#include "ServerConsole.h"
#include "Common/Utils.h"
#include "DataManager/DataStore.h"
#include "SDKCom.h"

CCatalog  g_testAlarmCatalog;
bool      g_addAlarmFlag;
//��ʼ��
CSDKDevInfo * CSDKDevInfo::m_pCSDKDevInfo = nullptr;

CSDKDevInfo::CSDKDevInfo(void)
	:
	m_pAdaptorFactory(nullptr),
	m_ptrSynAdapter(nullptr),
	m_pSynClient(nullptr)
{
	m_pCSDKDevInfo = this;
	auto m_Gatewayid = appGlobal_ConfInfo.m_LocalPlatform.str_ID;
	m_GBID_Creater.initial(m_Gatewayid.GetString());
}

CSDKDevInfo::~CSDKDevInfo(void)
{
	if (m_pAdaptorFactory)
		m_pAdaptorFactory->Release();

	if (m_pSynClient)
		m_pSynClient->Release();
}

int CSDKDevInfo::Init(_Factory *&pAdaptorFactory, _SynClient *&pSynClient)
{
	pAdaptorFactory = nullptr;
	pSynClient = nullptr;

	if (FALSE == ReadDeviceTypeDef())
		return -1;

	if (FALSE == InitSynClient())
		return -1;

	pAdaptorFactory = m_pAdaptorFactory;
	pSynClient = m_pSynClient;
	m_oDecoderPairMap.SetExpiry(10);
	return 0;
}

void CSDKDevInfo::Cleanup()
{
	HUSDeviceLinkInfo *pChannelInfo = nullptr;

	auto pos = m_oDecoderMap.GetStartPos();
	while (pos)
	{
		CString strID;
		m_oDecoderMap.GetNext(pos, strID, pChannelInfo);
		if (pChannelInfo)
		{
			delete pChannelInfo;
			pChannelInfo = nullptr;
		}
	}

	if (m_pAdaptorFactory)
		m_pAdaptorFactory->Release();

	if (m_pSynClient)
		m_pSynClient->Release();
}

BOOL CSDKDevInfo::InitSynClient()
{
	CLog::Log(SDKCOM, LL_NORMAL, "����VSM����");

	_AdaptorFactoryWrapperPtr pAdaptorFactoryWrapper;

	HRESULT hr;
	if (SUCCEEDED(hr = pAdaptorFactoryWrapper.CreateInstance(__uuidof(AdaptorFactoryWrapper))))
	{
		m_pAdaptorFactory = pAdaptorFactoryWrapper->GetAdaptorFactoryInstance();
	}
	else
	{
		if (FAILED(hr = CoCreateInstance(CLSID_Factory, nullptr, CLSCTX_INPROC_SERVER, IID__Factory, reinterpret_cast<LPVOID*>(&m_pAdaptorFactory))))
		{
			CLog::Log(SDKCOM, LL_NORMAL, "_Factory�ӿڴ���ʧ�ܡ�");
			return FALSE;
		}
	}

	// ����SynClient
	if (FAILED(hr = CoCreateInstance(CLSID_SynClient, nullptr, CLSCTX_INPROC_SERVER, IID__SynClient, reinterpret_cast<LPVOID*>(&m_pSynClient))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "_SynClient�ӿڴ���ʧ�ܡ�");
		return FALSE;
	}
	// �ڵ���SynClient����������֮ǰ�������÷������ͣ����GB28181 Gateway����Ҫ����һ���µ��ַ������硰GB28181GW����
	//Ŀǰ���䶨��Ϊ��"GBGateway"
	CString strServiceTag = "GBGateway";

	m_pSynClient->PutServerType(strServiceTag.GetString());
	m_guidGateway = m_pSynClient->GetConfigID(strServiceTag.GetString());
	m_pAdaptorFactory->SiteAddress = m_pSynClient->GetSiteIP();
	// ��ʼ��SynClient���贫��Gateway��GUID
	// ��ʼ�����̻��VMSվ���ȡ�豸����д��SiteTree�ļ���C:\Program Files (x86)\Common Files\Honeywell\HUS\SynchronizeFiles\...xml�����ȽϺ�ʱ

	if (!m_pSynClient->Initialize(m_guidGateway))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "_SynClient��ʼ��ʧ�ܡ���");
		return FALSE;
	}

	// �õ�SiteImageAdaptor����
	m_ptrSynAdapter = m_pSynClient->SiteImageAdaptor;
	_bstr_t strProperties = m_ptrSynAdapter->GetDeviceProperties();
	m_pAdaptorFactory->DeviceTypeFromFile = FALSE;
	m_pAdaptorFactory->DeviceTypeData = strProperties;

	//	pSynAdapter->QueryInterface(IID_IDeviceConfig, (LPVOID*)&pDeviceConfig);
	//	pSynAdapter = pDeviceConfig;
	// ��ȡ�豸�б�������Gateway��GUID������ֵ���豸����
	// ���ﷵ�ص��豸�б�������йҽ��ڵ�ǰGateway�µ��豸
	SAFEARRAY* arDevices = m_ptrSynAdapter->GetLinks(m_guidGateway);
	_ECElementPtr *arECElements = nullptr;

	if (FAILED(SafeArrayAccessData(arDevices, reinterpret_cast<void**>(&arECElements))))
	{
		// ...
		CLog::Log(SDKCOM, LL_NORMAL, "SafeArrayAccessDataʧ�ܡ�");
		return FALSE;;
	}

	CLog::Log(SDKCOM, LL_NORMAL, "��ȡ%d���豸�������Ϣ����", arDevices->rgsabound[0].cElements);
	for (UINT i = 0; i < arDevices->rgsabound[0].cElements; i++)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "�����豸[%d]", i);
		IniDeviveProperty(arECElements[i]);
	}

	SafeArrayUnaccessData(arDevices);
	SafeArrayDestroy(arDevices);
	CLog::Log(SDKCOM, LL_NORMAL, "��Ϣ��ȡ��ɡ�");
	return TRUE;
}

BOOL CSDKDevInfo::ChannelLookup(const char *pszKey, DeviceObject_t & husdeviceInfo)
{
	DeviceObject_t husDevceInfo;
	if (CDataStore::LookupHUSDeviceByDeviceID(pszKey, husDevceInfo, TRUE))
	{
		husdeviceInfo = husDevceInfo;
		return TRUE;
	}
	return FALSE;
	//return m_oChannelGBIDMap.Lookup(pszKey, pInfo, TRUE);
}

void CSDKDevInfo::ChannelLookupEnd()
{
	CDataStore::ManualUnlockHUSDeviceInfoMap();
	//m_oChannelGBIDMap.ManualUnlock();
}

BOOL CSDKDevInfo::DecoderLookup(const char *pszKey, HUSDeviceLinkInfo *pInfo)
{
	return m_oDecoderMap.Lookup(pszKey, pInfo, TRUE);
}

void CSDKDevInfo::DecoderLookupEnd()
{
	m_oDecoderMap.ManualUnlock();
}

BOOL CSDKDevInfo::DecoderPairRemove(const char *pszKey, DecoderPairInfo_t &pInfo)
{
	return m_oDecoderPairMap.RemoveKey(pszKey, pInfo);
}

void CSDKDevInfo::AlarmingReomve(const char *pszKey)
{
	if (nullptr == pszKey)
		m_oAlarmingDeviceMap.RemoveAll();
	else
		m_oAlarmingDeviceMap.RemoveKey(pszKey);
}

// ȡ���豸��Ӧ��EC��NVR��Ϣ
void CSDKDevInfo::SetOnlineStatusByGUID(const char *pszGUID, int isOffline)
{
	DeviceObject tInfo;
	CString strGUID = pszGUID;
	CString strID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	if (!CDataStore::LookupDeviceID(strGUID, strID)) return;
	DeviceObject deviceInfo;
	if (CDataStore::LookupHUSDevice(strGUID, deviceInfo, TRUE))
	{
		if (isOffline)
		{
			deviceInfo.extDevInfo.SetOnlineStatus(_T("OFFLINE"));
			INT evttype = EventNotify::UpdateType::ut_off;
			CDataStore::AddHUSDeviceObj(strGUID, deviceInfo);
			m_oStatusChangeMap.SetAt(strGUID, evttype);
			CLog::Log(SDKCOM, LL_DEBUG, "�豸���� GUID:%s", strID);
			p_oSDKCom->NoticeDevInfo(strGUID, EventNotify::UpdateType::ut_off, strID, ot_devinfo_subscribe_notify);
		}
		else
		{
			deviceInfo.extDevInfo.SetOnlineStatus(_T("ONLINE"));
			INT evttype = EventNotify::UpdateType::ut_on;
			CDataStore::AddHUSDeviceObj(strGUID, deviceInfo);
			m_oStatusChangeMap.SetAt(pszGUID, evttype);
			CLog::Log(SDKCOM, LL_DEBUG, "�豸���� GUID:%s", strID);
			p_oSDKCom->NoticeDevInfo(strGUID, EventNotify::UpdateType::ut_on, strID, ot_devinfo_subscribe_notify);
		}
	}
	else
		CLog::Log(SDKCOM, LL_NORMAL, _T("��GBID���豸״̬��� GUID:%s ��ǰ�豸�ϴ�״̬�루0δ����,1���ߣ�:%d"), strGUID, isOffline);

	CDataStore::ManualUnlockHUSDeviceInfoMap();
}

// ɾ���豸����
void CSDKDevInfo::DeleteDeviceObject(const char *pszGUID)
{
	CString deviceGBID;

	if (CDataStore::LookupDeviceID(pszGUID, deviceGBID, TRUE))
	{
		//�Ƴ��豸IDӳ���ϵ
		CDataStore::RemoveHUSDevice(pszGUID);
		CDataStore::RemoveDevice(deviceGBID);
	}
	else
	{
		CDataStore::ManualUnlockGUIDMap();
		CLog::Log(SDKCOM, LL_NORMAL, "ɾ���豸����ʧ�ܣ�δ֪��GUID��%s", pszGUID);
		return;
	}
	CDataStore::ManualUnlockGUIDMap();
	//ɾ��HUS�豸��Ϣ
	DeviceObject deviceToDelete;
	if (CDataStore::LookupHUSDevice(pszGUID, deviceToDelete, TRUE))
	{
		CDataStore::RemoveHUSDeviceObj(pszGUID);
	}
	else
	{
		CDataStore::ManualUnlockHUSDeviceInfoMap();
		CLog::Log(SDKCOM, LL_NORMAL, "ɾ���豸����ʧ�ܣ�δ֪��GUID��%s", pszGUID);
		return;
	}
	CDataStore::ManualUnlockHUSDeviceInfoMap();

	// ɾ�����豸
	if (deviceToDelete.eGBDevType == GBDeviceType::OT_DEVICE)
		while (0 < deviceToDelete.oSubDevGUIDList.GetCount())
		{
			CString strGUID;
			GUID guidSubDev = deviceToDelete.oSubDevGUIDList.RemoveHead();
			Utils::GUIDToCString(guidSubDev, strGUID);
			DeviceObject subDeviceToDelete;

			if (CDataStore::LookupHUSDevice(strGUID, subDeviceToDelete, TRUE))
			{
				CString strGUIDtoDelete;
				if (GBDeviceType::OT_CAMERA == subDeviceToDelete.eGBDevType)
				{
					if (CDataStore::LookupGUID(subDeviceToDelete.strDeviceID, strGUIDtoDelete, TRUE))
					{
						CDataStore::RemoveDevice(deviceToDelete.strDeviceID);
						CDataStore::RemoveHUSDevice(strGUIDtoDelete);
					}
					CDataStore::ManualUnlockDeviceIDMap();
				}
				else if (GBDeviceType::OT_ALARM == deviceToDelete.eGBDevType)
				{
					//m_oAlarmGBIDMap.RemoveKey(deviceToDelete.strDeviceID);
					CDataStore::RemoveDeviceAlarmStatus(deviceToDelete.strDeviceID);
				}
				else if (GBDeviceType::OT_DECODER == deviceToDelete.eGBDevType)
				{
					HUSDeviceLinkInfo_t* p_DeviceLinkInfo = nullptr;
					if (m_oDecoderMap.Lookup(deviceToDelete.strDeviceID, p_DeviceLinkInfo, TRUE))
					{
						m_oDecoderMap.RemoveKey(deviceToDelete.strDeviceID);
						delete p_DeviceLinkInfo;
						p_DeviceLinkInfo = nullptr;
					}
				}
			}
			CDataStore::ManualUnlockHUSDeviceInfoMap();
		}
	else
	{
		CLog::Log(SDKCOM, LL_NORMAL, "ɾ���豸ʱ����ͨ������ �豸��GUID��%s", pszGUID);
	}
}

// �����豸��Ϣ
void CSDKDevInfo::UpdateGBDeviceIDByGUID(const char *pszGUID)
{
	CLog::Log(SDKCOM, LL_NORMAL, "%s", __FUNCTION__);
	CString strConfigName = appGlobal_ConfInfo.m_strDevConfPath + _T("\\") + pszGUID;

	// ȡ�����µ�GBID
	CMap<CString, LPCSTR, CString, CString&> strDeviceIDMap;
	char szData[MAX_PATH];
	CString strDeviceType;
	CString strDeviceID;
	GetPrivateProfileString(_T("CATALOG"), _T("DeviceID"), _T("NaN"), szData, MAX_PATH, strConfigName);
	strDeviceID = szData;
	strDeviceIDMap.SetAt(pszGUID, strDeviceID);
	//update Device RelationShip

	// ȡ���豸����
	GetPrivateProfileString(_T("INFO"), _T("DeviceType"), _T("DeviceType"), szData, MAX_PATH, strConfigName);
	strDeviceType = szData;

	// ȡ�����豸ID��GUID
	CString strSection;
	CString strChannelID;
	CString strGUID;
	for (int j = 0; j < MAX_CHANNEL; j++)
	{
		strSection.Format(_T("CHANNEL_CATALOG%d"), j);
		GetPrivateProfileString(strSection, _T("DeviceID"), _T("NaN"), szData, MAX_PATH, strConfigName);
		strChannelID = szData;

		GetPrivateProfileString(strSection, _T("GUID"), _T("NaN"), szData, MAX_PATH, strConfigName);
		// GUIDΪ�գ������ڸ�ͨ��
		if (0 == strcmp(szData, _T("NaN")))
			break;

		strGUID = szData;
		strGUID.MakeLower();
		if (0 == j && 0 == strDeviceType.CompareNoCase(_T("ipc")))
		{
			strDeviceIDMap.SetAt(strGUID, strDeviceID);
			continue;
		}
		strDeviceIDMap.SetAt(strGUID, strChannelID);
	}

	POSITION pos = strDeviceIDMap.GetStartPosition();
	while (pos)
	{
		strDeviceIDMap.GetNextAssoc(pos, strGUID, strDeviceID);
		DeviceObject tNewInfo, tOldInfo;
		if (!CDataStore::LookupHUSDevice(strGUID, tOldInfo, TRUE))
		{
			CDataStore::ManualUnlockHUSDeviceInfoMap();
			CLog::Log(SDKCOM, LL_NORMAL, "�����豸��Ϣʧ�ܣ�δ֪��GUID:%s", strGUID);
			continue;
		}
		tNewInfo = tOldInfo;
		tNewInfo.strDeviceID = strDeviceID;
		CDataStore::AddHUSDeviceObj(strGUID, tNewInfo, FALSE);
		CDataStore::ManualUnlockHUSDeviceInfoMap();
		Utils::StringCpy_s(szData, MAX_PATH, strDeviceID);
		CString	oldstrDeviceID = tOldInfo.strDeviceID; //get old deviceID to delete
		GBDeviceType	eGBDevType = tOldInfo.eGBDevType;

		// GBIDû�и��£������滻
		if (0 == oldstrDeviceID.Compare(szData))
		{
			continue;
		}

		// û����Ч��GBID
		if (ID_LEN != oldstrDeviceID.GetLength())
		{
			// ��GUID��key��ѯ
			oldstrDeviceID = strGUID;
		}

		// �滻ԭ����Ϣ
		HUSDeviceLinkInfo *deviceLinkInfo = nullptr;
		if (GBDeviceType::OT_DEVICE == eGBDevType || GBDeviceType::OT_CAMERA == eGBDevType)
		{
			CString husDeviceGUID;
			if (CDataStore::LookupGUID(oldstrDeviceID, husDeviceGUID, TRUE)) //����ӳ���ϵ
			{
				CDataStore::RemoveDevice(oldstrDeviceID, FALSE);
				CDataStore::AddDeviceID(strDeviceID, husDeviceGUID, FALSE);
			}
			CDataStore::ManualUnlockGUIDMap();
		}
		else if (GBDeviceType::OT_ALARM == eGBDevType)
		{
			AlarmInfo_t tServiceInfo;
			if (CDataStore::LookupDeviceAlarmStatus(oldstrDeviceID, tServiceInfo, TRUE))
			{
				CDataStore::RemoveDeviceAlarmStatus(oldstrDeviceID, FALSE);
				if (ID_LEN != strlen(szData))
					CDataStore::SetDeviceAlarmStatus(strGUID, tServiceInfo, FALSE);
				else
					CDataStore::SetDeviceAlarmStatus(szData, tServiceInfo, FALSE);
			}
			//m_oAlarmGBIDMap.ManualUnlock();
			//CDataStore::ManualUnlockDeviceIDMap();
			CDataStore::ManualUnlockDeviceInfoMap();
		}
		else if (GBDeviceType::OT_DECODER == eGBDevType)
		{
			if (m_oDecoderMap.Lookup(oldstrDeviceID, deviceLinkInfo, TRUE))
			{
				m_oDecoderMap.RemoveKey(oldstrDeviceID, FALSE);
				if (ID_LEN != strlen(szData))
					m_oDecoderMap.SetAt(strGUID, deviceLinkInfo, FALSE);
				else
					m_oDecoderMap.SetAt(szData, deviceLinkInfo, FALSE);
			}
			m_oDecoderMap.ManualUnlock();
		}
	}
}

void CSDKDevInfo::DecoderPairTimeout()
{
	m_oDecoderPairMap.CheckTimeOut();
}

DeviceChangedMap &CSDKDevInfo::GetStatusChangedMap()
{
	return m_oStatusChangeMap;
}

CSDKDevInfo::AlarmingDeviceMap &CSDKDevInfo::GetAlarming()
{
	return m_oAlarmingDeviceMap;
}

BOOL CSDKDevInfo::AlarmLookup(const char *pszKey, AlarmInfo_t &pInfo)
{
	return CDataStore::LookupDeviceAlarmStatus(pszKey, pInfo, TRUE);
}

void CSDKDevInfo::AlarmLookupEnd()
{
	//m_oAlarmGBIDMap.ManualUnlock();
	//CDataStore::ManualUnlockDeviceIDMap();
	CDataStore::ManualUnlockDeviceInfoMap();
}

void CSDKDevInfo::AlarmSetAt(const char *pszKey, AlarmInfo_t &pInfo, BOOL bIsLock)
{
	CDataStore::SetDeviceAlarmStatus(pszKey, pInfo, bIsLock);
}

BOOL CSDKDevInfo::DecoderIPLookup(const char *pszKey, CString &pszID)
{
	return m_oDecoderIPtoIDMap.Lookup(pszKey, pszID, FALSE);
}

void CSDKDevInfo::DecoderIPLookupEnd()
{
	m_oDecoderIPtoIDMap.ManualUnlock();
}

void CSDKDevInfo::DecoderPairPush(const char *pszKey, DecoderPairInfo_t &info)
{
	m_oDecoderPairMap.Push(pszKey, info);
}

BOOL CSDKDevInfo::DeviceLookup(const char *pszKey, DeviceObject_t *&pInfo)
{
	return CDataStore::LookupHUSDeviceByDeviceID(pszKey, *pInfo, TRUE);
}

void CSDKDevInfo::DeviceLookupEnd()
{
	CDataStore::ManualUnlockHUSDeviceInfoMap();
}

// ������ڱ������豸��Ϣ
int CSDKDevInfo::AddAlarmList(const char *pszGUID, const char *pszDescribe, const char *pszLevel  /*same with��strAlarmSeverity*/, const char *pszTime, int nAlarmType, int nAlarmMethord, int nAlarmStatus)
{
	if (0 == strlen(pszGUID))
		return -1;
	CLog::Log(SDKCOM, LL_DEBUG, _T("�豸�澯���豸GUID:%s �澯����:%s �澯��Ϣ:%s"), pszGUID, pszLevel, pszDescribe);
	CString			strDeviceGBID;
	DeviceObject	tInfo;
	CString strGUID = pszGUID;
	if ("{" != strGUID.Left(1))
		strGUID = "{" + strGUID + "}";
	// ȡ���豸��Ӧ��GBID
	CDataStore::LookupDeviceID(strGUID.GetString(), strDeviceGBID, FALSE);
	if (!strDeviceGBID.IsEmpty())
	{
		AlarmInfo_t		tAlarmInfo;
		if (CDataStore::LookupDeviceAlarmStatus(strDeviceGBID, tAlarmInfo, TRUE)) //�����豸
		{
			// �жϲ�����״̬
			if (AlarmInfo_t::DutyStatus::OFFDUTY == tAlarmInfo.eStatus)
			{
				//m_oAlarmGBIDMap.ManualUnlock();
				//CDataStore::ManualUnlockDeviceIDMap();
				CDataStore::ManualUnlockDeviceInfoMap();
				CLog::Log(SDKCOM, LL_DEBUG, _T("�豸�ѳ������澯���ϱ����豸ID��%s"), strDeviceGBID);
				return 0;
			}
			if (AlarmInfo_t::DutyStatus::ONDUTY == tAlarmInfo.eStatus)
			{
				tAlarmInfo.eStatus = AlarmInfo_t::DutyStatus::ALARM;
				tAlarmInfo.strDeviceGBID = strDeviceGBID;
				tAlarmInfo.m_nAlarmMethord = nAlarmMethord;
				tAlarmInfo.m_nAlarmType = nAlarmType;
				tAlarmInfo.m_nAlarmStatus = nAlarmStatus;
				tAlarmInfo.strDescribe = pszDescribe;
				tAlarmInfo.strLevel = pszLevel;
				tAlarmInfo.strTime = pszTime;
				//m_oAlarmGBIDMap.SetAt(strDeviceGBID, tAlarmInfo, FALSE);
				CDataStore::SetDeviceAlarmStatus(strDeviceGBID, tAlarmInfo, FALSE);
				CLog::Log(SDKCOM, LL_DEBUG, _T("�豸�Ѳ������澯�ϱ����豸ID��%s"), strDeviceGBID);
				this->p_oSDKCom->NoticeDevInfo(pszGUID, EventNotify::UpdateType::ut_alarm);
			}
		}
		else
		{
			CLog::Log(SDKCOM, LL_NORMAL, _T("�����ϱ�ʧ�ܣ�δ֪��GB28181�豸ID��%s"), strDeviceGBID);
			//m_oAlarmGBIDMap.ManualUnlock();
			//CDataStore::ManualUnlockDeviceIDMap();
			CDataStore::ManualUnlockDeviceInfoMap();
			return 0;
		}
		//m_oAlarmGBIDMap.ManualUnlock();
		//CDataStore::ManualUnlockDeviceIDMap();
		CDataStore::ManualUnlockDeviceInfoMap();
		// ��ӵ���������
		AlarmInfo_t alarmInfo(strDeviceGBID, nAlarmMethord, nAlarmType, nAlarmStatus, pszDescribe, pszLevel, pszTime);
		m_oAlarmingDeviceMap.SetAt(strDeviceGBID, alarmInfo);
	}
	return 0;
}

BOOL CSDKDevInfo::ReadDeviceTypeDef()
{
	CStdioFile oFile;
	if (FALSE == oFile.Open(appGlobal_ConfInfo.m_strTypeDefFileName, CStdioFile::modeRead))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "���Ͷ����ļ�%s��ʧ�ܣ�", appGlobal_ConfInfo.m_strTypeDefFileName);
		return FALSE;
	}

	CString strContent;
	while (TRUE == oFile.ReadString(strContent))
	{
		int nPos = strContent.Find(",");
		if (0 < nPos)
		{
			CString strFront = strContent.Left(nPos);
			strContent = strContent.Right(strContent.GetLength() - nPos - 1);
			strFront.MakeLower();
			m_oDeviceTypeDefMap.SetAt(strFront, strContent);
		}
	}
	return TRUE;
}

void CSDKDevInfo::IniDeviveProperty(_ECElementPtr ptrECElement)
{
	GUID		guidEC;
	GUID		guidDevice;
	CString		strPath;
	strPath = appGlobal_ConfInfo.m_strDevConfPath + _T("\\");
	// �����豸����
	guidDevice = ptrECElement->GetID();  // �豸��GUID
	_bstr_t strTypeMark = m_ptrSynAdapter->GetTypeMark(guidDevice);  // �豸���ͣ���"DVR", "Channel", "RStreamer"
	_bstr_t strName = ptrECElement->GetName();  // �豸����
	// ȡ���豸����GUID
	auto guidType = m_ptrSynAdapter->GetTypeID(guidDevice);
	CString strGUID;
	Utils::GUIDToCString(guidType, strGUID, FALSE);
	CString strType;
	// ȡ�ö�Ӧ��GB�豸���͵�
	if (m_oDeviceTypeDefMap.Lookup(strGUID, strType))
	{
		strTypeMark = strType;
	}
	// ��ȡ��ǰ�豸����Ӧ��EC�����GUID
	GUID guidTarget;
	guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(guidDevice, &guidTarget);

	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID, FALSE);
	m_oECToDevMap.SetAt(strDeviceGUID, guidEC);

	CLog::Log(SDKCOM, LL_NORMAL, _T("%s strGUID = %s strDeviceGUID = %s\r\n"), __FUNCTION__, strGUID, strDeviceGUID);

	// ȡ���豸��IP
	CString pszDevIP;
	GetSettingsParam(GUID_NULL, L"IP", pszDevIP, ptrECElement);
	// ȡ���豸��Port
	CString pszDevPort;
	GetSettingsParam(GUID_NULL, L"Port", pszDevPort, ptrECElement);

	CLog::Log(SDKCOM, LL_NORMAL, _T("%s strGUID = %s strDeviceGUID = %s pszDevIp = %s pszDevPort = %s\r\n"), __FUNCTION__, strGUID, strDeviceGUID, pszDevIP, pszDevPort);
	WriteDeviceInfoConfig(strPath.GetString(), strTypeMark, strName, guidDevice, guidEC, pszDevIP, pszDevPort);
}

BOOL CSDKDevInfo::GetSettingsParam(GUID guidDevice, WCHAR *pwszParamName, CString &strParmaValue, _ECElementPtr  p_EleSetting) const
{
	if (nullptr == m_ptrSynAdapter && p_EleSetting == nullptr)
		return FALSE;

	_ElementSettingsPtr pSetting = nullptr;

	if (p_EleSetting != nullptr&& guidDevice == GUID_NULL)
		pSetting = p_EleSetting->GetSettings();
	else
		pSetting = m_ptrSynAdapter->GetElementSettings(guidDevice);
	if (nullptr == pSetting)
		return FALSE;
	// ȡ�ò�������
	VARIANT varParam;
	TCHAR *pszValue;
	auto bstrParamName = SysAllocString(pwszParamName);
	pSetting->get_Item(bstrParamName, &varParam);
	SysFreeString(bstrParamName);

	pszValue = _com_util::ConvertBSTRToString(varParam.bstrVal);
	strParmaValue = pszValue;
	SAFE_DELETE_ARRAY(pszValue);
	return TRUE;
}

void CSDKDevInfo::AddToDataStore(DeviceObject & objec2add, DeviceObject* parentObject)
{
	if (parentObject != nullptr)
		parentObject->oSubDevGUIDList.AddTail(objec2add.guidDevice);
	CDataStore::AddHUSDeviceObj(objec2add.strGuidDevice, objec2add);
	CDataStore::AddGUID(objec2add.strGuidDevice, objec2add.strDeviceID);
}
void CSDKDevInfo::FillSubDevInfoByparent(DeviceObject & sub_device, const DeviceObject& parent_device)
{
	CString strDeviceGUID;
	Utils::GUIDToCString(sub_device.guidDevice, strDeviceGUID);
	sub_device.strGuidDevice = strDeviceGUID;
	sub_device.strIP = parent_device.strIP;
	sub_device.strPort = parent_device.strPort;
	sub_device.linkedInfo.strDeviceName = sub_device.strName; //����������Ϣ������Onwer
	sub_device.linkedInfo.guidDevice = sub_device.guidDevice;
	sub_device.linkedInfo.guidParent = parent_device.guidDevice;
	sub_device.linkedInfo.guidEC = parent_device.linkedInfo.guidEC;
	sub_device.linkedInfo.strDeviceName = sub_device.strName; //�������ӵ�Onwer
	sub_device.linkedInfo.guidDevice = sub_device.guidDevice;

}

void CSDKDevInfo::WriteDeviceInfoConfig(const char *pszConfigPath, _bstr_t &bstrType, _bstr_t &bstrName, const GUID &guidDevice, const GUID &guidEC, const char *pszDevIP, const char *pszDevPort)
{
	// ȡ���豸����
	CString strType = static_cast<char *>(bstrType);
	// ֻ�������豸��DVR��decoder,device(ipc,encoder)
	HUSDeviceType _husDevType = HUSDeviceType::DT_NULL;
	auto  _gb_deviceType = GetDeviceTypeByName(strType, _husDevType, TRUE);
	if (_gb_deviceType == OT_NONE)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "�������豸 Type:%s ,����������һ���豸", strType);
		return;
	}
	CString strDeviceName = static_cast<char *>(bstrName);
	CString strDeviceIP = pszDevIP;
	CString strDevicePort = pszDevPort;
	//ת��DVR�豸��GUIDΪ�ַ���
	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "�����豸��Ϣ ,�豸GUID:%s Type:%s", strDeviceGUID, strType);

	// ���ɱ����豸�ͱ���ͨ�������ݽṹ
	DeviceObject newdeviceInfo;
	newdeviceInfo.eGBDevType = _gb_deviceType; //��Ӧ��GB�豸����
	newdeviceInfo.guidDevice = guidDevice;
	newdeviceInfo.strGuidDevice = strDeviceGUID;
	newdeviceInfo.strName = strDeviceName;
	newdeviceInfo.strDeviceTyeMark = strType; //�ַ�����ǣ�ͬʱ����GB��HUS���豸����
	newdeviceInfo.strIP = strDeviceIP;
	newdeviceInfo.strPort = strDevicePort;
	newdeviceInfo.linkedInfo.guidDevice = guidDevice;
	newdeviceInfo.linkedInfo.strDeviceName = strDeviceName;
	newdeviceInfo.linkedInfo.guidParent = GUID_NULL;
	newdeviceInfo.linkedInfo.guidEC = guidEC;
	newdeviceInfo.extDevInfo.SetIOType(DT_DVR); //HUS��DVR�����

	CString strConfigName = pszConfigPath + strDeviceGUID;
	WriteDvrDeviceConfig(strConfigName, newdeviceInfo);
	//dvr�豸������

	_ECElementPtr	*arrSubElements = nullptr;
	auto sarrSubDevices = m_ptrSynAdapter->GetSubElementsArray(guidDevice);
	if (FAILED(SafeArrayAccessData(sarrSubDevices, reinterpret_cast<void**>(&arrSubElements))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "HUSDevice�� %s configPath = %s  Access SubChannel Info failure, but dvr Object init OK !\r\n", __FUNCTION__, strConfigName);

		return	AddToDataStore(newdeviceInfo);
	}
	auto channel_count = sarrSubDevices->rgsabound[0].cElements; //ͨ��������
	CLog::Log(SDKCOM, LL_NORMAL, "HUSDevice: %s configPath = %s ,Access SubChannel Info OK, SubChannel Count = %d\r\n", __FUNCTION__, pszConfigPath, channel_count);
	auto nElementIndex = 0;
	// �������豸
	for (auto j = 0UL; j < channel_count; j++)
	{
		auto			guid_Channel = arrSubElements[j]->GetID();
		CString			strChannelMark = static_cast<LPCTSTR>(m_ptrSynAdapter->GetTypeMark(guid_Channel));
		auto            bstr_ChannelName = arrSubElements[j]->GetName();
		auto            strChannelName = CString(static_cast<LPCTSTR>(bstr_ChannelName));
		CString			ChannelOrderID;
		//�ж��ǲ��Ǳ�������ͨ�����ǵĻ�����Ӧ����
		if (_gb_deviceType == OT_ENCODER)
		{
			GetSettingsParam(guid_Channel, L"Param", ChannelOrderID); 			//��ȡEV4ͨ�����
			auto channelorder = _ttoi(ChannelOrderID);
			if (ChannelOrderID.GetLength() != 1)
			{
				GetSettingsParam(guid_Channel, L"ChannelID", ChannelOrderID);	//��ȡE8X,E4Xͨ�����
				nElementIndex = channelorder;
			}
			else
				nElementIndex = channelorder - 1;
		}
		DeviceObject subChannelObject;
		subChannelObject.guidDevice = guid_Channel;
		subChannelObject.strName = strChannelName;
		subChannelObject.strDeviceTyeMark = strChannelMark;
		FillSubDevInfoByparent(subChannelObject, newdeviceInfo);
		subChannelObject.linkedInfo.strDeviceName = strChannelName;
		_husDevType = HUSDeviceType::DT_NULL;
		auto  _gb_sub_deviceType = GetDeviceTypeByName(strChannelMark, _husDevType);
		subChannelObject.eGBDevType = _gb_sub_deviceType;
		subChannelObject.extDevInfo.SetIOType(_husDevType);

		if (_husDevType == HUSDeviceType::DT_CHANNEL)
		{
			// ��Channel���Ϊ����Alarm��
			//(��HUS�豸�����У�IPC ��ChannelͬʱҲ�Ǳ���ͨ����PTZͨ��,����Streamer�����Ҳ���б�����)������
			// channelͬʱҲ��һ�������Alarm��������Ҫ��Channel ��Ϊһ����������ӡ�
			WriteAlarmChannelConfig(nElementIndex + channel_count, strConfigName, subChannelObject);
			// ��ʼ������ͨ��״̬
			//HUS��Stremaer ��GB������һ��IPC,��Ҫ�������һ�����󣬲��Ҹö������߼�����Channel���д��ڡ�
			DeviceObject	subVideoObject;
			subVideoObject.eGBDevType = GBDeviceType::OT_CAMERA;
			subVideoObject.extDevInfo.SetIOType(DT_STREAMER);
			FillSubDevInfoByparent(subVideoObject, subChannelObject);
			if (newdeviceInfo.eGBDevType == GBDeviceType::OT_IPC)
			{
				subVideoObject.strDeviceID = newdeviceInfo.strDeviceID;
				subVideoObject.eGBDevType = GBDeviceType::OT_IPC;
			}
			auto  result = WriteVideoChannelConfig(nElementIndex++, strConfigName, subVideoObject);

			if (newdeviceInfo.eGBDevType == GBDeviceType::OT_IPC)
			{
				newdeviceInfo.linkedInfo.guidEC = subVideoObject.linkedInfo.guidEC;
				newdeviceInfo.linkedInfo.guidNVR = subVideoObject.linkedInfo.guidNVR;
				newdeviceInfo.linkedInfo.guidVirSteamer = subVideoObject.linkedInfo.guidVirSteamer;
				newdeviceInfo.linkedInfo.strNVRIP = subVideoObject.linkedInfo.strNVRIP;
				newdeviceInfo.linkedInfo.StrStreamerGUID = subVideoObject.strGuidDevice;
				newdeviceInfo.linkedInfo.guidParent = subVideoObject.guidDevice;//ptz���ƶ�Ӧ��stream���GUID
				newdeviceInfo.linkedInfo.guidVirSteamer = subVideoObject.linkedInfo.guidVirSteamer;
			}
			if (result == S_FALSE && j < channel_count) continue;
			if (newdeviceInfo.eGBDevType != GBDeviceType::OT_IPC)///ignore Streamer OF IPC for HUS Device.
			{
				subVideoObject.linkedInfo.StrStreamerGUID = subVideoObject.strGuidDevice;
				subVideoObject.linkedInfo.strDeviceName = subChannelObject.strName;
				subVideoObject.linkedInfo.guidParent = subVideoObject.guidDevice;
				AddToDataStore(subVideoObject, &newdeviceInfo);
			}
		}
		else if (_husDevType == HUSDeviceType::DT_ALARM_CHANNEL)
		{
			WriteAlarmChannelConfig(nElementIndex++, strConfigName, subChannelObject);
		}
		else if (_husDevType == HUSDeviceType::DT_DECODER_CHANNEL)
		{
			WriteDecorderChannelConfig(nElementIndex++, strConfigName, subChannelObject);
		}
		AddToDataStore(subChannelObject, &newdeviceInfo);
	}
	AddToDataStore(newdeviceInfo);
}

void CSDKDevInfo::WriteDvrDeviceConfig(const CString & strConfigName, DeviceObject & newDVRDeviceObject)
{
	TCHAR szData[MAX_PATH] = { 0 };

	// д���豸���ͺ��豸��
	WritePrivateProfileString("INFO", "DeviceType", newDVRDeviceObject.strDeviceTyeMark, strConfigName);
	WritePrivateProfileString("CATALOG", "Name", newDVRDeviceObject.strName, strConfigName);

	//��ȡ�豸ID,��һ���ǻ�ȡ��������ΪdeviceManagerδ����,Ҫ�Զ�����GBID
	GetPrivateProfileString("CATALOG", "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	CString CurrentDeviceID(szData);  //����DVR��GBID.
	if (CurrentDeviceID.GetLength() != ID_LEN)
	{
		CurrentDeviceID = m_GBID_Creater.Create_DevIDByType(newDVRDeviceObject.eGBDevType).c_str();
		Utils::StringCpy_s(szData, MAX_PATH, CurrentDeviceID);
		WritePrivateProfileString("CATALOG", "DeviceID", szData, strConfigName);
	}
	CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s szData = %s strlen(szData) = %d ID_LEN = %d\r\n", __FUNCTION__, strConfigName, CurrentDeviceID, CurrentDeviceID.GetLength(), ID_LEN);

	newDVRDeviceObject.strDeviceID = CurrentDeviceID;
	newDVRDeviceObject.extDevInfo.SetDeviceID(CurrentDeviceID);
}

HRESULT CSDKDevInfo::WriteVideoChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subVideoObject)
{
	auto sarrStreamer = m_ptrSynAdapter->GetSubElementsArray(subVideoObject.linkedInfo.guidParent);
	_ECElementPtr *arrStreamer = nullptr;
	if (FAILED(SafeArrayAccessData(sarrStreamer, reinterpret_cast<void**>(&arrStreamer))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s SafeArrayAccessData failure no channel\r\n", __FUNCTION__, strConfigName);
		return S_FALSE;
	}
	GUID			guid_Streamer;
	// ��ȡ��������GUID�������ж�����GB��NVR
	guid_Streamer = arrStreamer[0]->GetID();
	CString			strStreamerMark = static_cast<LPCTSTR>(m_ptrSynAdapter->GetTypeMark(guid_Streamer));
	auto bstr_streamername = arrStreamer[0]->GetName();
	auto strStreamerName = CString(static_cast<LPCTSTR>(bstr_streamername));
	CString			strStreamerGUID;
	Utils::GUIDToCString(guid_Streamer, strStreamerGUID);
	CLog::Log(SDKCOM, LL_NORMAL, "��ʼ��HUS��������,�豸GUID:%s Type:%s", strStreamerGUID, strStreamerMark);

	subVideoObject.strName = strStreamerName;
	subVideoObject.strDeviceTyeMark = strStreamerMark;
	subVideoObject.guidDevice = guid_Streamer;
	subVideoObject.strGuidDevice = strStreamerGUID;
	// ��ȡ��ǰ�豸����Ӧ��NVR�����GUID
	// ���ﷵ�ص���GUID���飬��Ϊһ���豸���ܹҽ��ڶ��NVR������
	// ����GB28181 Gateway��˵��ѡ���һ��NVR����
	GUID *arguidNVR = nullptr;
	auto arNVR = m_ptrSynAdapter->GetSServerIDArrayByElementID(subVideoObject.guidDevice);

	if (arNVR != nullptr && FAILED(SafeArrayAccessData(arNVR, reinterpret_cast<void**>(&arguidNVR))))
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s access Device nvr info failure\r\n", __FUNCTION__, strConfigName);
		return S_FALSE;
	}
	else if (arNVR == nullptr)
	{
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s No NVR Linked to streamer: {%s}  \r\n", __FUNCTION__, strConfigName, strStreamerGUID);
		return S_FALSE;
	}
	GetSettingsParam(arguidNVR[0], L"IP", subVideoObject.linkedInfo.strNVRIP);
	subVideoObject.linkedInfo.guidNVR = arguidNVR[0]; //ֻ�����һ��NVR��GUID.
// �豸������Ϣд���豸�����ļ���

	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder); //ENCODER(E4V/E8X��)��DECODER����ͨ���϶�  DVR
	WritePrivateProfileString(strSection, "GUID", subVideoObject.strGuidDevice, strConfigName);
	WritePrivateProfileString(strSection, "Model", "Camera", strConfigName); //streamer ������ΪCamera.
	WritePrivateProfileString(strSection, "Name", subVideoObject.strName, strConfigName); //write streamer name
	// ���豸��IPCʱ����ȡCATALOG��ID
	TCHAR szData[MAX_PATH] = { 0 };
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	//���� Steamer��GBID.
	CString CurrentStreamerID(szData);
	if (CurrentStreamerID.GetLength() != ID_LEN)
	{
		if (subVideoObject.eGBDevType == GBDeviceType::OT_IPC)
			CurrentStreamerID = subVideoObject.strDeviceID;
		else
			CurrentStreamerID = m_GBID_Creater.CreateSubDeviceid(subVideoObject.eGBDevType, subVideoObject.strDeviceID, indexOrder).c_str();
		CLog::Log(SDKCOM, LL_NORMAL, "%s configPath = %s add device szData = %s\r\n", __FUNCTION__, strConfigName, szData);
		// ���GBID ��HUS��GUID��ӳ��
		WritePrivateProfileString(strSection, "DeviceID", CurrentStreamerID, strConfigName);
	}
	subVideoObject.strDeviceID = CurrentStreamerID;

	//�����豸���ӵ���EC��ID.
	if (GUID_NULL == subVideoObject.linkedInfo.guidEC)  //��ǰ��DVRδ���ӵ�EC
	{
		GUID tmpguidTarget;
		subVideoObject.linkedInfo.guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(subVideoObject.linkedInfo.guidParent, &tmpguidTarget);
	}
	m_ptrSynAdapter->GetECServerIDByElementID_2(subVideoObject.guidDevice, &(subVideoObject.linkedInfo.guidVirSteamer));
	CString strVirStreamer;
	Utils::GUIDToCString(subVideoObject.linkedInfo.guidVirSteamer, strVirStreamer, FALSE);
	m_oECToDevMap.SetAt(strVirStreamer, subVideoObject.linkedInfo.guidEC);
	CLog::Log(SDKCOM, LL_DEBUG, _T("�������Ƶ����ECӳ����� GUID��%s VirGUID:%s ���г��ȣ�%d"), subVideoObject.strGuidDevice, strVirStreamer, m_oECToDevMap.GetSize());
	return S_OK;
}

void CSDKDevInfo::WriteAlarmChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subAlarmObject)
{
	TCHAR szData[MAX_PATH] = { 0 };
	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder);
	WritePrivateProfileString(strSection, "GUID", subAlarmObject.strGuidDevice, strConfigName);
	WritePrivateProfileString(strSection, "Model", subAlarmObject.strDeviceTyeMark, strConfigName);
	WritePrivateProfileString(strSection, "Name", subAlarmObject.strName, strConfigName);
	GetPrivateProfileString(strSection, "DeviceID", "\0", szData, MAX_PATH, strConfigName);
	CString strAlarmDeviceID(szData);
	if (ID_LEN != strAlarmDeviceID.GetLength())
	{
		Utils::StringCpy_s(szData, MAX_PATH, m_GBID_Creater.Create_AlarmDevID().c_str());
		WritePrivateProfileString(strSection, "DeviceID", szData, strConfigName);
		CLog::Log(SDKCOM, LL_NORMAL, "%s AlarmChannel deviceId = %s AlarmChannel Guid = %s\r\n", __FUNCTION__, szData, subAlarmObject.strGuidDevice);
	}
	if (GUID_NULL == subAlarmObject.linkedInfo.guidEC)
	{
		GUID tmpguidTarget;
		subAlarmObject.linkedInfo.guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(subAlarmObject.guidDevice, &tmpguidTarget);
	}

	subAlarmObject.strDeviceID = strAlarmDeviceID;
	subAlarmObject.strAlarmID = strAlarmDeviceID;
}

void CSDKDevInfo::WriteDecorderChannelConfig(int indexOrder, const CString &strConfigName, DeviceObject &subDisplayObject /*Streamer OF the Channel */)
{
	TCHAR szData[MAX_PATH] = { 0 };
	CString strSection;
	strSection.Format("CHANNEL_CATALOG%d", indexOrder);
	WritePrivateProfileString(strSection, "GUID", subDisplayObject.strGuidDevice, strConfigName);
	WritePrivateProfileString(strSection, "Model", subDisplayObject.strDeviceTyeMark, strConfigName);
	GetPrivateProfileString(strSection, "DeviceID", "NaN", szData, MAX_PATH, strConfigName);
	CString strDecoderDisplayDeviceID(szData);
	if (ID_LEN != strDecoderDisplayDeviceID.GetLength())
	{
		strDecoderDisplayDeviceID = m_GBID_Creater.Create_DecoderID().c_str();
		Utils::StringCpy_s(szData, MAX_PATH, strDecoderDisplayDeviceID);
		WritePrivateProfileString(strSection, "DeviceID", strDecoderDisplayDeviceID, strConfigName);
	}
	if (GUID_NULL == subDisplayObject.linkedInfo.guidEC)
	{
		GUID tmpguidTarget;
		subDisplayObject.linkedInfo.guidEC = m_ptrSynAdapter->GetECServerIDByElementID_2(subDisplayObject.guidDevice, &tmpguidTarget);
	}
	// ȡ��ͨ�����
	CString strMonitorNum;
	CString strChannelID;
	GetSettingsParam(subDisplayObject.guidDevice, L"Monitor", strMonitorNum);
	GetSettingsParam(subDisplayObject.guidDevice, L"ChannelID", strChannelID);

	auto  p_LinkedInfo = new HUSDeviceLinkInfo();
	p_LinkedInfo->strChannelNum = strMonitorNum + ":" + strChannelID;
	p_LinkedInfo->strDevIP = subDisplayObject.strIP;
	p_LinkedInfo->strDevPort = subDisplayObject.strPort;

	// ��GBID��key
	m_oDecoderMap.SetAt(strDecoderDisplayDeviceID, p_LinkedInfo);

	CString strAddress = subDisplayObject.strIP + ":" + subDisplayObject.strPort;
	subDisplayObject.strDeviceID = strDecoderDisplayDeviceID;
	m_oDecoderIPtoIDMap.SetAt(strAddress, subDisplayObject.strDeviceID);
}

// ����豸����
void CSDKDevInfo::AddDeviceObject(const GUID &guidDevcie)
{
	if (nullptr == m_ptrSynAdapter)
		return;

	// ��ȡ�豸�б�������Gateway��GUID������ֵ���豸����
	// ���ﷵ�ص��豸�б�������йҽ��ڵ�ǰGateway�µ��豸
	_ECElementPtr ptrECElement = m_ptrSynAdapter->GetElement(guidDevcie);
	if (nullptr == ptrECElement)
		return;

	IniDeviveProperty(ptrECElement);
}

ECInfoMap &CSDKDevInfo::GetECToDevMap()
{
	return m_oECToDevMap;
}

void CSDKDevInfo::DecoderBind(const GUID &guidDecoder, const GUID &guidChannel)
{
	HUSDeviceLinkInfo *pInfo = nullptr;
	CString strDecoderGUID;
	Utils::GUIDToCString(guidDecoder, strDecoderGUID);

	DeviceObject tObjInfo;
	CString deviceID;
	if ("{" != strDecoderGUID.Left(1))
		strDecoderGUID = "{" + strDecoderGUID + "}";
	CDataStore::LookupDeviceID(strDecoderGUID, deviceID);
	if (m_oDecoderMap.Lookup(deviceID, pInfo, TRUE))
	{
		pInfo->guidParent = guidChannel;
	}
	m_oDecoderMap.ManualUnlock();
}

void CSDKDevInfo::GUIDTranslatedIntoGBID(const GUID &guidDevice, CString &strGBID)
{
	CString strDeviceGUID;
	Utils::GUIDToCString(guidDevice, strDeviceGUID);
	CString strDeviceID;
	if ("{" != strDeviceGUID.Left(1))
		strDeviceGUID = "{" + strDeviceGUID + "}";
	CDataStore::LookupDeviceID(strDeviceGUID, strDeviceID);
	strGBID = strDeviceID;
}

// �����������Ƶ����
void CSDKDevInfo::DecoderUnbind(const GUID &guidDecoder)
{
	HUSDeviceLinkInfo *pInfo = nullptr;
	CString strDecoderGUID;
	Utils::GUIDToCString(guidDecoder, strDecoderGUID);
	CString strDeviceID;
	if ("{" != strDecoderGUID.Left(1))
		strDecoderGUID = "{" + strDecoderGUID + "}";
	CDataStore::LookupDeviceID(strDecoderGUID, strDeviceID);

	if (m_oDecoderMap.Lookup(strDeviceID, pInfo, TRUE))
	{
		memset(&(pInfo->guidParent), 0, sizeof(pInfo->guidParent));
	}
	m_oDecoderMap.ManualUnlock();
}

int CSDKDevInfo::GetAllGUID(CList<GUID> &guidList) const
{
	//ʵ�ʶ������DVR��GUID.
	return CDataStore::GetAllDeviceGUID(guidList);
}

CString CSDKDevInfo::GetGatewyGUID(BOOL bIsWithBraces) const
{
	CString strGatewayGUID;
	Utils::GUIDToCString(m_guidGateway, strGatewayGUID, bIsWithBraces);
	return  strGatewayGUID;
}

void CSDKDevInfo::AddHUSDeviceLinkInfo(const TCHAR* pszGUID, HUSDeviceLinkInfo& deviceLinkinfo)
{
	//�鲻���豸��Ϣ�����
	DeviceObject tmpDeviceInfo;
	if (!CDataStore::LookupHUSDevice(pszGUID, tmpDeviceInfo, TRUE))
	{
		tmpDeviceInfo.linkedInfo = deviceLinkinfo;
		CDataStore::AddHUSDeviceObj(pszGUID, tmpDeviceInfo);
	}
	CDataStore::ManualUnlockHUSDeviceInfoMap();
}
void CSDKDevInfo::SetOwner(CSDKCom* p_SDKCom)
{
	this->p_oSDKCom = p_SDKCom;
}

GBDeviceType CSDKDevInfo::GetDeviceTypeByName(CString & strType, HUSDeviceType & husDevType, BOOL isDvrLayer)
{
	//Channel  Contains: channel/alarm/decoder
	strType.MakeLower();
	GBDeviceType _gbdt = GBDeviceType::OT_NONE;

	if (isDvrLayer)
	{
		husDevType = DT_DVR;
		if (strType.Find(_T("ipc")) >= 0)
			_gbdt = GBDeviceType::OT_IPC;
		else if (strType.Find(_T("dvr")) >= 0)
			_gbdt = GBDeviceType::OT_DVR;
		else if (strType.Find(_T("encoder")) >= 0)
			_gbdt = GBDeviceType::OT_ENCODER;
		else if (strType.Find(_T("nvr")) >= 0)
			_gbdt = GBDeviceType::OT_NVR;
		return _gbdt;
	}

	if (strType.Find(_T("channel")) >= 0)
	{
		_gbdt = GBDeviceType::OT_ALARM;
		husDevType = DT_CHANNEL;
	}
	else if (strType.Find(_T("alarm")) >= 0)
	{
		_gbdt = GBDeviceType::OT_ALARM;
		husDevType = HUSDeviceType::DT_ALARM_CHANNEL;
	}
	else if (strType.Find(_T("decoder")) >= 0)
	{
		_gbdt = GBDeviceType::OT_DISPLAY;
		husDevType = HUSDeviceType::DT_DECODER_CHANNEL;
	}
	else if (strType.Find(_T("streamer")) >= 0)
	{
		_gbdt = GBDeviceType::OT_CAMERA;
		husDevType = DT_STREAMER;
	}
	return _gbdt;
}