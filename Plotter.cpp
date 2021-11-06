#include "stdafx.h"
#include "Plotter.h"
#include "Globals.h"
#include "Stitcher.h"
#include <math.h>

const double ONE_OVER_LOG2 = 1.44269504;


//=========================================================================================================
// Variables common to all instances of this class
//=========================================================================================================
volatile U32 CPlotter::m_next_column;
CCriticalSection columns_completed_cs;
//=========================================================================================================


//=========================================================================================================
// This points to a fractal iterator function
//=========================================================================================================
escape (*Iterator)(double real, double imag);
//=========================================================================================================



//=========================================================================================================
// square_and_add() - Squares a complex number 'n' and adds a complex constant 'c' to it
//=========================================================================================================
complex square_and_add(complex n, complex c)
{
    complex result;
    result.real = n.real * n.real - n.imag * n.imag;
    result.imag = 2 * n.real * n.imag;

    result.real += c.real;
    result.imag += c.imag;
    return result;
}
//=========================================================================================================



//=========================================================================================================
// Iterator_Mandelbrot() - Iterator for the Mandlebrot set
//=========================================================================================================
escape Iterator_Mandelbrot(double real, double imag)
{
    // Define this point on the complex plane
    complex c = { real, imag };

    // We begin our iterated complex value at c
    complex z = c;

    // So far we've done no iterations
    int iter = 0;

    // Iterate on z^2 + c...
    while (iter < (int)dwell)
    {
        // Keep track of how many iterations we do
        ++iter;

        // Compute the new value of 'z'
        z = square_and_add(z, c);

        // If our new point has gone out of bounds, keep track of how long it took
        if ((z.real * z.real + z.imag * z.imag) >= 4.0)
        {
            z = square_and_add(z, c);
            z = square_and_add(z, c);

            double abs_squared = z.real * z.real + z.imag * z.imag;

            return{ iter, abs_squared };
        }
    }

    // We never exceeded the escape radius
    return{ 0 , 0.0 };
}
//=========================================================================================================




//=========================================================================================================
// Iterator_Julia01() - Iterator for Julia Set #1
//=========================================================================================================
escape Iterator_Julia01(double real, double imag)
{
    complex z = { real, imag };
    complex c = { -0.8,  0.156 };
  
    // So far we've done no iterations
    int iter = 0;

    // Iterate on z^2 + c...
    while (iter < (int)dwell)
    {
        // Keep track of how many iterations we do
        ++iter;

        // Compute the new value of 'z'
        z = square_and_add(z, c);

        // If our new point has gone out of bounds, keep track of how long it took
        if ((z.real * z.real + z.imag * z.imag) >= 4.0)
        {
            z = square_and_add(z, c);
            z = square_and_add(z, c);

            double abs_squared = z.real * z.real + z.imag * z.imag;

            return{ iter, abs_squared };
        }
    }

    // We never exceeded the escape radius
    return{ 0 , 0.0 };
}
//=========================================================================================================





//=========================================================================================================
// Init() - Initialize the thread
//=========================================================================================================
void CPlotter::Init()
{
    CreatePipe(&m_hread_cmd, &m_hwrite_cmd, NULL, 0);
    CreatePipe(&m_hread_rsp, &m_hwrite_rsp, NULL, 0);
}
//=========================================================================================================


//=========================================================================================================
// Wait() - Waits for this thread to finish it's computation
//=========================================================================================================
void CPlotter::Wait()
{
    char buffer;

    ReadFile(m_hread_rsp, &buffer, 1, nullptr, nullptr);
}
//=========================================================================================================


//=========================================================================================================
// Start() -Starts the computations for this thread
//=========================================================================================================
void CPlotter::Start(char command)
{
    m_is_task_complete = false;
    WriteFile(m_hwrite_cmd, &command, 1, nullptr, nullptr);
}
//=========================================================================================================



//=========================================================================================================
// StartPanel() - Begins plotting an entire panel
//=========================================================================================================
void CPlotter::StartPanel(char command)
{
    // This is the next column number that will be issued for plotting
    m_next_column = 0;

    for (U32 i=0; i<cpu_count; ++i) Plotter[i].Start(command);
}
//=========================================================================================================


//=========================================================================================================
// SetFractal() - Determine which fractal we're going to plot
//=========================================================================================================
void CPlotter::SetFractal(U32 fractal)
{
    T_COORD mandelbrot = { -0.75, 0.0, 3.0, 3.0 };
    T_COORD julia      = {     0,   0, 3.5, 3.5 };

    // Get rid of any existing coordinates
    while (coord_stack.size()) coord_stack.pop();
    
    switch (fractal)
    {
    case 0:
        Iterator = Iterator_Mandelbrot;
        coord_stack.push(mandelbrot);
        break;

    case 1:
        Iterator = Iterator_Julia01;
        coord_stack.push(julia);
        break;
    }
}
//=========================================================================================================



