#include "stdafx.h"
#include "SavePoiDlg.h"
#include "Globals.h"
#include "resource.h"


//=========================================================================================================
// Constructor() - Called every time we create this dialog
//=========================================================================================================
CSavePoiDlg::CSavePoiDlg(CWnd* pParent) : CDialogEx(IDD_SAVEPOI_DLG, pParent) {}
//=========================================================================================================


//=========================================================================================================
// DoDataExchange() - Swaps data between GUI and variables
//=========================================================================================================
void CSavePoiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_NAME, m_name);
}
//=========================================================================================================


//=========================================================================================================
// OnInitDialog() - Initializes the main dialog
//=========================================================================================================
BOOL CSavePoiDlg::OnInitDialog()
{
    // Let the base-class do it's thing
	CDialogEx::OnInitDialog();

    // Make sure the name start out blank
    m_name.Empty();

    // Put the cursor in the "Name" edit field
    SetFocusTo(IDC_NAME);
  
    // "FALSE" means we've set the focus to our desired field.
	return FALSE; 
}
//=========================================================================================================


//=========================================================================================================
// ValidateName() - Validates the "m_name" field
//=========================================================================================================
bool ValidateName(CString name)
{
    // Blank names are not allowed
    if (name.IsEmpty())
    {
        AfxMessageBox(L"Must enter a name");
        return false;
    }

    // If this name already exists, complain
    if (places.find(name) != places.end())
    {
        AfxMessageBox(L"This name already exists, please select another");
        return false;
    }

    // If we get here, all is well
    return true;
}
//=========================================================================================================


//=========================================================================================================
// OnOK() - Called when the user presses the OK button
//=========================================================================================================
void CSavePoiDlg::OnOK()
{
    // Fetch the data field from the GUI
    UpdateData(TRUE);
    
    // Trim leading and trailing spaces
    m_name.TrimLeft();
    m_name.TrimRight();

    // If the name is valid, we're done
    if (ValidateName(m_name)) EndDialog(1);   
}
//=========================================================================================================



//=========================================================================================================
// OnCancel() - Called when the user presses the CANCEL button
//=========================================================================================================
void CSavePoiDlg::OnCancel() {EndDialog(0);}
//=========================================================================================================