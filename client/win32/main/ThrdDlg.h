#pragma once
#include "afxcmn.h"


// CThrdDlg dialog

class CThrdDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CThrdDlg)

public:
	CThrdDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CThrdDlg();

// Dialog Data
	enum { IDD = IDD_THRD_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_thrdlist;
	virtual BOOL OnInitDialog();

private:
	void update_thrdlist();
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL DestroyWindow();
};
