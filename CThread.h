////////////////////////////////////////////////////////
//  Copyright (c) 2014 Cepheid Corporation
//
//  CThread.h  Define Class CThread
//
//  This base class provides "worker threads" for activities
//  that need to "run in the background", detached from the 
//  main user interface thread.  
////////////////////////////////////////////////////////

#ifndef _CTHREAD_H_
#define _CTHREAD_H_
#include <Afxmt.h>
#include "StdAfx.h"
#include <stack>
using std::stack;


//============================================================================
// Any messages you want to send back to the user interface should be 
// defined as WM_THREAD + <some_constant>
//============================================================================
#define CWM_THREAD   (WM_APP + 100)
#define CWM_TH_STOP  (CWM_THREAD - 1)
#define CWM_PRINTF   (CWM_THREAD - 2)
//============================================================================



//============================================================================
// This message will be passed to the user interface whenever the thread
// terminates or pauses
//============================================================================
struct CThStopMsg
{
    bool    Terminated;
    LPARAM  P1, P2, P3, P4;
    CThStopMsg(bool b, LPARAM p1=0, LPARAM p2=0, LPARAM p3=0)
    {Terminated = b; P1 = p1; P2 = p2; P3 = p3;}
};
//============================================================================

//============================================================================
// This is the structure that gets passed to the user interface thread by
// the default implementation of the "Printf" method.
//============================================================================
struct CThPrintfMsg
{
    int     iColor;
    CString sText;
    CThPrintfMsg(int c, CString s) {iColor = c; sText = s;};
};
//============================================================================


//============================================================================
// CThread - This is a base class used for spawning threads that are detatched
//           from the user interface. 
//============================================================================
class CThread 
{
    //-------------------------------------------------------------
    // Publicly accessable methods begin here.  These methods are 
    // used by other threads (and other classes) to interact with
    // this thread.
    //-------------------------------------------------------------
public:

    // Default constructor
    CThread();

    // Spawn the "Main()" method in a new thread.  "Handle" is the
    // handle of the user interface window or 0 to indicate there is
    // no user interface.  P1, P2 and P3 are optional parameters
    // that will be passed to "Main()"
    void    Spawn(HWND Handle, int P1=0, int P2=0, int P3=0);

    // Ask this thread to abort.
    void    RequestAbort(int iReason = 0);
    
    // Wait for this thread to pause
    bool    WaitForThreadPause(int iMilliseconds = 1000);

    // Ask our paused thread to unpause
    void    UnpauseThread(int iReason = 0);

    // Set a new thread ID.
    // Thread ID's are passed back to the user-interface
    // as the first parameter of all messages. (This function
    // is very rarely useful)
    void    SetThreadID(int ID);

    //--------------------------------------------------
    // Methods accessable to derived classes begin here
    //--------------------------------------------------
protected:

    // This routine must declared by the derived class.  Main()
    // is what will be called by "Spawn()"
    virtual void Main(int P1, int P2, int P3) = 0;

    // This function will be called the first time that "Spawn" is
    // called on this object.  Feel free to over-ride this function.
    virtual void InitializeThread() {}

    // Performs "printf" style output to a main dialog window
    virtual void Printf(int iColor, CString fmt, ...);

    // Call this to bring down the thread.  P1, P2, and P3 are
    // optional parameters that will be passed to the user-interface
    // thread.  This will send a "WM_TH_STOP" messaged to the user
    // interface.
    virtual void TerminateThread(int P1=0, int P2=0, int P3=0);

    // Call this to suspend the thread until some other thread
    // calls "UnpauseThread()".  P1, P2, and P3 are optional parameters
    // that will be passed to the user-interface thread
    void    PauseThread(int P1=0, int P2=0, int P3=0);

    // Call this to send information back to the user interface
    void    NotifyUI(int iMessage, LPARAM param = 0);

    // This will be true if some thread has asked us to abort
    bool    AbortRequested();

    // Use "LockProcess()" and "UnlockProcess()" as a pair to
    // force "one-thread-at-a-time" access to critical sections
    // of code.  (These need to *not* be virtual void because
    // they rely on the return address, and virtual functions
    // don't follow the normal rules for return addresses!)
    void    LockProcess();
    void    UnlockProcess();

    //--------------------------------------------------
    // Variables accesable to derived classes begin here
    //--------------------------------------------------
protected:
    
    // This is a handle to the main dialog box
    HWND    m_hWnd;

    // This is the programmer assigned ID of this thread
    int     m_ID;

    // This is an optional indicator that tells us why we
    // were told to unpause
    int     m_iUnpauseReason;

    // This contains a numeric code that tells us why we're
    // being asked to abort.
    int     m_iAbortReason;

    // This will be true when the thread is paused
    bool    m_bThreadPaused;

private:
    //--------------------------------------------
    // Variables private to this class begin here
    //--------------------------------------------
    
    // This is the event that controls our "paused" state
    CEvent          m_UnpauseEvent;

    // This is a count of how many objects of type "CThread" have been
    // created
    static int      m_iThreadCount;

    // This is true if "InitializeThread" has been called at least 
    // once.
    bool            m_bInitializedThread;

    // This will be set to true by some other thread that's
    // asking this thread to abort.
    bool            m_bAbortRequested;

    // This is a stack of addresses of locked code
    stack<int>      m_PlockStack;

    // This is to allow the external "LaunchThread()" function
    // access to "Main()"
    friend UINT     LaunchCThread(LPVOID);

};
//============================================================================

#endif
