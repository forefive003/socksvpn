// GoProxyProcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoProxy.h"
#include "GoProxyProcDlg.h"
#include "afxdialogex.h"


#include "common.h"
#include "engine_ip.h"
#include "common_def.h"
#include "proxyConfig.h"
#include "clientMain.h"
#include "procMgr.h"

#include "CBaseObj.h"
#include "CBaseConn.h"
#include "CClient.h"
#include "CSrvRemote.h"
#include "CVpnRemote.h"
#include "CConMgr.h"

// GoProxyProcDlg dialog

IMPLEMENT_DYNAMIC(GoProxyProcDlg, CDialogEx)

GoProxyProcDlg::GoProxyProcDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(GoProxyProcDlg::IDD, pParent)
{

}

GoProxyProcDlg::~GoProxyProcDlg()
{
}

void GoProxyProcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_APPS, m_lstApps);
	DDX_Control(pDX, IDC_LIST_PROXY, m_lstProxyProcess);
}


BEGIN_MESSAGE_MAP(GoProxyProcDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &GoProxyProcDlg::OnBnClickedAddProc)
	ON_BN_CLICKED(IDC_BUTTON2, &GoProxyProcDlg::OnBnClickedDelProc)
	ON_BN_CLICKED(ID_BTN_QUIT, &GoProxyProcDlg::OnBnClickedBtnQuit)
END_MESSAGE_MAP()


// GoProxyProcDlg message handlers


LRESULT GoProxyProcDlg::OnUpdateProxyProcessList(WPARAM wParam, LPARAM lParam)
{
	for (int i = m_lstProxyProcess.GetItemCount() - 1; i >= 0; i--)
	{
		m_lstProxyProcess.DeleteItem(i);
	}

	MUTEX_LOCK(g_prox_proc_lock);

	PROC_LIST_RItr itr = g_prox_proc_objs.rbegin();
	proc_proxy_t *procInfo = NULL;
	int nRow = 0, nRowRet = 0;

	char strTemp[64] = { 0 };

	while (itr != g_prox_proc_objs.rend())
	{
		procInfo = *itr;

		/*需要在listCtrl属性中去掉自动排序功能*/
		if (procInfo->is_proc_64)
		{
			char proc_name[PROC_NAME_LEN + 8] = { 0 };
			SNPRINTF(proc_name, PROC_NAME_LEN + 8, "%s(64bit)", procInfo->proc_name);
			nRowRet = m_lstProxyProcess.InsertItem(nRow, proc_name);//插入行
		}
		else
		{
			nRowRet = m_lstProxyProcess.InsertItem(nRow, procInfo->proc_name);//插入行
		}

		SNPRINTF(strTemp, 64, "%lu", procInfo->proc_id);
		m_lstProxyProcess.SetItemText(nRowRet, 1, strTemp);//id		

		//tm* local;
		//local = localtime((time_t*)&procInfo->start_time);
		//strftime(strTemp, sizeof(strTemp) - 1, "%Y-%m-%d %H:%M:%S", local);
		//m_lstProxyProcess.SetItemText(nRowRet, 2, strTemp);//启动时间

		m_lstProxyProcess.SetItemText(nRowRet, 2, g_proxy_proc_status_desc[procInfo->status]);//状态

		itr++;
		nRow++;
	}

	MUTEX_UNLOCK(g_prox_proc_lock);

	return 0;
}

