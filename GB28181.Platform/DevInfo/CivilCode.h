#pragma once

typedef struct PlaceInfo{
	CString strName;				// ����
	CString strLongitude;			// ����
	CString strLatitude;			// γ��
}PlaceInfo_t;

// ������������͵�����Ϣ��ӳ��
typedef CMap<CString, LPCSTR, PlaceInfo_t, PlaceInfo_t&>	PlaceInfoMap;

// ʡ���С��к����ء����غͻ������¼�ӳ��
typedef CMap<CString, LPCSTR, CList<CString, LPCSTR>*, CList<CString, LPCSTR>*>			GradeMap;
class CCivilCode
{
public:
	CCivilCode(void);
	~CCivilCode(void);

	void Init(const char *pszFileName);

	// �����������������ȡ������Ϣ
	BOOL GetPlaceInfo(const char *pszCode, PlaceInfo_t &tPlaceInfo) const;

	// ����µ�������������͵�����Ϣ
	int AddCode(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude);
	int AddCode(const char *pszCode, const PlaceInfo_t *ptPlaceInfo);

	// �������
	static void InsertSort(CList<CString, LPCSTR> *pList, const char* pszData);

	// ����ָ���������������Ӧ�ĵ�����Ϣ
	void ModifyPlaceInfo(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude);

	// д�������Ϣ
	int WriteInfo(const char *pszCode, const char *pszPlaceName, const char *pszLongitude, const char *pszLatitude) const;

	// ����ָ����������ĵ�������Ϣ
	void ReadPlaceInfo(const char *pszCode, PlaceInfo_t &tPlaceInfo);
	
	// �����������������ļ�
	static void CreateCodeFile(const char *pszFileName);

	GradeMap		m_oProvinceToCityMap;
	GradeMap		m_oCityToCountyMap;
	GradeMap		m_oCountyToStationMap;
	CString			m_strFileName;
protected:
	static void FormatLength(CString &strData,				// ����ʽ�����ַ���
		char cCharset,							// �����ַ�
		int nDigit);							// ��ʽ����ĳ���

protected:
	PlaceInfoMap	m_oPlaceInfoMap;
};

