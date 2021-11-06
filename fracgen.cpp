//=========================================================================================================
// FracGen - Mandlebrot fractal generator
//
// Icons created with http://icoconvert.com/image_to_icon_converter/
//=========================================================================================================
#include "stdafx.h"
#include "resource.h"
#include "typedefs.h"
#include "History.h"
#include "DisplayBox.h"
#include "Globals.h"
#include "CThread.h"
#include "SpecFile.h"
#include "SavePoiDlg.h"
#include "HueIndicator.h"
#include <memory>
#include <vector>
#include <map>

using std::vector;
using std::auto_ptr;


//=========================================================================================================
// CViewport - a CStatic class that we use for painting a bitmap into the viewport
//=========================================================================================================
class CViewport : public CStatic
{
private:
    void OnPaint( );
    
	DECLARE_MESSAGE_MAP()
};
//=========================================================================================================

//=========================================================================================================
// CMainDlg() - Describes the main dialog box
//=========================================================================================================
class CMainDlg : public CDialogEx
{
public:

	CMainDlg(CWnd* pParent = NULL);	// standard constructor

protected:

    // Printf-style access to the main output window
    void    wPrintf(int iColor, CString fmt, ...);
    LRESULT ThPrintf(WPARAM iSite, LPARAM Value);

    void    OnRedraw();
    void    OnBack();
    void    OnRestart();
    void    OnRender();
    void    OnAbort();
    void    OnExit();
    void    OnAutoZoom();
    void    OnPlaces();
    void    OnColorScheme();
    void    OnSavePOI();
    void    OnFractal();
    void    OnInvertRGB();
    void    OnInvertAll();
    void    OnGreyscale();
    void    DrawViewport();
    void    ReshadeViewport();
    void    CenterZoom(bool zoom_in);
    void    Shade(double* fractal, pixel* image, U32 panel_area);
    void    SetUI(int state);
    U32     GetOversampleFromGUI();
    LRESULT OnThStop(WPARAM iSite, LPARAM value);
    LRESULT OnProgress(WPARAM iSite, LPARAM value);

protected:

	void    DoDataExchange(CDataExchange* pDX);	
    BOOL    OnInitDialog();
    void    OnLButtonDown(UINT nFlags, CPoint point);
    void    OnLButtonUp  (UINT nflags, CPoint point);
    void    OnMouseMove  (UINT nFlags, CPoint point);
    BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    void    OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    BOOL    PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()

    DisplayBox    m_window;
    CViewport     m_viewport;
    HCURSOR       m_cursor;
 
};
//=========================================================================================================


//============================================================================================================
// GetLogicalProcessorCount() - Returns the number of logical processors available
//============================================================================================================
unsigned int GetLogicalProcessorCount()
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi[100];
    DWORD buflen = sizeof lpi;
    unsigned int physical_cores = 0, logical_cores = 0;

    // The call we're about to do to "GetModuleHandle" will return us one of these
    typedef BOOL (WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

    // Get a handle to the "kernel32" O/S module;
    HMODULE hm = GetModuleHandle(TEXT("kernel32"));

    // Get a pointer to the "GetLogicalProcessorInformation" system call
    LPFN_GLPI glpi = (LPFN_GLPI) GetProcAddress(hm, "GetLogicalProcessorInformation");

    // If we couldn't find that system call, assume we only have one logical processor
    if (glpi == nullptr) return 1;

    // Fetch the list of processor cores
    if (!glpi(lpi, &buflen)) return 1;

    // Find out how many processor-core descriptors were fetched
    int descriptor_count = buflen / sizeof SYSTEM_LOGICAL_PROCESSOR_INFORMATION;

    // Loop through each descriptor record...
    for (int d=0; d<descriptor_count; ++d)
    {
        // Get a handy reference to this descriptor
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION& pi = lpi[d];

        // We're only interested in Core Processor records
        if (pi.Relationship != RelationProcessorCore) continue;
       
        // We have one more physical core
        ++physical_cores;
       
        // There will be one bit set in the processor-mask for every logical core this physical core has
        for (int bit=0; bit<32; ++bit)
        {
            U32 mask = (1 << bit);
            if (pi.ProcessorMask & mask) ++logical_cores;
        }
    }

    // Tell the caller how many logical processors are present on this CPU
    return logical_cores;
}
//=========================================================================================================

//=========================================================================================================
// OnPaint() - Called anytime the viewport image must be repainted
//=========================================================================================================
void CViewport::OnPaint()
{
    CPaintDC paintDC(this);
    CBitmap  bmp;
    CDC      memDC;

    // Create a bitmap that is compatible with the screen
    bmp.CreateCompatibleBitmap(&paintDC, VIEWPORT_SIZE, VIEWPORT_SIZE);

    // Stuff our bitmap with the pixels of our image
    bmp.SetBitmapBits(VIEWPORT_PIXELS * sizeof pixel, viewport);
    
    // Create a device-context in memory and stuff it with our bitmap
    memDC.CreateCompatibleDC(&paintDC);
    CBitmap *pOldbmp = memDC.SelectObject(&bmp);
    paintDC.BitBlt(0,0, VIEWPORT_SIZE, VIEWPORT_SIZE ,&memDC,0,0,SRCCOPY);
    memDC.SelectObject(pOldbmp);

    // If the left mouse button is down or if we have a lasso'd region
    if (lmb_down || lassod)
    {
        CPen pen(PS_DOT, 1, RGB(123, 0, 0));
        paintDC.SelectObject(GetStockObject(NULL_BRUSH));
        paintDC.SelectObject(&pen);
        paintDC.Rectangle(lasso_ancx, lasso_ancy, lasso_extx, lasso_exty);
    }
}
//=========================================================================================================