//=========================================================================================================
// ThreadsCompleted() - Returns the number of threads that have completed their tasks
//=========================================================================================================
U32 CPlotter::ThreadsCompleted()
{
    U32 count = 0;

    // Get a count of how many threads have completed their task
    for (U32 i = 0; i < cpu_count; ++i) if (Plotter[i].m_is_task_complete) ++count;

    // And tell the caller how many threads have completed their task
    return count;
}
//=========================================================================================================




//=========================================================================================================
// IssueColumn() - Returns the number of the next column that required plotting
//
// Note: Column number returned is *relative to the current panel*
//=========================================================================================================
int CPlotter::IssueColumn()
{
    static CCriticalSection cs;
  
    // Assume for the moment that we are out of column numbers
    int result = -1;

    // Only one thread at a time is allowed to request a new column number
    cs.Lock();

    // If there is a column number available, it's our result
    if (m_next_column < ps.cols_this_panel) result = m_next_column++;

    // Allow other threads to run this routine
    cs.Unlock();

    // Hand the caller his column number
    return result;
}
//=========================================================================================================




//=========================================================================================================
// NotifyComplete() - Tell the master thread that this thread has completed it's task
//=========================================================================================================
void CPlotter::NotifyComplete()
{
    char dummy=0;
    WriteFile(m_hwrite_rsp, &dummy, 1, nullptr, nullptr);
    m_is_task_complete = true;
}
//=========================================================================================================

//=========================================================================================================
// Reshade() - Reshades the viewport window
//=========================================================================================================
void CPlotter::Reshade()
{
    // Figure out how many rows each thread needs to reshade
    int rows_per_thread = VIEWPORT_SIZE / cpu_count;

    // Determine which row is the first row this thread should operate on
    int first_row = m_ID * rows_per_thread;

    // Figure out how many rows this thread is responsible for reshading
    int rows_this_thread = (m_ID == cpu_count - 1) ? VIEWPORT_SIZE - first_row : rows_per_thread;

    // This is the total number of elements this thread is responsible for reshading
    int total_elements = rows_this_thread * VIEWPORT_SIZE;

    // Point to the first fractal row that this thread is responsible for
    frac_value* fvp = fractal + first_row * VIEWPORT_SIZE;
    
    // Point to the first pixel row that this thread is responsible for
    pixel* pxp = viewport + first_row * VIEWPORT_SIZE;

    // Reshade all of the pixels we are responsible for
    while (total_elements--) *pxp++ = Shader.GetColor(*fvp++);
}
//=========================================================================================================





//=========================================================================================================
// Main() - Computes particle paths through the complex plane
//=========================================================================================================
void CPlotter::Main(int P1, int P2, int P3)
{   
    char   command;
    frac_value value;

WaitForCommand:

    // Wait for a new command to arrive
    ReadFile(m_hread_cmd, &command, 1, nullptr, nullptr);

    // If we're just reshading, do so
    if (command == MT_RESHADE)
    {
        Reshade();
        NotifyComplete();
        goto WaitForCommand;
    }

    // Compute the pixel number at the left hand edge of this panel
    U32 panel_left_x = ps.panel_number * ps.panel_width;

    // Determine how wide 1/4 of a pixel is
    double quarter_pixel = ps.pixel_size / 4;

    // Set all of the components of a fractal value to "unused"
    value.e[0] = value.e[1] = { -2, 0 };

    // Determine the left-most real coordinate in the render
    double min_real = ps.coord.center.real - ps.coord.span.real / 2;


NextColumn:

    // Fetch a new pixel columns (0 thru panel-width - 1)
    int col_rel2_panel = IssueColumn();

    // If there are no columns left to process, we're done
    if (col_rel2_panel < 0 || aborting)
    {
        NotifyComplete();
        goto WaitForCommand;
    }

    // Convert this column number (local to this panel) to a pixel x-coordinate
    U32 pixel_x = panel_left_x + col_rel2_panel;

    // Compute the real value that corresponds to this column
    double real = min_real + (ps.pixel_size * pixel_x);

    // Point to our list of imaginary values to use
    double* p_i = imaginary;

    // Point to the first element of this column
    pixel* p_element= ps.bitmap + col_rel2_panel;

    // Loop through each row of pixels
    for (U32 pixel_y=0; pixel_y<ps.rows; ++pixel_y)
    {
        // If we've been told to abort, make it so
        if (aborting)
        {
            NotifyComplete();
            goto WaitForCommand;
        }

        // look up the imaginary portion of this coordinate
        double imag = *p_i++;

        // Find the (possibly oversampled) fractal value of this pixel
        switch (ps.oversample)
        {
        case 0:
            value.e[0] = Iterator(real, imag);
            break;

        case 4:
            value.e[0] = Iterator(real - quarter_pixel, imag - quarter_pixel);
            value.e[1] = Iterator(real - quarter_pixel, imag + quarter_pixel);
            value.e[2] = Iterator(real + quarter_pixel, imag - quarter_pixel);
            value.e[3] = Iterator(real + quarter_pixel, imag + quarter_pixel);
            break;

        case 9:

            value.e[0] = Iterator(real - quarter_pixel, imag - quarter_pixel);
            value.e[1] = Iterator(real                , imag - quarter_pixel);
            value.e[2] = Iterator(real + quarter_pixel, imag - quarter_pixel);

            value.e[3] = Iterator(real - quarter_pixel, imag                );
            value.e[4] = Iterator(real                , imag                );
            value.e[5] = Iterator(real + quarter_pixel, imag                );

            value.e[6] = Iterator(real - quarter_pixel, imag + quarter_pixel);
            value.e[7] = Iterator(real                , imag + quarter_pixel);
            value.e[8] = Iterator(real + quarter_pixel, imag + quarter_pixel);
            break;
        }

        // Fetch the pixel color that corresponds to this value
        pixel px = Shader.GetColor(value);

        // Compute the index of the element where this pixel gets stored
        U32 index = pixel_y * ps.cols_this_panel + col_rel2_panel;
        
        // Store the pixel into the bitmap
        ps.bitmap[index] = px;

        // If we're computing the viewport, store the fractal value for later use
        if (ps.bitmap == viewport) fractal[index] = value;
    }

    // We've completed an entire column of points
    columns_completed_cs.Lock();
    ++columns_completed;
    columns_completed_cs.Unlock();

    // Go fetch another column to compute
    goto NextColumn;
 }
