////////////////////////////////////////////////////////
//  Copyright (c) 2014 Cepheid Corporation
//
//  DisplayBox.h
//
//  Class definition for an extended listbox used 
//  for display
////////////////////////////////////////////////////////

#ifndef _DISPLAYBOX_H_
#define _DISPLAYBOX_H_


//============================================================================
// To use this class, the listbox control in your GUI should have these
// properties set:
//
// In The Style-tab:  "Selection"   = None
//                    "Owner Draw"  = Variable
//                    "Has Strings" = Checked
//
//============================================================================




//============================================================================
// class DisplayBox - Describes a scrollable listbox type object that can
//                    display strings of various colors
//============================================================================
class DisplayBox : public CListBox
{
public:

    // Removes all strings from the display-box
    void    Clear();
    
    // Adds a line of text to the display
    void    Display(LPCTSTR lpszItem, int rgb = 0);

    
protected:
    
    int AddString(LPCTSTR lpszItem);                // Adds a string to the list box
    int AddString(LPCTSTR lpszItem, COLORREF rgb);  // Adds a colored string to the list box
    int InsertString(int nIndex, LPCTSTR lpszItem); // Inserts a string to the list box
    int InsertString(int nIndex, LPCTSTR lpszItem, COLORREF rgb);   // Inserts a colored string to the list box
    void SetItemColor(int nIndex, COLORREF rgb);    // Sets the color of an item in the list box
    int EnableWindow(BOOL bState);


    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    DECLARE_MESSAGE_MAP()
};
//============================================================================

#endif