//=========================================================================================================
// Message Map for the main dialog
//=========================================================================================================
BEGIN_MESSAGE_MAP(CViewport, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()
//=========================================================================================================


//=========================================================================================================
//            Exactly one instance of our application program
//=========================================================================================================
class CApp : public CWinApp {public: BOOL InitInstance();} theApp;
//=========================================================================================================


//=========================================================================================================
// AllocatePanel() - Allocate as much memory as we can for the rendering panel
//=========================================================================================================
U32 AllocatePanel()
{
    // 500 million pixels is the largest panel we can allocate
    U32 count = 500000000;

    for (int i = 0; i < 499; ++i)
    {
        try
        {
            panel = new pixel[count * sizeof pixel];
            if (panel != nullptr) break;
        }
        catch (...) {}
        count -= 1000000;
    }

    // Tell the caller how many pixels we allocated for the panel
    return count;
}
//=========================================================================================================



//=========================================================================================================
// InitInstance() - The main-line code for the application
//=========================================================================================================
BOOL CApp::InitInstance()
{

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	AfxEnableControlContainer();

    // Count the number of logical processors we have
    cpu_count = GetLogicalProcessorCount();

    // Allocate enough memory for the viewport
    viewport = new pixel[VIEWPORT_SIZE * VIEWPORT_SIZE];

    // Allocate as much memory as we can for a rendering panel
    max_panel_size = AllocatePanel();

    // And execute the main dialog
	CMainDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return FALSE;
}
//=========================================================================================================


//=========================================================================================================
// Constructor() - Called once to create the main dialog box
//=========================================================================================================
CMainDlg::CMainDlg(CWnd* pParent) : CDialogEx(IDD_MAIN_DLG, pParent) {}
//=========================================================================================================


//=========================================================================================================
// Message Map for the main dialog
//=========================================================================================================
BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)

    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_HSCROLL()

    ON_BN_CLICKED   (IDC_REDRAW,     OnRedraw     )
    ON_BN_CLICKED   (IDC_BACK,       OnBack       )
    ON_BN_CLICKED   (IDC_RESTART,    OnRestart    )
    ON_BN_CLICKED   (IDC_BIGPLOT,    OnRender     )
    ON_BN_CLICKED   (IDC_ABORT,      OnAbort      )
    ON_BN_CLICKED   (IDC_AUTOZOOM,   OnAutoZoom   )
    ON_BN_CLICKED   (IDC_SAVE_POI,   OnSavePOI    )
    ON_BN_CLICKED   (IDC_INVERT_R,   OnInvertRGB  )
    ON_BN_CLICKED   (IDC_INVERT_G,   OnInvertRGB  )
    ON_BN_CLICKED   (IDC_INVERT_B,   OnInvertRGB  )
    ON_BN_CLICKED   (IDC_INVERT_ALL, OnInvertAll  )
    ON_BN_CLICKED   (IDC_GREYSCALE,  OnGreyscale  )
    ON_BN_CLICKED   (IDC_EXIT,       OnExit       )
    ON_CBN_SELCHANGE(IDC_PLACES,     OnPlaces     )
    ON_CBN_SELCHANGE(IDC_CLR_SCHEME, OnColorScheme)
    ON_CBN_SELCHANGE(IDC_FRACTAL,    OnFractal    )
    

    ON_MESSAGE(CWM_PRINTF,   ThPrintf  )
    ON_MESSAGE(CWM_TH_STOP,  OnThStop  )
    ON_MESSAGE(CWM_PROGRESS, OnProgress)

END_MESSAGE_MAP()
//=========================================================================================================


//=========================================================================================================
// DoDataExchange() - Swaps data between GUI and variables
//=========================================================================================================
void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Text    (pDX, IDC_DWELL,        dwell       );
    DDX_CBString(pDX, IDC_PLACES,       selected_poi);
    DDX_Check   (pDX, IDC_AUTOZOOM,     auto_zoom   );
    DDX_Check   (pDX, IDC_INVERT_R,     invert_r    );
    DDX_Check   (pDX, IDC_INVERT_G,     invert_g    );
    DDX_Check   (pDX, IDC_INVERT_B,     invert_b    );
    DDX_Check   (pDX, IDC_INVERT_ALL,   invert_all  );
    DDX_Check   (pDX, IDC_GREYSCALE,    greyscale   );
    DDX_Text    (pDX, IDC_RENDER_WIDTH, render_width);
}
//=========================================================================================================


