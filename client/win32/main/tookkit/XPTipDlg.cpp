// XPTipDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "XPTipDlg.h"
#include "afxdialogex.h"


// CXPTipDlg 对话框

IMPLEMENT_DYNAMIC(CXPTipDlg, CDialog)

CXPTipDlg::CXPTipDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CXPTipDlg::IDD, pParent)
{
	m_bTracking = FALSE;
}

CXPTipDlg::~CXPTipDlg()
{
}

void CXPTipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CXPTipDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()


// CXPTipDlg 消息处理程序


void CXPTipDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	//**重绘背景色
	if (m_isShow)
	{
		CRect re;
		dc.SetBkColor(RGB(255,255,150));
		GetClientRect(&re);
		CBrush brush;
		brush.CreateSolidBrush(RGB(255,255,150));
		dc.SelectObject(&brush);
		//dc.Rectangle(re);

		dc.SetTextColor(RGB(0,0,0));
		dc.TextOut(0, 0, m_disText);
	}

	// 不为绘图消息调用 CDialog::OnPaint()
}

void CXPTipDlg::showtip(CPoint point1, CString strText)
{
	m_disText = strText;
	m_isShow = TRUE;

	DWORD pos=GetMessagePos();
	CPoint point = (LOWORD(pos),HIWORD(pos));//当前鼠标的位置

	point.x=LOWORD(pos);
	point.y=HIWORD(pos);

	CRect reDlg;
	reDlg.left=point.x;
	reDlg.top=point.y;
	reDlg.right=reDlg.left + 10 * strText.GetLength();
	reDlg.bottom=reDlg.top + 25;

	this->MoveWindow(reDlg); //是相对于屏幕的

	this->ShowWindow(TRUE);
	this->Invalidate();
}

void CXPTipDlg::hidetip()
{
	m_isShow = FALSE;

	this->ShowWindow(FALSE);
	this->Invalidate();
}




void CXPTipDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (!m_bTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE;
		tme.dwHoverTime = 1;
		m_bTracking = _TrackMouseEvent(&tme);
	}

	CDialog::OnMouseMove(nFlags, point);
}


LRESULT CXPTipDlg::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bTracking = FALSE;
	hidetip();

	return 0;
}
