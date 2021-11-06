#pragma once
#include "stdafx.h"
//=========================================================================================================
// CSavePoiDlg() - Describes the "Save a Place of Interest" dialog box
//=========================================================================================================
class CSavePoiDlg : public CDialogEx
{
public:

    // Constructor
	CSavePoiDlg(CWnd* pParent = NULL);

    // Called when the user presses the OK button
    void    OnOK();

    // Called when the user presses the CANCEL button
    void    OnCancel();

    // The name that the user wants to save this point as
    CString m_name;

protected:

	void    DoDataExchange(CDataExchange* pDX);	
    BOOL    OnInitDialog();
};
//=========================================================================================================

