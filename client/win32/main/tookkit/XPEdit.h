#pragma once


// XPEdit

class XPEdit : public CEdit
{
	DECLARE_DYNAMIC(XPEdit)

public:
	XPEdit();
	virtual ~XPEdit();

	virtual int  GetDelta();
    virtual void SetDelta(int nDelta);
    virtual void GetRange(int &min, int &max)const; 
    virtual void SetRange(int min, int max);
    virtual    void ChangeDecimal(int step);
    virtual BOOL SetDecimal(int nDecimal);
    virtual int GetDecimal()const;


protected:
	int m_nDelta, m_nMinValue, m_nMaxValue;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
};