//=========================================================================================================
// ReshadeViewport() - Repaints the viewport view using the current shader settings
//=========================================================================================================
void CMainDlg::ReshadeViewport()
{
    U32 i;

    // Tell the background threads to peform a reshade
    for (i = 0; i < cpu_count; ++i) Plotter[i].Start(MT_RESHADE);

    // Wait for the background threads to finish 
    for (i = 0; i < cpu_count; ++i) Plotter[i].Wait();

    // Force a repaint of the viewport
    GetDlgItem(IDC_VIEWPORT)->Invalidate(false);
}
//=========================================================================================================


//=========================================================================================================
// OnInitDialog() - Initializes the main dialog
//=========================================================================================================
BOOL CMainDlg::OnInitDialog()
{
    CString    s;
    CComboBox* pCB;

    // Start all of the computation threads
    for (U32 i=0; i<cpu_count; ++i)
    {
        Plotter[i].SetThreadID(i);
        Plotter[i].Init();
        Plotter[i].Spawn(GetSafeHwnd());
    }

    // Let the base-class do it's thing
	CDialogEx::OnInitDialog();

    // Create a title for our main dialog window
    s.Format(L"Mandelbrot Explorer v%s - (c) 2017 Douglas Wolf", VERSION_BUILD);
    SetWindowText(s);

    // Subclass the dialog controls that we have customized
    m_window.SubclassDlgItem(IDC_WINDOW, this);
    m_viewport.SubclassDlgItem(IDC_VIEWPORT, this);
    fixed_hue_indicator.SubclassDlgItem(IDC_HUE, this);

    // Create the names of the color schemes
    Shader.InitSchemeNames((CComboBox*)GetDlgItem(IDC_CLR_SCHEME));

    // Tell the shader to use the default color scheme
    Shader.SetScheme(CS_DEFAULT);

    // Set up the fixed-hue slider
    CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_FIXED_HUE);
    pSlider->SetRange(0, 1000);
    pSlider->SetPos(585);
    Shader.SetFixedHue(.585);

    // Tell the user how many cores we have
    wPrintf(0, L"%i logical CPU cores found", cpu_count);

    // Set up the "Oversampling" combo-box
    pCB = (CComboBox*)GetDlgItem(IDC_OVERSAMPLE);
    pCB->AddString(L" No Oversample");
    pCB->AddString(L" 4x Oversample");
    pCB->AddString(L" 9x Oversample");
    pCB->SetCurSel(0);

    // Compute the initial viewport
    CPlotter::SetFractal(0);
    DrawViewport();

    // Set up the fractal-selector combo box
    pCB = ((CComboBox*)GetDlgItem(IDC_FRACTAL));
    pCB->AddString(L" Mandelbrot");
    pCB->AddString(L"Julia Set #1");
    pCB->SetCurSel(0);


    // Create the list of built-in places of interest
    CreateBuiltinPOI();

    // Read in the settings file
    ReadSettings();

    // Add all of the places names to the appropriate dialog control
    pCB = (CComboBox*)GetDlgItem(IDC_PLACES);
    for (auto it=places.begin(); it != places.end(); ++it) pCB->AddString(L" " + it->second.name);
    pCB->SelectString(0, L" " + NONE_SELECTED);

 
    // Determine the co-ordinates of the viewport
    RECT rect;
    GetDlgItem(IDC_VIEWPORT)->GetWindowRect(&rect);
    ScreenToClient(&rect);
    viewport_ulx = rect.left;
    viewport_uly = rect.top;

    // We start out with an ordinary cursor
    m_cursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);

    // Give the user some hints
    wPrintf(0, L"Hint: Shift-click to re-center the image on the selected point");
    wPrintf(0, L"Hint: PageUp to zoom in by 2X.  PageDn to zoom out by 2X"); 
  
	return TRUE;  // return TRUE  unless you set the focus to a control
}
//=========================================================================================================


//=========================================================================================================
// GetOversampleFromGUI() - Returns the oversampling factor by reading the GUI control
//=========================================================================================================
U32 CMainDlg::GetOversampleFromGUI()
{
    // These are the possible oversampling counts that we can return
    U32 count[] = { 0, 4, 9 };

    // Fetch the current index from the IDC_OVERSAMPLE combo-box
    int index = ((CComboBox*)GetDlgItem(IDC_OVERSAMPLE))->GetCurSel();

    // And hand the caller the corresponding oversample count
    return count[index];
}
//=========================================================================================================



//=========================================================================================================
// OnSetCursor() - Called whenever the cursor could potentially change
//=========================================================================================================
BOOL CMainDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    ::SetCursor(m_cursor);
    return TRUE;
}
//=========================================================================================================


//=========================================================================================================
// wPrintf() - Displays site-specific printf() style data into the output
//             window.
//
// Passed:  iColor = a CLR constant from CephUtilsImp.h
//          fmt    = print style formatting and data
//=========================================================================================================
void CMainDlg::wPrintf(int iColor, CString fmt, ...)
{
	CString	out;

    // This is a pointer to a variable argument list
    va_list FirstArg;

    // Point to the first argument after the "fmt" parameter
    va_start(FirstArg, fmt);

	// Perform a "printf" of our arguments into "out"
	out.FormatV(fmt, FirstArg);

    // Tell the system we're done with FirstArg
    va_end(FirstArg);

    // Output to the message window
    m_window.Display(out, iColor);
}
//=========================================================================================================