//=========================================================================================================



//============================================================================================================
// ComputeImaginaryValues() - Computes imaginary values for each row of the panel
//============================================================================================================
void ComputeImaginaryValues()
{
    // Determine the largest imaginary coordinate.  (The one at the very top of the image)
    double max_imaginary = ps.coord.center.imag + ps.coord.span.imag/2;

    // Loop through each row of pixels
    for (U32 pixel_y=0; pixel_y<ps.rows; ++pixel_y)
    {
        // Compute the imaginary portion of this coordinate
        imaginary[pixel_y] = max_imaginary - (ps.coord.span.imag * pixel_y / ps.rows);
    }
            
}
//============================================================================================================


//=========================================================================================================
// Main() - Starts up when the worker thread gets spawned
//=========================================================================================================
void CWorker::Main(int P1, int P2, int P3)
{
    CString fn;
    CStitcher stitcher;

    // We're not aborting
    aborting = false;

    // How often will we check for progress updates?
    U32 update_delay = (ps.bitmap == viewport) ? 200 : 2000;

    // We are 0 percent complete
    U32 pct_complete = 0;

    // Tell the UI that we're at 0%
    NotifyUI(CWM_PROGRESS, 0);

    // This is the global variable that tracked completed columns
    columns_completed = 0;

    // We haven't rendered any columns yet
    U32 cols_remaining = ps.columns;

    // Start out with panel number 0
    ps.panel_number = 0;

    // Compute all the imaginary values our render is going to use
    ComputeImaginaryValues();

    // While there are columns remaining to render...
    while (cols_remaining)
    {
        // We'd like to render all remaining columns
        ps.cols_this_panel = cols_remaining;

        // We can only render one panel-width's of pixels at a time
        if (ps.cols_this_panel > ps.panel_width) ps.cols_this_panel = ps.panel_width;
        
        // Start rendering this panel
        CPlotter::StartPanel(MT_PLOT);
        
        // Until we hit 100% complete...
        while (CPlotter::ThreadsCompleted() != cpu_count)
        {
            // Compute the new percentage
            U32 new_pct = (U32)(100 * columns_completed / ps.columns);

            // If the percent complete has changed, say so
            if (new_pct != pct_complete)
            {
                pct_complete = new_pct;
                NotifyUI(CWM_PROGRESS, pct_complete);
            }

            // Do nothing for a moment
            Sleep(update_delay);
        }

        // Wait for all plotting threads to complete
        for (U32 i=0; i<cpu_count; ++i)  Plotter[i].Wait();

        // If we're aborting this render, delete the panels and drop dead
        if (aborting)
        {
            stitcher.Cleanup();
            NotifyUI(CWM_PROGRESS, PROGRESS_ABORTED);
            TerminateThread();
        }

        // If we're doing a full render to the panel...
        if (ps.bitmap != viewport)
        {
            // Construct a filename for this panel
            fn.Format(L"panel_%04i.bmp", ps.panel_number + 1);

            // And write this bitmap file
            WriteBmp(fn, panel, ps.cols_this_panel, ps.rows, 0);

            // Add this filename to the list of stitcher inputs
            stitcher.AddFile(fn);
        }

        // Increment for the next panel number
        ++ps.panel_number;

        // This is how many columns we have left to render
        cols_remaining -= ps.cols_this_panel;
    }

    // Tell the UI that we are 100% complete
    NotifyUI(CWM_PROGRESS, 100);

    // If this was a full render, stitch together the rendered panels
    if (ps.bitmap != viewport)
    {
        NotifyUI(CWM_PROGRESS, PROGRESS_STITCHING);
        stitcher.Stitch(L"render.bmp");
        NotifyUI(CWM_PROGRESS, PROGRESS_FINISHED);
    }

    // We're done with the computations
    TerminateThread();
}
//=========================================================================================================