BOOL GoProxyProcDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	LONG lStyle;
	DWORD dwStyle;
	CRect rectList;

	lStyle = GetWindowLong(m_lstProxyProcess.m_hWnd, GWL_STYLE);//获取当前窗口style 
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位 
	lStyle |= LVS_REPORT; //设置style 
	SetWindowLong(m_lstProxyProcess.m_hWnd, GWL_STYLE, lStyle);//设置style 
	dwStyle = m_lstProxyProcess.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl） 
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl） 
	m_lstProxyProcess.SetExtendedStyle(dwStyle); //设置扩展风格 
	
	m_lstProxyProcess.GetClientRect(rectList); //获得当前客户区信息
	m_lstProxyProcess.InsertColumn(0, "名称", LVCFMT_LEFT, rectList.Width() / 2);//插入列
	m_lstProxyProcess.InsertColumn(1, "PID", LVCFMT_LEFT, rectList.Width() / 4);
	//m_lstProxyProcess.InsertColumn(2, "启动时间", LVCFMT_LEFT, rectList.Width() / 3);
	m_lstProxyProcess.InsertColumn(2, "代理状态", LVCFMT_LEFT, rectList.Width() / 4);
	
	lStyle = GetWindowLong(m_lstApps.m_hWnd, GWL_STYLE);//获取当前窗口style 
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位 
	lStyle |= LVS_REPORT; //设置style 
	SetWindowLong(m_lstApps.m_hWnd, GWL_STYLE, lStyle);//设置style 
	dwStyle = m_lstApps.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）
	m_lstApps.SetExtendedStyle(dwStyle); //设置扩展风格 

	m_lstApps.GetClientRect(rectList); //获得当前客户区信息
	m_lstApps.InsertColumn(0, "名称", LVCFMT_LEFT, rectList.Width());

	/*插入数据*/
	for (int ii = 0; ii < g_proxy_procs_cnt; ii++)
	{
		m_lstApps.InsertItem(ii, g_proxy_procs[ii]);
	}

	OnUpdateProxyProcessList(0,0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void GoProxyProcDlg::OnBnClickedAddProc()
{
	// TODO: Add your control notification handler code here
	// 设置过滤器   
	TCHAR szFilter[] = _T("可执行文件(*.exe)|*.exe");
	// 构造打开文件对话框   
	CFileDialog fileDlg(TRUE, _T("exe"), NULL, 0, szFilter, this);
	CString strFilePath;

	// 显示打开文件对话框   
	if (IDOK == fileDlg.DoModal())
	{
		// 如果点击了文件对话框上的“打开”按钮，则将选择的文件路径显示到编辑框里   
		strFilePath = fileDlg.GetPathName();

		char exe_name[64] = { 0 };
		const char *ch = _tcsrchr(strFilePath, _T('\\')); //查找最后一个\出现的位置，并返回\后面的字符（包括\）

		strncpy(exe_name, &ch[1], 64);
		if (proxy_cfg_proc_is_exist(exe_name))
		{
			CString tmpStr;
			tmpStr.Format(_T("%s Alreay exist!"), exe_name);
			MessageBox(tmpStr, "Error", MB_OK);
			return;
		}

		if (proxy_cfg_forbit_inject_proc_is_exist(exe_name))
		{
			CString tmpStr;
			tmpStr.Format(_T("%s can't config to be injected!"), exe_name);
			MessageBox(tmpStr, "Error", MB_OK);
			return;
		}

		if (0 != proxy_cfg_add_proc(exe_name))
		{
			CString tmpStr;
			tmpStr.Format(_T("Add exe %s failed!"), exe_name);
			MessageBox(tmpStr, "Error", MB_OK);
			return;
		}

		m_lstApps.InsertItem(0, exe_name);
		CString tmpStr;
		tmpStr.Format(_T("Add exe %s success!"), exe_name);
		MessageBox(tmpStr, "Ok", MB_OK);
	}
}


void GoProxyProcDlg::OnBnClickedDelProc()
{
	// TODO: Add your control notification handler code here
	CString str;
	for (int ii = 0; ii < m_lstApps.GetItemCount(); ii++)
	{
		if (m_lstApps.GetItemState(ii, LVIS_SELECTED) == LVIS_SELECTED)
		{
			char exe_name[128] = { 0 };
			LVITEM lvi;
			lvi.iItem = ii;
			lvi.iSubItem = 0;
			lvi.mask = LVIF_TEXT;
			lvi.pszText = exe_name;
			lvi.cchTextMax = 128;
			m_lstApps.GetItem(&lvi);

			/*删除配置*/
			proxy_cfg_del_proc(exe_name);
			/*从列表框中删除*/
			m_lstApps.DeleteItem(ii);

			CString tmpStr;
			tmpStr.Format(_T("Del exe %s!"), exe_name);
			MessageBox(tmpStr, "Ok", MB_OK);
		}
	}
}


void GoProxyProcDlg::OnBnClickedBtnQuit()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}
