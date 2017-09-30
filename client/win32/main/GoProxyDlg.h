
// FullProxyDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "XPButton.h"
#include "XPListCtrl.h"

// CGoProxyDlg dialog
class CGoProxyDlg : public CDialogEx
{

// Construction
public:
	CGoProxyDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FULLPROXY_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	afx_msg LRESULT OnUpdateProxyIpList(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnUpdateProxyServers(WPARAM wParam,LPARAM lParam);

	afx_msg LRESULT OnTopShow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayNotify(WPARAM wParam,LPARAM lParam);
	afx_msg void OnMyAppExitMenu();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedProcConfig();
	afx_msg void OnDestroy();
	afx_msg void OnClose();	
	
	virtual BOOL DestroyWindow();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnBnClickedBtnProcCfg();
	afx_msg void OnBnClickedBtnStart();

	CXPListCtrl m_lstProxyIp;

private:
	void init_config_display();
	void init_servers_display();
	
public:
	CStatusBarCtrl  m_StatusBar;
	CStatusBarCtrl  m_StatusBar1;

	CComboBox m_cmb_proxy_type;
	CIPAddressCtrl m_vpn_ipaddr;
	CEdit m_edt_vpn_port;
	CEdit m_edt_local_port;
	CButton m_chk_auth;
	CEdit m_edt_username;
	CEdit m_edt_passwd;
	CComboBox m_cmb_encry_type;
	CEdit m_edt_encry_key;

	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnCbnSelchangeComboProxyType();
	afx_msg void OnCbnSelchangeComboEncryType();
	afx_msg void OnBnClickedChkAuth();
	CStatic m_stc_proxy_server;
	
	#if 0
	CXPButton m_btnProcCfg;
	CXPButton m_btnSaveCfg;
	CXPButton m_btnExit;	
	CXPButton m_btnStart;
	#else
	CXPButton m_btnProcCfg;
	CXPButton m_btnSaveCfg;
	CXPButton m_btnExit;	
	CXPButton m_btnStart;
	#endif
	CComboBox m_cmbServers;
	CXPButton m_btnSrvFresh;

	afx_msg void OnBnClickedBtnSrvFresh();
};
