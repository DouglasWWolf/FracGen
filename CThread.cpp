////////////////////////////////////////////////////////
//  Copyright (c) 2014 Cepheid Corporation
//
//  CThread.cpp   Implements Class CThread
//
//  This base class provides "worker threads" for activities
//  that need to "run in the background", detached from the 
//  main user interface thread.  
////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CThread.h"
#include <map>
#include <memory>
using std::unique_ptr;
using std::map;


//============================================================================
// CThreadPlockMap() - This class implements a map that map an integer to
//                     a pointer to a CCriticalSection object.
//============================================================================
class CThreadPlockMap
{
public:

    // Returns a pointer to the CriticalSection 
    // for the supplied address
    CCriticalSection* operator[](const int& iAddress)
    {
        m_AccessControl.Lock();
        if (m_LockMap.find(iAddress) == m_LockMap.end())
        {
            m_LockMap[iAddress] = new CCriticalSection;
        }
        CCriticalSection* p = m_LockMap[iAddress];
        m_AccessControl.Unlock();
        return p;
    }

private:

    // Maps code-space addresses to CCriticalSection objects
    map<int, CCriticalSection*> m_LockMap;

    // This controls access to this object
    CCriticalSection m_AccessControl;
};
//============================================================================

//============================================================================
// This is a count of how many CThread objects have been created
//============================================================================
int CThread::m_iThreadCount = 0;
//============================================================================

//============================================================================
// This object maps codespace addresses to CriticalSection objects
//============================================================================
static CThreadPlockMap ThreadPlockMap;
//============================================================================


//============================================================================
// This is used to pass information from "Spawn()" to "LaunchThread()"
//============================================================================
struct CThreadSpawn
{
    CThread*    Object;
    int         P1, P2, P3;
    CThreadSpawn(CThread* p, int p1, int p2, int p3)
    {Object = p, P1 = p1, P2 = p2, P3 = p3;}
};
//============================================================================


//============================================================================
// LaunchCThread() - This is a helper function responsible for actually 
//                   bringing the thread into existence.
//
// Passed: ThreadPtr = Pointer to a CThreadSpawn object *ON THE HEAP*
//                     When this routine is done, it will automatically
//                     free the memory used by this CThreadSpawn object
//============================================================================
UINT LaunchCThread(LPVOID ThreadPtr)
{
    // Turn "ThreadPtr" into a pointer to a CThreadSpawn object
    unique_ptr<CThreadSpawn> p((CThreadSpawn*)ThreadPtr);

    // Spin up "Main()" in a new thread. 
    p->Object->Main(p->P1, p->P2, p->P3);

    // And tell the caller we're launched!
    return 0;
}
//============================================================================


//============================================================================
// Default Constructor
//============================================================================
CThread::CThread()
{
    // The default Thread ID is the # of CThread objects that have been
    // constructed previous to this one.  A different Thread ID can be
    // set manuall by SetThreadID()
    m_ID = m_iThreadCount++;

    // We've not yet called "Initialize()"
    m_bInitializedThread = false;

    // This thread is not paused
    m_bThreadPaused = false;

    // Indicate that no other thread has told us to unpause yet
    m_UnpauseEvent.ResetEvent();

}
//============================================================================


//============================================================================
// Spawn() - Starts derived-class method Main() in a new thread
//
// Passed:   Handle = Handle for the user interface window
//============================================================================
void CThread::Spawn(HWND Handle, int P1, int P2, int P3)
{
    // Fill in the handle of the user-interface window
    m_hWnd = Handle;
    
    // This will get set to "true" by another thread that is asking
    // us to abort.
    m_bAbortRequested = false;

    // If we haven't called "InitializeThread" yet, do so.
    if (!m_bInitializedThread)
    {
        InitializeThread();
        m_bInitializedThread = true;
    }

    // Build an object we can use to pass parameters to LaunchThread().
    // "LaunchThread()" will delete this object for us
    CThreadSpawn *params = new CThreadSpawn(this, P1, P2, P3);

    // Launch "Main" in a new thread
    AfxBeginThread(LaunchCThread, params);

}
//============================================================================


//============================================================================
// SetThreadID() - Set a new ID for this thread.  This is what will be passed
//                 to the user-interface as the first parameter of all msgs.
//============================================================================
void CThread::SetThreadID(int ID) {m_ID = ID;}
//============================================================================



//============================================================================
// AbortRequested() - Returns true if some thread has asked us to terminate
//============================================================================
bool CThread::AbortRequested() {return m_bAbortRequested;}
//============================================================================


//============================================================================
// NotifyUI() - Sends a message back to the user interface
//
// Passed:  iMessage = One of the WM_xxx constants
//          param    = Parameter appropriate to the WM_xxx constant
//============================================================================
void CThread::NotifyUI(int iMessage, LPARAM param)
{
    // If we don't have a window handle, we can't very well
    // send messages now can we?
    if (m_hWnd == (HWND)0) return;

    // Place this message in the user-interface's message queue
    ::PostMessage(m_hWnd, iMessage, m_ID, param);
}
//============================================================================