//=========================================================================================================
// ThPrintf() - Handler for the WM_TH_PRINT message
//
// Notes: "Value" is actually a pointer to a CThPrintfMsg object
//========================================================================================================
LRESULT CMainDlg::ThPrintf(WPARAM iSite, LPARAM Value)
{
    // Make "Value" an auto_ptr to a CThPrintfMsg object
    auto_ptr<CThPrintfMsg> pMsg((CThPrintfMsg*)Value);

    // Fetch the text that we'll be displaying
    CString sText = pMsg->sText;

    // Print the string in the window
    wPrintf(pMsg->iColor, L"%s", sText);

    // This return value is just to keep the compiler happy
    return 0;
}
//=========================================================================================================


//=========================================================================================================
// OnHScroll() - Handles the "fixed hue" slider
//=========================================================================================================
void CMainDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CSliderCtrl* pSlider = reinterpret_cast<CSliderCtrl*>(pScrollBar);

    // If this is a slider-movement message, update the indicator
    if (nSBCode == TB_THUMBTRACK)
    {
        Shader.SetFixedHue(nPos / 1000.0);
        return;
    }

    // When the user lets go of the thumb-bar, update the viewport
    if (nSBCode == TB_THUMBPOSITION)
    {
        ReshadeViewport();
        return;
    }
  
}
//=========================================================================================================




//=========================================================================================================
// MouseToPlane() - Converts mouse coordinates (relative to the viewport) to a complex number
//=========================================================================================================
complex MouseToPlane(int x, int y)
{
    complex result;

    // Fetch our current co-ordinates
    T_COORD current = coord_stack.top();

    // Figure out where the upper-left corner of the viewport is
    double ulc_real = current.center.real - current.span.real / 2;
    double ulc_imag = current.center.imag + current.span.imag / 2;

    // And compute the plane-coordinates that correspond to the mouse coordinates
    result.real = ulc_real + current.span.real * x / VIEWPORT_SIZE;
    result.imag = ulc_imag - current.span.imag * y / VIEWPORT_SIZE;

    // And the resulting set of coordinates to the caller
    return result;
}
//=========================================================================================================


//=========================================================================================================
// OnLButtonDown() - Called when the user presses the mouse-button 
//=========================================================================================================
void CMainDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    T_COORD next;

    // User isn't allowed to select a region while we're computing
    if (ui_state != UI_IDLE) return;

    // If the shift-key was down, we're just recentering the image
    bool shift = (GetKeyState(VK_SHIFT) & 0xFFF0) != 0;

    // Adjust our mouse-coordinates to make them local to the bitmap control
    point.x -= viewport_ulx;
    point.y -= viewport_uly;

    // If the mouse cursor isn't over the viewport, do nothing
    if (point.x < 0 || point.x >= VIEWPORT_SIZE) return;
    if (point.y < 0 || point.y >= VIEWPORT_SIZE) return;
   
    // We have no area lassod
    lassod = false;

    // If we're recentering the image...
    if (shift || auto_zoom)
    {
        // Fetch the coordinates that are currently displayed
        T_COORD current = coord_stack.top();
        
        // The center of our new drawing will be where the mouse is pointing
        next.center = MouseToPlane(point.x, point.y);

        // If we're auto-zooming, our center stays the same
        if (auto_zoom)
        {
            next.span.real = current.span.real / 2;
            next.span.imag = current.span.imag / 2;
        }

        // If the shift key was down, we're just recentering, not adjusting the span
        if (shift) next.span = current.span;

        // Place these newly centered coordinates on the stack and redraw the viewport
        coord_stack.push(next);

        // And draw the newly centered viewport
        DrawViewport();

        // We're done here
        return;
    }

    // Determine where in the window we are
    lasso_ancx = lasso_extx = point.x;
    lasso_ancy = lasso_exty = point.y;
  
    // The left mouse button is down
    lmb_down = true;

    // Force a repaint of the viewport
    GetDlgItem(IDC_VIEWPORT)->Invalidate(false);
}
//=========================================================================================================


//=========================================================================================================
// OnMouseMove() - Called when the user presses the mouse-button 
//=========================================================================================================
void CMainDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    bool now_in_viewport = true;

    // Adjust our mouse-coordinates to make them local to the bitmap control
    point.x -= viewport_ulx;
    point.y -= viewport_uly;

     // Limit the mouse position to within the viewport
    if (point.x < 0) {now_in_viewport = false; point.x = 0;}
    if (point.y < 0) {now_in_viewport = false; point.y = 0;}
    if (point.x >= VIEWPORT_SIZE) {now_in_viewport = false; point.x = VIEWPORT_SIZE;}
    if (point.y >= VIEWPORT_SIZE) {now_in_viewport = false; point.y = VIEWPORT_SIZE;}

    // If the cursor has just moved either into or out of the viewport
    if (now_in_viewport != cursor_in_viewport)
    {
        // Keep track of whether the cursor is in the viewport
        cursor_in_viewport = now_in_viewport;

        // Load the correct cursor
        if (cursor_in_viewport && auto_zoom)
            m_cursor = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_CROSSHAIRS));
        else
            m_cursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
    }

    // If the left-mouse button isn't down, we're not selecting a viewport region
    if (!lmb_down) return;

    // Record the extent of the lasso
    lasso_extx = point.x;
    lasso_exty = point.y;

    // Force a repaint of the viewport
    GetDlgItem(IDC_VIEWPORT)->Invalidate(false);
}
//=========================================================================================================



