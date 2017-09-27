#pragma once

#include "XPButton.h"
#include "XPListCtrl.h"

// GoProxyProcDlg dialog

class GoProxyProcDlg : public CDialogEx
{
	DECLARE_DYNAMIC(GoProxyProcDlg)

public:
	GoProxyProcDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~GoProxyProcDlg();

// Dialog Data
	enum { IDD = IDD_PROC_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	CXPListCtrl m_lstApps;
	CXPListCtrl m_lstProxyProcess;

	virtual BOOL OnInitDialog();

	LRESULT OnUpdateProxyProcessList(WPARAM wParam, LPARAM lParam);

	afx_msg void OnBnClickedAddProc();
	afx_msg void OnBnClickedDelProc();
	afx_msg void OnBnClickedBtnQuit();
};
