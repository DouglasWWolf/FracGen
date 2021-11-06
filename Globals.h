#pragma once
#include "typedefs.h"

#include "Plotter.h"
#include <stack>
#include <vector>
#include <map>
#include "Shader.h"
#include "HueIndicator.h"
#include "cmspline.h"

using std::stack;
using std::vector;
using std::map;

#define VIEWPORT_SIZE  800
#define VIEWPORT_PIXELS (VIEWPORT_SIZE * VIEWPORT_SIZE)
#define MAX_THREADS 32
#define DEFAULT_DWELL 100
#define CWM_PROGRESS (CWM_THREAD + 1)

// User interface states
enum 
{
    UI_IDLE,
    UI_BUSY_VIEW,
    UI_BUSY_RENDER
};

// Progress states
enum
{
    PROGRESS_STITCHING = -1000,
    PROGRESS_ABORTING,
    PROGRESS_ABORTED,
    PROGRESS_FINISHED
};

//============================================================================
// Helper routines from Settings.cpp
//============================================================================
bool ReadSettings();
bool SaveSettings();
void CreateBuiltinPOI();
//============================================================================

//============================================================================
// WriteBmp() - Miscellaneous helper tools
//============================================================================
bool  WriteBmp(CString fn, pixel* image, U32 cols, U32 rows, U32 panel_width = 0);
//============================================================================


//============================================================================
//                   Some handy MFC related macros
//============================================================================
#define EnableControl(x)    GetDlgItem(x)->EnableWindow(true)
#define DisableControl(x)   GetDlgItem(x)->EnableWindow(false)
#define SetFocusTo(x)       GetDlgItem(x)->SetFocus()
#define PostDataToWindow()  UpdateData(false)
#define GetDataFromWindow() UpdateData(true)
//=======================================================================



//=======================================================================
// These are all of the rendering inputs concerning the plot region
//=======================================================================
struct plot_settings
{
    U32     rows;
    U32     columns;
    U32     panel_width;
    U32     cols_this_panel;
    U32     panel_number;
    T_COORD coord;
    double  pixel_size;
    pixel*  bitmap;
    U32     oversample;
};
//=======================================================================


//=======================================================================
// Definition of a "place of interest"
//=======================================================================
struct poi
{
    CString  name;
    unsigned fractal;
    bool     builtin;
    bool     center;
    double   real;
    double   imag;
    double   span;
};
//=======================================================================



extern plot_settings ps;

// This is the thread that manages all of the background tasks
extern CWorker  Worker;

// This bitmap holds the image we display in the viewport
extern pixel*   viewport;
extern pixel*   panel;

// This is the maximum size of the rendering panel, in pixels
extern U32      max_panel_size;

// This is the width (in pixels) of a full render
extern U32      render_width;

// This stores all of the fractal values for re-coloring the viewport
extern frac_value fractal[VIEWPORT_SIZE * VIEWPORT_SIZE];

// These are the class-threads that perform the point-plotting
extern CPlotter   Plotter[MAX_THREADS];

// This is a pixel shader
extern  CShader    Shader;

// This is the fixed-hue indicator
extern CHueIndicator fixed_hue_indicator;

// One imaginary value for every possible row in a panel
extern double imaginary[1000000];

// The co-ordinates of the upper-left corner of the viewport;
extern int viewport_ulx;
extern int viewport_uly;

// Anchor point for the lasso
extern int lasso_ancx;
extern int lasso_ancy;

// Extent of the lasso 
extern int lasso_extx;
extern int lasso_exty;

// This will be true when the left mouse button is down
extern bool lmb_down;

// This will be true when the user has lasso'd a region of the viewport
extern bool lassod;

// The number of logical CPU's that we have
extern U32 cpu_count;
// This will be true if we're in "single-click zoom" mode
extern BOOL auto_zoom ;

// A stack of coordinates
extern stack<T_COORD> coord_stack;

// This is the dwell limit
extern U32 dwell;

// The number of columns completed for this render
extern volatile U32 columns_completed;

// The current state of the user-interface
extern int ui_state;

// This will be true if the cursor is currently in the viewport
extern bool cursor_in_viewport;


// This is displayed when no point-of-interest is selected
extern const CString NONE_SELECTED;

// This is the currently selected place of interest
extern CString selected_poi;

// This contains a record for every place of interest
extern map<CString, poi> places;

// Flags that tell whether to invert one or more color channels
extern BOOL invert_r, invert_b, invert_g, invert_all;

// Will be true if colors should be converted to greyscale
extern BOOL greyscale;

// These are the cubic splines for building color gradients
extern CubicMonoSpline spline_red;
extern CubicMonoSpline spline_green;
extern CubicMonoSpline spline_blue;

// This will be true when we're aborting a render
extern bool aborting;