//=========================================================================================================
// OnLButtonUp() - Called when the user releases the mouse-button 
//=========================================================================================================
void CMainDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    int temp;

    // If the left-mouse button isn't down, we're not selecting a viewport region
    if (!lmb_down) return;

    // Adjust our mouse-coordinates to make them local to the bitmap control
    point.x -= viewport_ulx;
    point.y -= viewport_uly;

    // Limit the mouse position to within the viewport
    if (point.x < 0) point.x = 0;
    if (point.y < 0) point.y = 0;
    if (point.x >= VIEWPORT_SIZE) point.x = VIEWPORT_SIZE;
    if (point.y >= VIEWPORT_SIZE) point.y = VIEWPORT_SIZE;
    
    // The left mouse button is no longer down
    lmb_down = false;
    
    // Record the extent of the lasso
    lasso_extx = point.x;
    lasso_exty = point.y;

    // Force a repaint of the viewport
    GetDlgItem(IDC_VIEWPORT)->Invalidate(false);

    // If the box is user drew is tiny, we don't have a valid lasso
    if (abs(lasso_ancx - lasso_extx) < 10) return;
    if (abs(lasso_ancy - lasso_exty) < 10) return;

    // Ensure lasso_anc is the upper left corner and lasso_ext the lower_right corner
    if (lasso_ancx > lasso_extx)
    {
        temp = lasso_ancx;
        lasso_ancx = lasso_extx;
        lasso_extx = temp;
    }
    if (lasso_ancy > lasso_exty)
    {
        temp = lasso_ancy;
        lasso_ancy = lasso_exty;
        lasso_exty = temp;
    }

    // We have a valid lasso!
    lassod = true;
}
//=========================================================================================================

//=========================================================================================================
// GetLassodCoords() - Returns the spacial coordinates of the lasso'd region
//=========================================================================================================
T_COORD GetLassodCoords(bool square)
{
    T_COORD lasso;

    // Assume for the moment that this is just a redraw
    T_COORD current = coord_stack.top();

    // If there is nothing lassod, just return the current coordinates
    if (!lassod) return current;
      
    // Determine how wide and and high the lasso'd region is
    U32 pixel_span_x = lasso_extx - lasso_ancx;
    U32 pixel_span_y = lasso_exty - lasso_ancy;

    U32 center_x = lasso_ancx + pixel_span_x/2;
    U32 center_y = lasso_ancy + pixel_span_y/2;

    // If we're supposed to make the region square, find the average of the spans
    // And recompute the lasso centered on the original lasso
    if (square)
    {
        pixel_span_x = pixel_span_y = (pixel_span_x + pixel_span_y) / 2;
        lasso_ancx = center_x - pixel_span_x / 2;
        lasso_ancy = center_y - pixel_span_y / 2;
        lasso_extx = lasso_ancx + pixel_span_x;
        lasso_exty = lasso_ancy + pixel_span_y;
    }

    // Find the coordinates of the upper-left and lower-right corners of the lasso'd region
    complex ulc = MouseToPlane(lasso_ancx, lasso_ancy);
    complex lrc = MouseToPlane(lasso_extx, lasso_exty);
    
    // Find the center of the lasso
    lasso.center.real = (ulc.real + lrc.real) / 2;
    lasso.center.imag = (ulc.imag + lrc.imag) / 2;

    // Find the span of the lasso
    lasso.span.real = fabs(ulc.real - lrc.real);
    lasso.span.imag = fabs(ulc.imag - lrc.imag);

    // Hand the lasso'd coordinates to the caller
    return lasso;
}
//=========================================================================================================


//=========================================================================================================
// DrawViewport() - Draws the viewport using the specified co-ordinates
//=========================================================================================================
void CMainDlg::DrawViewport()
{
    // Fetch the value of the GUI fields
    UpdateData(true);

    // Turn off the user interface
    SetUI(UI_BUSY_VIEW);

    // Fetch the top set of coordinates on the stack
    T_COORD coord = coord_stack.top();

    // Set up the plot settings
    ps.bitmap          = viewport;
    ps.rows            = VIEWPORT_SIZE;
    ps.columns         = VIEWPORT_SIZE;
    ps.panel_width     = VIEWPORT_SIZE;
    ps.coord           = coord_stack.top();
    ps.pixel_size      = ps.coord.span.real / ps.columns;
    ps.oversample      = GetOversampleFromGUI();

    // Render the new view
    Worker.Spawn(GetSafeHwnd(), MT_PLOT);

    // There is no longer a lasso'd region
    lassod = false;
}
//=========================================================================================================


