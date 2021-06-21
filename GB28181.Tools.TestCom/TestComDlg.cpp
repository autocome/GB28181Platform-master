
// TestComDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TestCom.h"
#include "TestComDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestComDlg �Ի���



CTestComDlg::CTestComDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TESTCOM_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	CoInitialize(NULL);
}

void CTestComDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON, m_btnDVRVideoRecordSearch);
	DDX_Control(pDX, IDC_BUTTON2, m_btnHUSVideoRecordSearch);
}

BEGIN_MESSAGE_MAP(CTestComDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON, &CTestComDlg::OnBnClickedButton)
	ON_BN_CLICKED(IDC_BUTTON2, &CTestComDlg::OnClickedButton2)
END_MESSAGE_MAP()


// CTestComDlg ��Ϣ�������

BOOL CTestComDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	//ShowWindow(SW_MAXIMIZE);

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CTestComDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CTestComDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CTestComDlg::OnBnClickedButton()
{
	//HUS_DataManager_Search::_DVRVideoRecordSearchContext hus_record_search_contextptr

	CComPtr<IDVRVideoRecordSearchContext> dvr_record_search_context = NULL;
	HRESULT hr = dvr_record_search_context.CoCreateInstance(HUS_DataManager_Search::CLSID_DVRVideoRecordSearchContext, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && dvr_record_search_context != NULL);
	if (FAILED(hr))
	{
		MessageBox(_T("DVRVideoRecordSearchע��ʧ��!"));
	}
	else
	{
		MessageBox(_T("DVRVideoRecordSearchע��ɹ�!"));
	}
}


void CTestComDlg::OnClickedButton2()
{
	CComPtr<IHUSVideoRecordSearchContext> hus_record_search_context = NULL;
	HRESULT hr = hus_record_search_context.CoCreateInstance(HUS_DataManager_Search::CLSID_HUSVideoRecordSearchContext, NULL, CLSCTX_INPROC_SERVER);
	ASSERT(SUCCEEDED(hr) && hus_record_search_context != NULL);
	if (FAILED(hr))
	{
		MessageBox(_T("HUSVideoRecordSearchע��ʧ��!"));
	}
	else
	{
		MessageBox(_T("HUSVideoRecordSearchע��ɹ�!"));
	}
}
