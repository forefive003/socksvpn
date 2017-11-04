// ConInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoProxy.h"
#include "ConInfoDlg.h"
#include "afxdialogex.h"

#include "commtype.h"
#include "logproc.h"
#include "common_def.h"
#include "proxyConfig.h"

#include "CNetAccept.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "CRemoteServer.h"
#include "relay_pkt_def.h"
#include "CSyslogMgr.h"
#include "CRemoteServerPool.h"


// CConInfoDlg dialog

IMPLEMENT_DYNAMIC(CConInfoDlg, CDialogEx)

CConInfoDlg::CConInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CConInfoDlg::IDD, pParent)
{

}

CConInfoDlg::~CConInfoDlg()
{
}

void CConInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CONNS, m_lst_conns);
}


BEGIN_MESSAGE_MAP(CConInfoDlg, CDialogEx)
	ON_WM_TIMER()
END_MESSAGE_MAP()



// CConInfoDlg message handlers

void CConInfoDlg::update_conns_list()
{
	int rowCnt = m_lst_conns.GetItemCount();
	int rowIndex = 0;

	char connNoStr[32] = { 0 };
	char srvPortStr[32] = { 0 };
	char statusStr[64] = { 0 };
	char sessionCntStr[32] = { 0 };

	CRemoteServer *rmtSrv = NULL;

	for (int ii = 0; ii < g_remoteSrvPool->m_max_conn_cnt; ii++)
	{
		SNPRINTF(connNoStr, sizeof(connNoStr), "%d", ii);

		if (rowIndex >= rowCnt)
		{
			m_lst_conns.InsertItem(rowIndex, connNoStr);//插入行 	
		}

		g_remoteSrvPool->lock_index(ii);
		rmtSrv = (CRemoteServer*)g_remoteSrvPool->m_conns_array[ii].connObj;
		if (NULL != rmtSrv)
		{
			SNPRINTF(srvPortStr, sizeof(srvPortStr), "%d", rmtSrv->m_local_port);
			
			if (rmtSrv->is_authed())
			{
				//设置状态栏文本
				SNPRINTF(statusStr, sizeof(statusStr), _T("代理服务器已认证"));
			}
			else if (rmtSrv->is_connected())
			{
				SNPRINTF(statusStr, sizeof(statusStr), _T("代理服务器正在认证中"));
			}
			else
			{
				SNPRINTF(statusStr, sizeof(statusStr), _T("正在连接VPN服务器"));
			}

			SNPRINTF(sessionCntStr, sizeof(sessionCntStr), _T("%d"), 
						g_remoteSrvPool->get_index_session_cnt(ii));
		}
		else
		{
			SNPRINTF(srvPortStr, sizeof(srvPortStr), _T("--"));
			SNPRINTF(statusStr, sizeof(statusStr), _T("VPN服务器断开"));
			SNPRINTF(sessionCntStr, sizeof(sessionCntStr), _T("--"));
		}
		g_remoteSrvPool->unlock_index(ii);

		m_lst_conns.SetItemText(rowIndex, 1, srvPortStr);
		m_lst_conns.SetItemText(rowIndex, 2, statusStr);
		m_lst_conns.SetItemText(rowIndex, 3, sessionCntStr);

		rowIndex++;
	}

	for (int i = rowIndex; i < rowCnt; i++)
	{
		m_lst_conns.DeleteItem(i);
	}
}

BOOL CConInfoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	LONG lStyle;
	DWORD dwStyle;
	CRect rectList;

	lStyle = GetWindowLong(m_lst_conns.m_hWnd, GWL_STYLE);//获取当前窗口style 
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位 
	lStyle |= LVS_REPORT; //设置style 
	SetWindowLong(m_lst_conns.m_hWnd, GWL_STYLE, lStyle);//设置style 
	dwStyle = m_lst_conns.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl） 
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl） 
	m_lst_conns.SetExtendedStyle(dwStyle); //设置扩展风格 

	m_lst_conns.GetClientRect(rectList); //获得当前客户区信息
	m_lst_conns.InsertColumn(0, "编号", LVCFMT_LEFT, rectList.Width() * 1 / 9);
	m_lst_conns.InsertColumn(1, "本地源端口", LVCFMT_LEFT, rectList.Width() * 2 / 9);
	m_lst_conns.InsertColumn(2, "当前状态", LVCFMT_LEFT, rectList.Width() * 4 / 9);
	m_lst_conns.InsertColumn(3, "会话数", LVCFMT_LEFT, rectList.Width() * 2 / 9);

	this->update_conns_list();

	SetTimer(2, 1000, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CConInfoDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	this->update_conns_list();

	CDialogEx::OnTimer(nIDEvent);
}

BOOL CConInfoDlg::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	KillTimer(2);
	return CDialogEx::DestroyWindow();
}