//=========================================================================================================
// OnInvertRGB() - Called whenever the user alters one of the Invert R, B, or G check-boxes
//=========================================================================================================
void CMainDlg::OnInvertRGB()
{
    // Fetch the value of the GUI fields
    UpdateData(true);

    // Find out whether we should turn "invert all" on or off
    invert_all = invert_r && invert_b && invert_g;

    // Update the "invert all" checkbox
    UpdateData(false);

    // And repaint the viewport
    ReshadeViewport();
}
//=========================================================================================================



//=========================================================================================================
// OnInvertAll() - Called whenever the user alters the "Invert all" checkbox
//=========================================================================================================
void CMainDlg::OnInvertAll()
{
    // Fetch the value of the GUI fields
    UpdateData(true);

    // Record whether or not to invert the individual R, G, and B channels
    invert_r = invert_b = invert_g = invert_all;

    // Update the "invert all" checkbox
    UpdateData(false);

    // And repaint the viewport
    ReshadeViewport();
}
//=========================================================================================================


//=========================================================================================================
// OnGreyscale() - Called whenever the user changes the state of the "Greyscale" checkbox
//=========================================================================================================
void CMainDlg::OnGreyscale()
{
    // Fetch the value of the GUI fields
    UpdateData(true);

    // And repaint the viewport
    ReshadeViewport();
}
//=========================================================================================================




//=========================================================================================================
// OnRender() - Renders the image to a file
//=========================================================================================================
void CMainDlg::OnRender()
{
    // Fetch the value of the GUI fields
    UpdateData(true);

    // Make sure the render width is something reasonable
    if (render_width < 10 || render_width > 1000000)
    {
        Popup(L"Render width must be between 10 and 1000000");
        return;
    }


    // This is how many columns the resulting image is going to have
    U32 cols = render_width;

    // Fetch the coordinates we're going to use
    T_COORD coord = GetLassodCoords(false);

    // Determine how many rows are going to be in this image
    U32 rows = (U32)(cols * coord.span.imag / coord.span.real + .5);

    // Make sure the whole thing will fit into memory
    if (rows > max_panel_size)
    {
        Popup(L"This is too big to fit into memory");
        return;
    }
    
    // Determine how many megapixels the fully rendered image will be
    double megapixels = rows * cols / 1000000.0;
    CString unit = L"Megapixels";

    // If we're over a gigapixel, change the units
    if (megapixels > 1000)
    {
        megapixels /= 1000.0;
        unit = L"Gigapixels";
    }

    // Show the user how big this will be and ask them if they are sure
    BOOL isOK = Popup(MB_OKCANCEL, L"The fully rendered image will be %u x %u (%1.3lf %s)"
                       L"\n\nAre you certain you wish to render this image?"    
                       , cols, rows, megapixels, unit);

    // If they said "no", we're done
    if (isOK == IDCANCEL) return;
    
    // Turn off the user interface
    SetUI(UI_BUSY_RENDER);
    
    // Determine the maximum width of a panel that will fit into our panel buffer
    U32 panel_width = max_panel_size / rows;

    // For convenience when writing/reading BMP files, round this down to a multiple of 4
    while (panel_width % 4) --panel_width;
    
    // Set up the plot settings
    ps.bitmap          = panel;
    ps.rows            = rows;
    ps.columns         = cols;
    ps.panel_width     = panel_width;
    ps.coord           = coord;
    ps.pixel_size      = ps.coord.span.real / ps.columns;
    ps.oversample      = GetOversampleFromGUI();

    // Render the new view
    Worker.Spawn(GetSafeHwnd(), MT_PLOT);

    // There is no longer a lasso'd region
    lassod = false;
}
//=========================================================================================================


//=========================================================================================================
// OnThStop() - Called when the Worker thread completes its computation
//========================================================================================================
LRESULT CMainDlg::OnThStop(WPARAM iSite, LPARAM Value)
{
     // Force a repaint of the viewport
    GetDlgItem(IDC_VIEWPORT)->Invalidate(false);

    // Turn the user interface back on
    SetUI(UI_IDLE);

    // This return value is just to keep the compiler happy
    return 0;
}
//=========================================================================================================


//=========================================================================================================
// OnAbort() - Called when the abort button is pressed
//=========================================================================================================
void CMainDlg::OnAbort()
{
    // Ask the user if they want to abort this render
    if (Popup(MB_YESNO, L"Are you certain you want to cancel the current render?") == IDNO) return;

    // If we're already idle, it means the render completed while the user was looking at the 
    // above dialog box
    if (ui_state == UI_IDLE) return;

    // Tell the user that we're aborting
    OnProgress(0, PROGRESS_ABORTING);

    // And set the flag that tells the computation threads to stop
    aborting = true;
}
//=========================================================================================================



