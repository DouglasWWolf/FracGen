#pragma once
#include "CThread.h"
#include "typedefs.h"

//=========================================================================================================
// These are the availbale Multi-threaded commands available
//=========================================================================================================
enum
{
    MT_PLOT, MT_RESHADE
};
//=========================================================================================================


//=========================================================================================================
// T_COORD - A set of computational coordinates
//=========================================================================================================
struct T_COORD
{
    complex center;
    complex span;
};
//=========================================================================================================


//=========================================================================================================
// CWorker - This is the class/thread that performs all background tasks
//=========================================================================================================
class CWorker : public CThread
{
public:

    // This routine is called when this thread spawns
    void Main(int P1, int P2, int P3);
};
//=========================================================================================================




//=========================================================================================================
// CPlotter - This is the class/thread that is responsible for plotting a column of pixels
//=========================================================================================================
class CPlotter : public CThread
{
public:

    // Call this to determine which fractal we're going to plot
    static void SetFractal(U32 fractal);
    
    // Starts the plot for an entire panel
    static void StartPanel(char command);

    // Returns a count of the number of threads that have completed their task
    static U32  ThreadsCompleted();

    // Initialize this computation thread
    void Init();

    // This routine is called when this thread spawns
    void Main(int P1, int P2, int P3);

    // Start a new plot or reshade
    void Start(char command);

    // Checks to see if "Wait()" will return immediately
    bool IsFinished() {return m_is_task_complete;}

    // Waits for "main()" to finish;
    void Wait();



protected:

    static int      IssueColumn();
    void            Reshade();
    void            NotifyComplete();
    volatile static U32  m_next_column;

    volatile bool m_is_task_complete;

    HANDLE  m_hread_cmd, m_hwrite_cmd;
    HANDLE  m_hread_rsp, m_hwrite_rsp;


};
//=========================================================================================================

