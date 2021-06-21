#include "StdAfx.h"
#include "regManagerCom.h"
#include "MainThread.h"

CRegManagerCom::CRegManagerCom(void)
{
}

CRegManagerCom::~CRegManagerCom(void)
{
}

void CRegManagerCom::Init(void)
{
    // ע��Module��Ϣ������
    RegisterProc(pfnQueueProc, this, 1);
}

void CRegManagerCom::Cleanup(void)
{
}


bool CRegManagerCom::HandleMsg(CMemPoolUnit * pUnit)
{
    CUnifiedMessage * pUnifiedMsg = reinterpret_cast<CUnifiedMessage *>(pUnit);

    // ȡ����Ϣ����
    DevOperateType eOperateType = pUnifiedMsg->GetOperateType();
/******************
    switch(eOperateType)
    {	
    // ת����Ϣ��SDKComģ�飬
    // ��ѯ�豸��Ӧ��GUID
    case ot_rtsp_search_no_sdp:
        {
            // ����SDP�ļ�
     
            pUnifiedMsg->SetOperateType(ot_sdk_search_guid_no_dsp);
            CString strPort;
            CRouter::PushMsg(SDKCOM, pUnifiedMsg);
        }
        break;

    case ot_rtsp_search_sdp:
        {
            // ����SDP�ļ�
            // SDPЭ��ջ
            SDPParser sdpStack;	
            if(NULL != pUnifiedMsg->GetPlayData())
            {
                CBigFile *pSDPFile = reinterpret_cast<CBigFile*>(pUnifiedMsg->GetPlayData());
                sdpStack.decode_sdp(pSDPFile->GetBuffer());
            }
            else
            {
                CLog::Log(RTSPCOM, LL_NORMAL, "SIP��Ϣ��δ�ҵ�DSP�ļ����� SN:%s", pUnifiedMsg->GetQuerySN());
                pUnifiedMsg->Free();
                break;
            }

            pUnifiedMsg->SetOperateType(ot_sdk_search_guid);
            CString strPort;
            if(0 < sdpStack.GetSDPStack().media_list.size())
            {
                strPort = sdpStack.GetSDPStack().media_list.front().port;
            }
            pUnifiedMsg->SetRecvAddress(sdpStack.GetSDPStack().connect_info.address, strPort);
            CRouter::PushMsg(SDKCOM, pUnifiedMsg);
        }
        break;
    // �յ��豸GUID
    case ot_rtsp_guid_result_no_sdp:
        {
            this->m_oThirdCallSessionMgr.ProcessSIP(pUnifiedMsg);
            break;
        }


    case ot_rtsp_guid_result_broadcast:
        {
            m_oBroadCastCallSessionMgr.ProcessSIP(pUnifiedMsg);
            break;
        }

    case ot_rtsp_guid_result:
        {
            this->m_oSessionMgr.ProcessSIP(pUnifiedMsg);
            break;
        }
    // ���ſ���
    case ot_rtsp_play_start:
    case ot_rtsp_play_stop:
    case ot_rtsp_play_ctrl:
    //	m_oSessionMgr.ProcessSIP(pUnifiedMsg);
        this->ProcessSIP(pUnifiedMsg);
            break;
    // ¼�����
    case ot_rtsp_record_start:
    case ot_rtsp_record_stop:
        m_oRecordMgr.ProcessSIP(pUnifiedMsg);
            break;
        default:
            break;
    }
***********************************************/
    return true;
}