//=========================================================================================================
// OnExit() - Called when the exit button is pressed
//=========================================================================================================
void CMainDlg::OnExit()
{
    const wchar_t* str = L"There is a render running.  "
                          "Are you certain you want to cancel it and exit?";

    // If we're in the middle of a render, ask the user if they're sure they want to exit
    if (ui_state == UI_BUSY_RENDER && Popup(MB_YESNO, str) == IDNO) return;

    // We're done, end the dialog
    EndDialog(0);
}
//=========================================================================================================



//=========================================================================================================
// OnAutoZoom() - Called when he "Auto Zoom" check-box gets clicked
//=========================================================================================================
void CMainDlg::OnAutoZoom()
{
    // Fetch the value of the GUI fields
    UpdateData(true);

    // If there is a lasso on screen, turn it off
    lassod = false;

    // Force a repaint of the viewport
    GetDlgItem(IDC_VIEWPORT)->Invalidate(false);
}
//=========================================================================================================



//=========================================================================================================
// OnPlaces() - Called whenever the user selects an item from the "Places of Interest"
//=========================================================================================================
void CMainDlg::OnPlaces()
{
    T_COORD coord;

    // Fetch the value of the GUI fields
    UpdateData(true);

    // Get rid of the leading space that we insert when we build the list
    selected_poi.TrimLeft();

    // If none was selected, we're done
    if (selected_poi == NONE_SELECTED) return;

    // Fetch the record of this place of interest
    poi place = places[selected_poi];

    // Select the appropriate fractal
    CPlotter::SetFractal(place.fractal);

    // Copy the span from the POI record into our coordinate structure
    coord.span.real = place.span;
    coord.span.imag = place.span;

    // If the coordinates in this record describe the center of the image...
    if (place.center)
    {
        coord.center.real = place.real;
        coord.center.imag = place.imag;
    }

    // Otherwise coordinates in this record describe the upper-left corner
    else
    {
        coord.center.real = place.real + place.span / 2;
        coord.center.imag = place.imag - place.span / 2;
    }

    // Put these coordinates on the stack and redraw the viewport
    coord_stack.push(coord);
    DrawViewport();
}
//=========================================================================================================



//=========================================================================================================
// OnColorScheme() - Called whenever the user selects a new color scheme
//=========================================================================================================
void CMainDlg::OnColorScheme()
{
    // Get a pointer to our combo-box
    CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CLR_SCHEME);

    // Find out the coloring-scheme number
    int scheme = pCB->GetCurSel();

    // Set the appropriate color scheme
    Shader.SetScheme(scheme);

    // The fixed hue slider is only enabled when the shader scheme is "fixed hue"
    GetDlgItem(IDC_FIXED_HUE)->EnableWindow(scheme == CS_FIXED_HUE);

    // And repaint the viewport in our current color scheme
    ReshadeViewport();
}
//=========================================================================================================




//=========================================================================================================
// OnProgress() - Called when a progress notification is sent
//========================================================================================================
LRESULT CMainDlg::OnProgress(WPARAM iSite, LPARAM Value)
{
    CString s;

    // Format the string
    switch (Value)
    {
    case PROGRESS_STITCHING:
        s = L"Stitching...";
        break;

    case PROGRESS_FINISHED:
        s = L"Finished";
        break;

    case PROGRESS_ABORTING:
        s = L"Aborting";
        break;

    case PROGRESS_ABORTED:
        s = L"Aborted";
        break;

    default:
        s.Format(L"%i%%", Value);
    }

    // And display the message
    GetDlgItem(IDC_PROGRESS)->SetWindowText(s);

    // This return value is just to keep the compiler happy
    return 0;
}
//=========================================================================================================




//=========================================================================================================
// OnRedraw() - Redraws or zooms the current viewport
//=========================================================================================================
void CMainDlg::OnRedraw()
{
    // If there's a lasso'd region, put it's coordinates at the top of the stack
    if (lassod) coord_stack.push(GetLassodCoords(true));

    // Draw this most recent set of coordinates
    DrawViewport(); 
}
//=========================================================================================================


//=========================================================================================================
// OnBack() - Redraws the previous set of co-ordinates (zoom-out)
//=========================================================================================================
void CMainDlg::OnBack()
{
    // Do nothing if we've zoomed all the way out
    if (coord_stack.size() == 1) return;

    // Remove the coordinates at the top of the stack
    coord_stack.pop();
   
    // Draw this most recent set of coordinates
    DrawViewport(); 
}
//=========================================================================================================


//=========================================================================================================
// OnSavePOI() - Called whenever the user wants to save a new place of interest
//=========================================================================================================
void CMainDlg::OnSavePOI()
{
    CSavePoiDlg dlg;
    poi place;

    // Ask the user to enter a name.  If they cancel, do nothing
    if (!dlg.DoModal()) return;

    // Fetch the currently drawn set of coordinates
    T_COORD coord = coord_stack.top();

    // Fill in the fields of a place structure
    place.name    = dlg.m_name;
    place.fractal = ((CComboBox*)GetDlgItem(IDC_FRACTAL))->GetCurSel();
    place.builtin = false;
    place.center  = true;
    place.real    = coord.center.real;
    place.imag    = coord.center.imag;
    place.span    = coord.span.real;
 
    // Put this place of interest into our map of places
    places[place.name] = place;

    // Get a references to our GUI control
    CComboBox& cb = *(CComboBox*)GetDlgItem(IDC_PLACES);

    // Add this place to our combo-box and make it the current selection
    cb.AddString(L" " + place.name);
    cb.SelectString(0, L" " + place.name);

    // Save the settings file and complain if we can't
    if (!SaveSettings()) Popup(L"Failed to save settings file!");
}
//=========================================================================================================


