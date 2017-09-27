// MyListCtrl.cpp : 实现文件
//

#include "stdafx.h"
#include "resource.h"
#include "XPListCtrl.h"


// CXPListCtrl

IMPLEMENT_DYNAMIC(CXPListCtrl, CListCtrl)

CXPListCtrl::CXPListCtrl()
{
	m_OddItemBkColor=0xFEF1E3;//奇数行背景颜色
	m_EvenItemBkColor=0xFFFFFF;//偶数行背景颜色
	m_HoverItemBkColor=0x7FFF;//热点行背景颜色
	m_SelectItemBkColor=GetSysColor(COLOR_HIGHLIGHT);//选中行背景颜色

	m_OddItemTextColor=GetSysColor(COLOR_BTNTEXT);//奇数行文本颜色
	m_EvenItemTextColor=GetSysColor(COLOR_BTNTEXT);//偶数行文本颜色
	m_HoverItemTextColor=GetSysColor(COLOR_HIGHLIGHTTEXT);//热点行文本颜色
	m_SelectItemTextColor=GetSysColor(COLOR_BTNTEXT);//选中行文本颜色
	m_nHoverIndex=-1;
	m_bTracking=FALSE;
#if 0
	m_tipDlg=new CXPTipDlg;
	m_tipDlg->Create(IDD_XPTIPDLG, this);
#endif
}

CXPListCtrl::~CXPListCtrl()
{
#if 0
	delete m_tipDlg;
#endif
}


BEGIN_MESSAGE_MAP(CXPListCtrl, CListCtrl)
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CXPListCtrl::OnNMCustomdraw)
END_MESSAGE_MAP()



// CXPListCtrl 消息处理程序


void CXPListCtrl::SetOddItemBkColor(COLORREF color,BOOL bDraw)//设置奇数行背景颜色
{
	m_OddItemBkColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetEvenItemBkColor(COLORREF color,BOOL bDraw)//设置偶数行背景颜色
{
	m_EvenItemBkColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetHoverItemBkColor(COLORREF color,BOOL bDraw)//设置热点行背景颜色
{
	m_HoverItemBkColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetSelectItemBkColor(COLORREF color,BOOL bDraw)//设置选中行背景颜色
{
	m_SelectItemBkColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetOddItemTextColor(COLORREF color,BOOL bDraw)//设置奇数行文本颜色
{
	m_OddItemTextColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetEvenItemTextColor(COLORREF color,BOOL bDraw)//设置偶数行文本颜色
{
	m_EvenItemTextColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetHoverItemTextColor(COLORREF color,BOOL bDraw)//设置热点行文本颜色
{
	m_HoverItemTextColor=color;
	if(bDraw)InvalidateRect(NULL);
}
void CXPListCtrl::SetSelectItemTextColor(COLORREF color,BOOL bDraw)//设置选中行文本颜色
{
	m_SelectItemTextColor=color;
	if(bDraw)InvalidateRect(NULL);
}


void CXPListCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	int nIndex=HitTest(point);
	if(nIndex!=m_nHoverIndex){
		int nOldIndex=m_nHoverIndex;
		m_nHoverIndex=nIndex;
		CRect rc;
		if(nOldIndex!=-1){
			GetItemRect(nOldIndex,&rc,LVIR_BOUNDS);
			InvalidateRect(&rc);
		}		
		if(m_nHoverIndex!=-1){
			GetItemRect(m_nHoverIndex,&rc,LVIR_BOUNDS);
			InvalidateRect(&rc);
#if 0
			CString strDesc = this->GetItemText(m_nHoverIndex, 3);
			m_tipDlg->showtip(point, strDesc);
#endif
		}
		else
		{
#if 0
			m_tipDlg->hidetip();
#endif
		}
	}

	//=====================================================
	if(!m_bTracking) 
	{ 
		TRACKMOUSEEVENT   tme; 
		tme.cbSize   =   sizeof(tme); 
		tme.hwndTrack   =   m_hWnd; 
		tme.dwFlags   =   TME_LEAVE;//   |   TME_HOVER; 
		tme.dwHoverTime   =   1; 
		m_bTracking   =   _TrackMouseEvent(&tme); 
	} 

	CListCtrl::OnMouseMove(nFlags, point);
}
LRESULT CXPListCtrl::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	m_bTracking=FALSE;
	
	if(m_nHoverIndex!=-1){
		CRect rc;
		GetItemRect(m_nHoverIndex,&rc,LVIR_BOUNDS);
		m_nHoverIndex=-1;
		InvalidateRect(&rc);
	}
	return 0;
}
void CXPListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	*pResult = 0;
	int nItemIndex=(int)pNMCD->nmcd.dwItemSpec;
	if (pNMCD->nmcd.dwDrawStage==CDDS_PREPAINT){
		*pResult = CDRF_NOTIFYITEMDRAW;
	}else{
		if(nItemIndex==m_nHoverIndex){ //热点行
			pNMCD->clrTextBk=m_HoverItemBkColor;
			pNMCD->clrText=m_HoverItemTextColor;
		}else if(GetItemState(nItemIndex,LVIS_SELECTED) == LVIS_SELECTED){ //选中行
			pNMCD->clrTextBk=m_SelectItemBkColor;
			pNMCD->clrText=pNMCD->clrFace=m_SelectItemTextColor;
			::SetTextColor(pNMCD->nmcd.hdc,m_SelectItemTextColor);
		}else if(nItemIndex % 2==0){//偶数行 比如 0、2、4、6
			pNMCD->clrTextBk=m_EvenItemBkColor;
			pNMCD->clrText=m_EvenItemTextColor;
		}else{	//奇数行 比如 1、3、5、7
			pNMCD->clrTextBk=m_OddItemBkColor;
			pNMCD->clrText=m_OddItemTextColor;
		}
		*pResult = CDRF_NEWFONT;
	}
}
