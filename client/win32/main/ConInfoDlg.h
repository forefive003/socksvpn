#pragma once
#include "afxcmn.h"


// CConInfoDlg dialog

class CConInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConInfoDlg)

public:
	CConInfoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConInfoDlg();

// Dialog Data
	enum { IDD = IDD_CONN_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_lst_conns;
	virtual BOOL OnInitDialog();
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL DestroyWindow();

private:
	void update_conns_list();
};
