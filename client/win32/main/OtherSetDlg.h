#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// COtherSetDlg dialog

class COtherSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(COtherSetDlg)

public:
	COtherSetDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COtherSetDlg();

// Dialog Data
	enum { IDD = IDD_ADVANCE_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCmbLoglevel();
	CComboBox m_cmb_loglevel;
	CEdit m_edt_thrdcnt;
	CEdit m_edt_cpucnt;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