//=========================================================================================================
// OnFractal() - Called whenever the user selects a new fractal type
//=========================================================================================================
void CMainDlg::OnFractal()
{
    // Find out what fractal type the user just selected
    U32 index = ((CComboBox*)GetDlgItem(IDC_FRACTAL))->GetCurSel();

    // Tell the plotter what kind of fractal to plot
    CPlotter::SetFractal(index);

    // And go draw the selected fractal
    OnRestart();
}
//=========================================================================================================


//=========================================================================================================
// OnRestart() - Redraws the fully zoomed out image
//=========================================================================================================
void CMainDlg::OnRestart()
{
    // Reset the dwell to the default
    dwell = DEFAULT_DWELL;

    // Reset to default oversample
    ((CComboBox*)GetDlgItem(IDC_OVERSAMPLE))->SetCurSel(0);

    // Place this dwell into the GUI field
    UpdateData(false);
    
    // Pop off everything except the very first set of coordinates
    while (coord_stack.size() > 1) coord_stack.pop();
    
    // Draw this most recent set of coordinates
    DrawViewport(); 
}
//=========================================================================================================


//=========================================================================================================
// CenterZoom() - Zooms in or out around the current center of the viewport
//=========================================================================================================
void CMainDlg::CenterZoom(bool zoom_in)
{
    // Fetch the coordinates that are current displayed
    T_COORD next, current = coord_stack.top();

    // Keep our current center
    next.center = current.center;

    // Determine what the scaling factor for zooming will be
    double scaling = (zoom_in) ? 0.5 : 2.0;

    // Change the size of the viewport by a factor of 2
    next.span.real = current.span.real * scaling;
    next.span.imag = current.span.imag * scaling;

    // Place these newly centered coordinates on the stack and redraw the viewport
    coord_stack.push(next);

    // And draw the newly centered viewport
    DrawViewport();

    // We're done here
    return;
}
//=========================================================================================================


//=========================================================================================================
// PreTranslateMessage() - Perform special handling for the escape key
//=========================================================================================================
BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
    // Throw away "Escape-key down" messages
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) return true;

    // If the user hit "Page Up" zoom in
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_PRIOR)
    {
        CenterZoom(true);
        return true;
    }

    // If the user hit "Page Down", zoom out
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_NEXT)
    {
        CenterZoom(false);
        return true;
    }

    // If "OnPreTranslateMessage" returned false, let CDialog do
    // normal message translation and processing
    return CDialogEx::PreTranslateMessage(pMsg);
}
//=========================================================================================================

 

//=========================================================================================================
// SetUI() - Sets the user-interfaces to one of the UI_XXX states
//=========================================================================================================
void CMainDlg::SetUI(int state)
{
    // Record what state the user interface is in
    ui_state = state;

    bool flag = (state == UI_IDLE);

    GetDlgItem(IDC_REDRAW      )->EnableWindow(flag);
    GetDlgItem(IDC_BACK        )->EnableWindow(flag);
    GetDlgItem(IDC_RESTART     )->EnableWindow(flag);
    GetDlgItem(IDC_BIGPLOT     )->EnableWindow(flag);
    GetDlgItem(IDC_DWELL       )->EnableWindow(flag);
    GetDlgItem(IDC_OVERSAMPLE  )->EnableWindow(flag);
    GetDlgItem(IDC_AUTOZOOM    )->EnableWindow(flag);
    GetDlgItem(IDC_PLACES      )->EnableWindow(flag);
    GetDlgItem(IDC_CLR_SCHEME  )->EnableWindow(flag);
    GetDlgItem(IDC_SAVE_POI    )->EnableWindow(flag);
    GetDlgItem(IDC_INVERT_R    )->EnableWindow(flag);
    GetDlgItem(IDC_INVERT_G    )->EnableWindow(flag);
    GetDlgItem(IDC_INVERT_B    )->EnableWindow(flag);
    GetDlgItem(IDC_INVERT_ALL  )->EnableWindow(flag);
    GetDlgItem(IDC_GREYSCALE   )->EnableWindow(flag);
    GetDlgItem(IDC_RENDER_WIDTH)->EnableWindow(flag);
    GetDlgItem(IDC_FIXED_HUE   )->EnableWindow(flag && Shader.GetScheme() == CS_FIXED_HUE);
    GetDlgItem(IDC_ABORT       )->EnableWindow(state == UI_BUSY_RENDER);


    // After re-enabling the user-interface, make sure the focus is 
    // somewhere that can capture keystrokes
    if (state == UI_IDLE) SetFocusTo(IDC_DWELL);
}
//=========================================================================================================
