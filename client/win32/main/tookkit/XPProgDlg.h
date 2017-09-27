#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "resource.h"		// main symbols

// XPProgDlg dialog

class XPProgDlg : public CDialogEx
{
	DECLARE_DYNAMIC(XPProgDlg)

public:
	XPProgDlg(int progMax, CString &strText, CWnd* pParent = NULL);   // standard constructor
	virtual ~XPProgDlg();

// Dialog Data
	enum { IDD = IDD_XPPROGDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnUpdateProgress(WPARAM wParam,LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_stc_prog_text;
	CProgressCtrl m_prog_ctrl;

private:
	int m_prog_max;
	CString m_strText;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
