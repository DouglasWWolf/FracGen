#include "stdafx.h"
#include "Globals.h"
#include "Plotter.h"


// This is the thread that manages all of the background tasks
CWorker  Worker;

// Memory that holds the viewport image
pixel*   viewport;
pixel*   panel;

// This is the maximum size of the rendering panel, in pixels
U32      max_panel_size;

// This is the width (in pixels) of a full render
U32      render_width = 4000;

// This stores all of the fractal values for re-coloring the viewport
frac_value fractal[VIEWPORT_SIZE * VIEWPORT_SIZE];

// This contains all of the settings needed for a plot
plot_settings ps;

// These are the class-threads that perform the point-plotting
CPlotter    Plotter[MAX_THREADS];

// This is a pixel shader
CShader    Shader;

// This is the fixed-hue indicator
CHueIndicator fixed_hue_indicator;

// One imaginary value for every possible row in a panel
double imaginary[1000000];

// Anchor point for the lasso
int lasso_ancx;
int lasso_ancy;

// Extent of the lasso 
int lasso_extx;
int lasso_exty;

// The co-ordinates of the upper-left corner of the viewport;
int viewport_ulx;
int viewport_uly;

// This will be true when the left mouse button is down
bool lmb_down;

// This will be true when the user has lasso'd a region of the viewport
bool lassod;

// The number of logical CPU's that we have
U32  cpu_count;

// This will be true if we're in "single-click zoom" mode
BOOL auto_zoom = false;

// A stack of coordinates
stack<T_COORD> coord_stack;

// This is the dwell limit
U32 dwell = DEFAULT_DWELL;

// The number of columns completed for this render
volatile U32 columns_completed;

// The current state of the user-interface
int ui_state = UI_IDLE;

// This will be true if the cursor is currently in the viewport
bool cursor_in_viewport = false;

// This is displayed when no point-of-interest is selected
const CString NONE_SELECTED = L"------ None Selected ------";

// This is the currently selected place of interest
CString selected_poi = NONE_SELECTED;

// This contains a record for every place of interest
map<CString, poi> places;

// Flags that tell whether to invert one or more color channels
BOOL invert_r, invert_b, invert_g, invert_all;

// Will be true if colors should be converted to greyscale
BOOL greyscale;

// These are the cubic splines for building color gradients
CubicMonoSpline spline_red;
CubicMonoSpline spline_green;
CubicMonoSpline spline_blue;

// This will be true when we're aborting a render
bool aborting;
