#pragma once


#include "resource.h"		// main symbols

// CXPTipDlg 对话框

class CXPTipDlg : public CDialog
{
	DECLARE_DYNAMIC(CXPTipDlg)

public:
	CXPTipDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CXPTipDlg();

// 对话框数据
	enum { IDD = IDD_XPTIPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	void showtip(CPoint point, CString strText);
	void hidetip();

	afx_msg void OnPaint();

private:
	BOOL m_bTracking;   //在鼠标按下没有释放时该值为true
	BOOL m_isShow;
	CString m_disText;
	CPoint m_disPoint;
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
};
