#pragma once

//============================================================================
//    These are STL headers required by various WinUtils headers
//============================================================================
#include <map>
using std::map;
#include <vector>
using std::vector;
#include <stack>
using std::stack;
#include <memory>
using std::auto_ptr;
//============================================================================


//============================================================================

//============================================================================
// These popup a ::Messagebox type dialog-box
//============================================================================
int     Popup(CString fmt, ...);
int     Popup(int Flags, CString fmt, ...);
void    SetPopupTitle(CString s);
void    SetDefaultPopupTitle(CString s);
//============================================================================

//============================================================================
WORD CRC16(BYTE* pData, int iLength, WORD wInitialCRC);
CString ComputerID();
//============================================================================




//============================================================================
//          These convert various data types into a CString
//============================================================================
CStringA toCStringA(const wchar_t* s);

CString toCString(char* fmt, ...);
CString toCString(int    v, char* fmt = "%i");
CString toCString(double v, char* fmt = "%1.2lf");
CString toCString(bool   v);
//============================================================================

//============================================================================
//                 These restart and shutdown the PC
//============================================================================
void SystemShutdown();
void SystemRestart();
//============================================================================

//============================================================================
// debugf() - printf to the debug window
//============================================================================
void debugf(CString fmt, ...);
//============================================================================


//============================================================================
// Handy String processing routines
//============================================================================
inline char*  Chomp(char* in)          // Removes cr/lf from the end of a line
{
    char*   p;
    p = strchr(in, 10); if (p) *p = 0;
    p = strchr(in, 13); if (p) *p = 0;
    return in;
}

inline wchar_t* Chomp(wchar_t* in)
{
    wchar_t*  p;
    p = wcschr(in, 10); if (p) *p = 0;
    p = wcschr(in, 13); if (p) *p = 0;
    return in;
}

inline CString MakeUpper(CString s) {s.MakeUpper(); return s;}
inline CString MakeLower(CString s) {s.MakeLower(); return s;}
inline CString TrimAll  (CString s) {s.TrimRight(); s.TrimLeft(); return s;}
//============================================================================



//============================================================================
// This is used to fetch the number of elements in an array
//============================================================================
#define sizeofarray(x) (sizeof(x) / sizeof(x[0]))
#define sizeofa(x) (sizeof x / sizeof x[0])
//============================================================================

//============================================================================
// This should have been a part of the language!
//============================================================================
#define until(x) while(!(x))
//============================================================================


//============================================================================
// WUTime() - An extension of CTime that does useful things
//============================================================================
class CUTime : public COleDateTime
{
public:

    // Default constructor builds a "zero day" timestamp
    CUTime()
    {
        SetDateTime(1970,1,1, 0,0,0);
    }

    // Constructs a CUTime from a CTime
    CUTime(CTime n)
    {
        SetDateTime
        (
            n.GetYear(),
            n.GetMonth(),
            n.GetDay(),
            n.GetHour(),
            n.GetMinute(),
            n.GetSecond()
        );
    }

    // Returns a string of the current time
    static CString Now()
    {
        return GetCurrentTime().Format(L"%H:%M:%S");
    }

    // Returns a string of the current date
    static CString Today()
    {
        return GetCurrentTime().Format(L"%m/%d/%y");
    }
    
    // Records this moment as an internal timestamp
    void   Mark() 
    {
       *(COleDateTime*)this = COleDateTime::GetCurrentTime();
    }

    // Returns a string of the internal timestamp date
    CString Date()
    {
        CString t = Format(L"%m/%d/%y");
        if (t.IsEmpty()) return L"00/00/00";
        return t;
    }
    
    // Returns a date of the internal timestamp time
    CString Time()
    {
        CString t = Format(L"%H:%M:%S");
        if (t.IsEmpty()) return L"00:00:00";
        return t;
    }

    // Returns the entire timestamp as a string
    CString Timestamp()
    {
        CString t = Format(L"%m/%d/%y %H:%M:%S");
        if (t.IsEmpty()) return L"00/00/00 00:00:00";
        return t;
    }

    // Returns a sortable date string
    CString Sortable()
    {
        return Format(L"%y/%m/%d %H:%M:%S");
    }
};
//============================================================================






//============================================================================
//     These are color constants for use various user-interface classes
//============================================================================
const int  CLR_LIGHT_GRAY  = RGB(192,192,192);    
const int  CLR_DARK_GRAY   = RGB(130,130,130);   
const int  CLR_BLACK       = RGB(0,0,0);
const int  CLR_YELLOW      = RGB(0,255,255);
const int  CLR_GREEN       = RGB(0,255,0);
const int  CLR_KELLY_GREEN = RGB(0,255,120);
const int  CLR_WHITE       = RGB(255,255,255);
const int  CLR_RED         = RGB(255,0,0);
const int  CLR_BLUE        = RGB(0,0,255);
const int  CLR_BLUEISH     = RGB(153,209,242);
const int  CLR_YELLOWISH   = RGB(253,200,77);
const int  CLR_MILKYISH    = RGB(244,249,198);
const int  CLR_DARK_GREEN  = RGB(0,128,0);
const int  CLR_ORANGE      = RGB(255,120,0);
const int  CLR_DIALOG_GREY = RGB(236, 233, 216);
//============================================================================



//============================================================================
//                   Some handy MFC related macros
//============================================================================
#define EnableControl(x)    GetDlgItem(x)->EnableWindow(true)
#define DisableControl(x)   GetDlgItem(x)->EnableWindow(false)
#define SetFocusTo(x)       GetDlgItem(x)->SetFocus()
#define PostDataToWindow()  UpdateData(false)
#define GetDataFromWindow() UpdateData(true)
//============================================================================
