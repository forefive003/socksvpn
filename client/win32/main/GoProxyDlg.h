
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
	afx_msg void OnDestroy();
	afx_msg void OnClose();	
	
	virtual BOOL DestroyWindow();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	
	CXPListCtrl m_lstProxyIp;

private:
	void init_config_display();
	void init_servers_display();
private:
	HBRUSH   m_brEdit;
	CFont	 m_cfontEdit;

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

	afx_msg void OnCbnSelchangeComboProxyType();
	afx_msg void OnCbnSelchangeComboEncryType();
	afx_msg void OnBnClickedChkAuth();
	CStatic m_stc_proxy_server;
	
	CComboBox m_cmbServers;

	CStatic m_server_static;

	CMenu m_Menu;
	afx_msg void OnMenuStart();
	afx_msg void OnMenuStop();
	afx_msg void OnMenuQuit();
	afx_msg void OnMenuSave();
	afx_msg void OnMenuInject();

	afx_msg void OnMenuOther();
	afx_msg void OnMenuSyslog();
	afx_msg void OnMenuAbout();
	afx_msg void OnMenuThrd();

	CEdit m_edt_alivecnt;
	CEdit m_edt_reqcnt;
	CEdit m_edt_replycnt;
	CEdit m_edt_sendrate;
	CEdit m_edt_recvrate;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnUpdateMenuStart(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMenuStop(CCmdUI *pCmdUI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnMenuConns();
};
