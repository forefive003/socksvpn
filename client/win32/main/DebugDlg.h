#pragma once
#include "afxcmn.h"
#include "XPListCtrl.h"
// CDebugDlg dialog

class CDebugDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDebugDlg)

public:
	CDebugDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDebugDlg();

// Dialog Data
	enum { IDD = IDD_DEBUG_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();

	virtual void OnOK();
	virtual void OnCancel();
	CXPListCtrl m_logList;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL DestroyWindow();

private:
	void update_logmsg();
};
