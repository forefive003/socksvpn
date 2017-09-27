// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoProxy.h"
#include "XPProgDlg.h"
#include "afxdialogex.h"


// XPProgDlg dialog

IMPLEMENT_DYNAMIC(XPProgDlg, CDialogEx)

XPProgDlg::XPProgDlg(int progMax, CString &strText, CWnd* pParent /*=NULL*/)
	: CDialogEx(XPProgDlg::IDD, pParent)
{
	m_prog_max = progMax;
	m_strText = strText;
}

XPProgDlg::~XPProgDlg()
{
}

void XPProgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STC_PROG_TEXT, m_stc_prog_text);
	DDX_Control(pDX, IDC_PROGRESS, m_prog_ctrl);
}


BEGIN_MESSAGE_MAP(XPProgDlg, CDialogEx)
ON_MESSAGE(WM_UPDATE_PROGRESS, OnUpdateProgress)
ON_WM_PAINT()
ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// XPProgDlg message handlers


BOOL XPProgDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	m_stc_prog_text.SetWindowTextA(m_strText);
	m_prog_ctrl.SetRange(0, m_prog_max);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT  XPProgDlg::OnUpdateProgress(WPARAM wParam,LPARAM lParam)
{
	int curProg = (int)wParam;
	char *curText = (char*)lParam;

	if (curProg != 0)
	{
		m_stc_prog_text.SetWindowTextA(curText);
		m_prog_ctrl.SetPos(curProg);
		delete curText;

		UpdateData(TRUE);
	}
	else
	{
		this->EndDialog(IDCANCEL);  //ÍË³ö
	}

	return 0;
}



void XPProgDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CDialogEx::OnPaint() for painting messages
#if 0
	CRect rect;
	GetClientRect(&rect);

	CBrush brBk;
	brBk.CreateSolidBrush(RGB(0,128,0));

	dc.FillRect(rect,&brBk);
#endif

	CDialogEx::OnPaint();
}


HBRUSH XPProgDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Change any attributes of the DC here

	// TODO:  Return a different brush if the default is not desired
		#if 0	
	int idTmp = pWnd->GetDlgCtrlID();
	switch (idTmp)
	{
		case IDC_STC_PROG_TEXT:	
		{
			pDC->SetBkMode(TRANSPARENT);
			return (HBRUSH)::GetStockObject(NULL_BRUSH);
		}
		default:
			break;
	}
#endif

	return hbr;
}
