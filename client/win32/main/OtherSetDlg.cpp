// OtherSetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoProxy.h"
#include "OtherSetDlg.h"
#include "afxdialogex.h"

#include "common.h"
#include "utilcommon.h"
#include "engine_ip.h"
#include "common_def.h"
#include "proxyConfig.h"
#include "clientMain.h"

// COtherSetDlg dialog

IMPLEMENT_DYNAMIC(COtherSetDlg, CDialogEx)

COtherSetDlg::COtherSetDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COtherSetDlg::IDD, pParent)
{

}

COtherSetDlg::~COtherSetDlg()
{
}

void COtherSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMB_LOGLEVEL, m_cmb_loglevel);
	DDX_Control(pDX, IDC_EDIT_THRDCNT, m_edt_thrdcnt);
	DDX_Control(pDX, IDC_EDIT_CPUCNT, m_edt_cpucnt);
}


BEGIN_MESSAGE_MAP(COtherSetDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_CMB_LOGLEVEL, &COtherSetDlg::OnCbnSelchangeCmbLoglevel)
	ON_BN_CLICKED(IDOK, &COtherSetDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// COtherSetDlg message handlers


void COtherSetDlg::OnCbnSelchangeCmbLoglevel()
{
	// TODO: Add your control notification handler code here
}


BOOL COtherSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	
	m_cmb_loglevel.InsertString(0, "DEBUG");
	m_cmb_loglevel.InsertString(1, "INFO");
	m_cmb_loglevel.InsertString(2, "WARNING");
	m_cmb_loglevel.InsertString(3, "ERROR");

	if (g_log_level == L_DEBUG)
	{
		m_cmb_loglevel.SetCurSel(0);
	}
	else if (g_log_level == L_INFO)
	{
		m_cmb_loglevel.SetCurSel(1);
	}
	else if (g_log_level == L_WARN)
	{
		m_cmb_loglevel.SetCurSel(2);
	}
	else
	{
		m_cmb_loglevel.SetCurSel(3);
	}

	CString strTemp;
	int cpu_num = util_get_cpu_core_num();

	strTemp.Format(_T("%d"), cpu_num);
	m_edt_cpucnt.SetWindowText(strTemp);

	strTemp.Format(_T("%d"), g_thrd_cnt);
	m_edt_thrdcnt.SetWindowText(strTemp);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void COtherSetDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	int loglevel = m_cmb_loglevel.GetCurSel();
	if (loglevel == 0)
	{
		loglevel = L_DEBUG;
	}
	else if (loglevel == 1)
	{
		loglevel = L_INFO;
	}
	else if (loglevel == 2)
	{
		loglevel = L_WARN;
	}
	else
	{
		loglevel = L_ERROR;
	}
	if (g_log_level != loglevel)
	{
		proxy_set_loglevel(loglevel);
		logger_set_level(loglevel);	
	}

	CString strTmp;
	int thrdcnt = 0;
	m_edt_thrdcnt.GetWindowTextA(strTmp);
	thrdcnt = _tcstoul(strTmp, NULL, 10);
	if (thrdcnt <= 0 || thrdcnt > 32)
	{
		MessageBox("Invalid thread count, should be 1~32!", "Error", MB_OK);
		return;
	}

	if (g_thrd_cnt != thrdcnt)
	{
		if (MessageBox("修改线程个数会自动重启代理, 确实要修改吗？", "提示", MB_OKCANCEL|MB_ICONWARNING) == IDOK)
		{
			proxy_set_thrdcnt(thrdcnt);
			if(0 == proxy_reset())
			{
				MessageBox("代理重启完成, 修改成功!", "提示", MB_OK);
			}
			else
			{
				MessageBox("代理重启失败, 请手动重启本程序!", "提示", MB_OK);
			}
		}		
	}
	else
	{
		MessageBox("修改成功!", "提示", MB_OK);
	}

	//CDialogEx::OnOK();
}