//============================================================================
// LockProcess() - Use this routine to enter a critical section of code
//                 that must only be executed by one thread at a time.
//
// ** NOTE **  You *MUST* call "UnlockProcess" sometime after this!
//============================================================================
void CThread::LockProcess()
{
#if 0
    DWORD   *rEBP, ReturnAddress;
   
    // Fetch the return address.  This will give us a unique identifier
    // of the piece of code that is trying to process lock.
    _asm mov rEBP, ebp
    ReturnAddress = rEBP[1];

    // Lock the critical section that corresponds to the 
    // return address
    ThreadPlockMap[ReturnAddress]->Lock();

    // Push this return address onto our process-lock stack
    m_PlockStack.push(ReturnAddress);
#endif

}
//============================================================================


//============================================================================
// UnlockProcess() - Tell other threads they may run the critical section
//                   of code that's being gaurded like this:
//
//                   LockProcess();
//                   {code that must be single threaded}
//                   UnlockProcess();
//============================================================================
void CThread::UnlockProcess()
{
    // If we don't have *any* process locks queued, don't do anything
    if (m_PlockStack.empty()) return;

    // Unlock the most recently locked critical section.
    ThreadPlockMap[m_PlockStack.top()]->Unlock();

    // Pop the most recent lock off our process-lock stack
    m_PlockStack.pop();
}
//============================================================================


//============================================================================
// TerminateThread() - Notifies the user interface that the thread is
//                     terminating and brings down the thread.
//============================================================================
void CThread::TerminateThread(int P1, int P2, int P3)
{
    // Build a stop message that says "The thread is terminating").
    // This object will be deleted by the message handler
    CThStopMsg *pMsg = new CThStopMsg(true, P1, P2, P3);

    // Tell the user interface that we're coming down
    NotifyUI(CWM_TH_STOP, (LPARAM)pMsg);

    // Bring down this thread
    AfxEndThread(0);
}
//============================================================================


//============================================================================
// RequestAbort() - Sets the flag that requests this thread to abort.  This
//                  routine is normally called by some other thread.
//============================================================================
void CThread::RequestAbort(int iReason)
{
    // Tell the thread to abort, and give it a reason
    m_bAbortRequested = true;
    m_iAbortReason = iReason;

    // Wake it up, just in case it was sleeping (i.e., "Paused")
    UnpauseThread();
}
//============================================================================


//============================================================================
// PauseThread() - Suspend this thread until another thread tells us to
//                 unpause
//============================================================================
void CThread::PauseThread(int P1, int P2, int P3)
{
    // Build a stop message that says "The thread is pausing".
    // This object will be deleted by the message handler
    CThStopMsg *pMsg = new CThStopMsg(false, P1, P2, P3);

    // Tell the user interface that we're pausing
    NotifyUI(CWM_TH_STOP, (LPARAM)pMsg);

    // This thread is effectively paused
    m_bThreadPaused = true;

    // Wait for another thread to tell us to continue.  The O/S will
    // automatically reset the event after it has unpaused this thread.
    ::WaitForSingleObject(m_UnpauseEvent.m_hObject, INFINITE);

    // This thread is no longer paused
    m_bThreadPaused = false;
}
//============================================================================

//============================================================================
// WaitForThreadPause() - This lets another thread wait around for a bit
//                        until this thread is paused
//============================================================================
bool CThread::WaitForThreadPause(int iMilliseconds)
{
    while (iMilliseconds > 0)
    {
        // If the thread is paused, wait for the "WaitForSingleObject()"
        // call to execute, and we're done
        if (m_bThreadPaused)
        {
            Sleep(5);
            return true;
        }
        
        // Do nothing for 10 milliseconds
        Sleep(10);
        iMilliseconds -= 10;
    }

    // Tell the caller that this thread never paused
    return false;
}
//============================================================================

//============================================================================
// UnpauseThread() - Tell a paused thread to wake up, and send it an
//                   optional parameter to let it know why it's being
//                   unpaused.
//============================================================================
void CThread::UnpauseThread(int iReason)
{
    // Keep track of why we are being told to unpause
    m_iUnpauseReason = iReason;

    // And tell the sleeping thread to wake up!
    m_UnpauseEvent.SetEvent();
}
//============================================================================


//============================================================================
// Printf() - printf() style function that sends the text to the main
//             dialog window to be displayed
//============================================================================
void CThread::Printf(int iColor, CString fmt, ...)
{
    CString buffer;

    // This is a pointer to a variable argument list
    va_list FirstArg;

    // Point to the first argument after the "fmt" parameter
    va_start(FirstArg, fmt);

    // Perform a "printf" of our arguments into the "buffer" area.
    buffer.FormatV(fmt, FirstArg);

    // Tell the system we're done with FirstArg
    va_end(FirstArg);

    // Create a new "Printf" object.  This will be deleted by
    // the message handler of the user-interface thread
    CThPrintfMsg* pMsg = new CThPrintfMsg(iColor, buffer);

    // And send this message to the user interface
    NotifyUI(CWM_PRINTF, (LPARAM)pMsg);
}
//============================================================================


