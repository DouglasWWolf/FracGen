//============================================================================
// IOServer.cpp - Describes a discrete I/O server
//============================================================================


//============================================================================
//                             #include files
//============================================================================
#include "stdafx.h"
#include "Resource.h"
#include "History.h"
#include "CephUtils.h"
#include "IOServer2.h"
#include "NIDAQmx.h"
#include <map>
using std::map;
//============================================================================


//============================================================================
//                               Constants
//============================================================================
const int SUICIDE_PORT = 777;
const int READER_PORT  = 1066;
const int WRITER_PORT  = 1067;
const int MAX_NODES    = 32;

//--------------------------------------------------------------------
// SYS_XXX - These are server status, returned via the "ST" response
//--------------------------------------------------------------------
enum
{
    SYS_FAULTED    = 0,
    SYS_BOOTING    = 1,
    SYS_IDLE       = 2,
    SYS_RUNNING    = 3
};
//============================================================================


//============================================================================
// Data written to/from the digital input and output ports
//============================================================================
const int INP_SIZE = 4;
const int OUT_SIZE = 4;

// This is the discrete input data
BYTE    g_InpData[INP_SIZE];

// This is the data from the previous "Read Inputs" operation
BYTE    g_PrevInp[INP_SIZE];

// Output data to be sent to the outputs
BYTE    g_OutData[OUT_SIZE];
//============================================================================


//============================================================================
// NIDAQmx Task handles for the 4 digital output ports and 4 digital input
// ports.  All ports are 8 bits wide
//============================================================================
TaskHandle Dig_In_Port[INP_SIZE], Dig_Out_Port[OUT_SIZE];


//============================================================================
// This struct describes an I/O object.  An IO object is a collection of
// one or more bits within a single byte in either g_InpData or g_OutData
//============================================================================
struct IO_Obj
{
    // The byte offset in one of the I/O arrays
    BYTE offset; 
    
    // The mask which indicates which bits this object pertains to
    BYTE mask;
    
    // How far left the bits must be shifted to normalize the field
    BYTE shift;
   
    // This is used to define the object
    void Define(int iOffset, int iMask)
    {
        // Save the values that were passed to us
        offset = iOffset; mask = iMask;

        // Loop through each bit from LSB to MSB...
        for (int bit=0; bit<8; ++bit)
        {
            // If this bit is on, tell the caller
            if (mask & (1<<bit))
            {
                shift = bit;
                break;
            }
        }
    }
    
    // This is used to get the value if its an input
    int GetValue()
    {
        return (g_InpData[offset] & mask) >> shift;
    }
    
    // This is used to set the value if it's an output
    void SetValue(int data)
    {
        g_OutData[offset] &= ~mask;
        g_OutData[offset] |= ((data << shift) & mask);
    }

    // This is used to simulate the value if it's an input
    void SimulateInput(int data)
    {
        g_InpData[offset] &= ~mask;
        g_InpData[offset] |= ((data << shift) & mask);
    }

};
typedef map<BYTE,IO_Obj> T_IOMap;
typedef T_IOMap::iterator IOMapIt;
//============================================================================


//============================================================================
// This defines a simulated input.  The key in the input ID, and the data is
// the value that that particular input ID should be forced to.
//============================================================================
typedef map<int,int> T_SIMap;
typedef T_SIMap::iterator SIMapIt;
//============================================================================



//============================================================================
// This is a thread that monitors a datagram socket for a "kill yourself" msg
//============================================================================
struct CShutdownMon : public CThread {void Main(int P1, int P2, int P3);};
//============================================================================


//============================================================================
// CDatagramWriter : This is a base-class than can write our output datagrams
//============================================================================
class CDatagramWriter : public CThread
{
protected:

    // Call this to send data to the client
    void        Send(char* cmd, BYTE* p = NULL, int iCount = 0);

    // This is a pointer to our output socket
    NetSock     *m_pOutSock;
};
//============================================================================


//============================================================================
// CReader() - This class (and it's associated thread) are responsible for
//             receiving, handling, and responding to messages from the client
//============================================================================
class CReader : public CDatagramWriter
{
protected:

    // This routine is started up in a new thread by the "Spawn" command
    // from the CThread base-class.
    void        Main(int P1, int P2, int P3);

private:

	// Fetches DI and DO definitions from a spec-file
	void		FetchSpecs();

    // Writes a value to an output port
    void        WriteByte(IOMapIt it, int iValue);

	// Define an input
	void        DefineInput(int ID, int offset, int mask);

	// Define an output
	void		DefineOutput(int ID, int offset, int mask);

};
//============================================================================


//============================================================================
// CIOSys() - This class (and it's associated thread) are responsible for
//            managing communication with the DaqBoard and occasionally querying
//            it to check for inputs that have changed state
//============================================================================
class CIOSys : public CDatagramWriter
{
protected:

    // This routine is started up in a new thread by the "Spawn" command
    // from the CThread base-class.
    void        Main(int P1, int P2, int P3);

    // Reads all of the discrete inputs
    void        ReadAllInputs();

    // This routine reports a change of state for any input bit
    // the client cares about
    void        ReportChangeOfState();
};
//============================================================================




//============================================================================
//                           Global Objects
//============================================================================
CMainDlg*       g_pMainDlg;

// This represents the command-line switches and parameters
CCommandLine	 CommandLine;

// This is the name of the spec-file
CString			 g_sSpecFile;

// Controls one-thread-at-a-time access to I/O card
CCriticalSection g_HardwareAccess;

// This thread reads UDP messages and acts on them
CReader          Reader;

// This thread periodically reads the discrete-inputs and processes them
CIOSys           IOSystem;

// This thread waits for a command that will cause this program to exit
CShutdownMon     SuicideWatch;

// This maps ID numbers to input objects
T_IOMap          InputMap;

// This maps ID numbers to output objects
T_IOMap          OutputMap;

// This is a list of input ID's, and values we want them to take on
T_SIMap          Simulated;

// Controls multi-threaded access to the InputMap, so that the Reader
// thread can't modify it while the IOSys thread is using it
CCriticalSection g_MapAccess;
  
// If this is true, we should periodically report state changes on the inputs
bool             g_bReporting;

// Overall system state
int              g_iSystemState = SYS_BOOTING;

// This is true if we want to see debug messages on the screen
BOOL             g_DEBUG;

// The default input-sampling period is 100ms
int              g_iRefreshPeriod = 100;
//============================================================================




//============================================================================
//            Exactly one instance of our application program
//============================================================================
class CApp : public CWinApp {public: BOOL InitInstance();} theApp;
//============================================================================


//============================================================================
// WritePort() - Writes a byte to a digital output port
//============================================================================
void WritePort(int port, BYTE data)
{
    int32 dummy;

    DAQmxWriteDigitalU8
    (
        Dig_Out_Port[port],
        1,1,10.0,
        DAQmx_Val_GroupByChannel,
        &data,
        &dummy,NULL
    );
}
//============================================================================


//============================================================================
// ConnectToDAQ() - Connects to the National Instruments PCI-6514 digital I/O
//                  card and configures the 8-bit wide input and output ports.
//
// On the DAQ card, the input ports are numbered Port 0 thru Port 3, and the
// output ports are numbered Port 4 thu Port 7.
//============================================================================
bool ConnectToDAQ()
{
    int  i, error;
    char port_name[100];

    // Loop, creating all of the digital input ports...
    for (i=0; i<INP_SIZE; ++i)
    {
        // Create the name of the port
        sprintf(port_name, "Dev1/port%i/line0:7", i);

        // Create the NIDAQmx task that will control this digital input port
        error = DAQmxCreateTask("", &Dig_In_Port[i]);
        if (error) return false;

        // Define this as an 8-bit wide input port
        error = DAQmxCreateDIChan(Dig_In_Port[i], port_name,"",DAQmx_Val_ChanForAllLines);
        if (error) return false;

        // Start the NIDAQmx task that controls this port
        error = DAQmxStartTask(Dig_In_Port[i]);
        if (error) return false;

    }

    // Loop, creating all of the digital output ports...
    for (i=0; i<OUT_SIZE; ++i)
    {
        // Create the name of the port
        sprintf(port_name, "Dev1/port%i/line0:7", i+4);

        // Create the NIDAQmx task that will control this digital input port
        error = DAQmxCreateTask("", &Dig_Out_Port[i]);
        if (error) return false;

        // Define this as an 8-bit wide output port
        error = DAQmxCreateDOChan(Dig_Out_Port[i], port_name,"",DAQmx_Val_ChanForAllLines);
        if (error) return false;

        // Start the NIDAQmx task that controls this port
        error = DAQmxStartTask(Dig_Out_Port[i]);
        if (error) return false;
    }

    // Tell the caller that all is well
    return true;
}
//============================================================================


//============================================================================
// SimulateInputs() - Forces bits in the g_InpData table to take on a
//                    predetermined state
//============================================================================
void SimulateInputs()
{
    // Loop through every simulated input...
    for (SIMapIt it = Simulated.begin(); it != Simulated.end(); ++it)    
    {
        // Fetch the input ID and the value we want it to have
        int ID     = it->first;
        int iValue = it->second;
        
        // Find this ID in our input map
        IOMapIt im = InputMap.find(ID);
        
        // If this Input ID # exists, simulate its input value
        if (im != InputMap.end()) (im->second).SimulateInput(iValue);
    }
}
//============================================================================




//============================================================================
// InitInstance() - Gets called once at program startup
//============================================================================
BOOL CApp::InitInstance()
{
    CString sTemp;

    // Define the title bar of all Popup boxes
    SetDefaultPopupTitle("IOServer2");

    // Enable the MFC TCP/IP socket system
    AfxSocketInit();

    // Enable the normal "3D" look of controls
    Enable3dControls();		

    // Make sure only one copy is running and that our clients can verify
    // that we're here
    if (CreateMutex(NULL, FALSE, "IOtServer") && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        exit(0);
    }

	// "-DEBUG" is a valid command-line switch
    CommandLine.DeclareSwitch("DEBUG");

    // "-REFRESH nnnn" is a valid command line switch
    CommandLine.DeclareSwitch("REFRESH", CLP_MANDATORY);
    
    // Parse the command line
    CommandLine.Parse();	

    // Decide whether or not we're displaying DEBUG messages
    g_DEBUG = CommandLine.Switch("DEBUG");

    // Find out if the command-line specified a refresh period
    if (CommandLine.Switch("REFRESH", &sTemp)) g_iRefreshPeriod = atoi(sTemp);

	// If there was a filename present on the command line, it's our specs
	if (CommandLine.ArgCount() > 0) g_sSpecFile = CommandLine.Arg(0);

    // Create a new main dialog
    g_pMainDlg = new CMainDlg();
        
    // Execute the dialog
    g_pMainDlg->DoModal();

    // Get rid of the dialog
    delete g_pMainDlg;

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
//============================================================================



//============================================================================
// Constructor() - Called once to create the main dialog box
//============================================================================
CMainDlg::CMainDlg(CWnd* pParent) : CDialog(IDD_MAIN_DLG, pParent) {}
//============================================================================


//============================================================================
// DoDataExchange() - Swaps information between the GUI fields and variables
//============================================================================
void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Check(pDX, IDC_DEBUG, g_DEBUG);
    CDialog::DoDataExchange(pDX);
}
//============================================================================



//============================================================================
// Message Map - Controls what routines get called for specified events
//============================================================================
BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
    ON_BN_CLICKED(IDC_EXIT,   OnExit)
    ON_BN_CLICKED(IDC_DEBUG,  OnDebug)
    ON_BN_CLICKED(IDC_HIDE,   OnHide)
    ON_MESSAGE   (CWM_PRINTF, ThPrintf)
END_MESSAGE_MAP()
//============================================================================


//============================================================================
// OnDebug() - This just fetches the status of the "Show Messages" checkbox
//============================================================================
void CMainDlg::OnDebug() {GetDataFromWindow();}
//============================================================================


//============================================================================
// OnExit() - Called when the user presses the "Exit" button
//============================================================================
void CMainDlg::OnExit()
{
    char* msg = "Are you certain you want to exit?";
    
    if (Popup(MB_YESNO | MB_ICONSTOP, msg) == IDYES) exit(1);
}    
//============================================================================


//============================================================================
// OnHide() - Gets called when the "Hide" button is pressed
//============================================================================
void CMainDlg::OnHide() 
{
    WINDOWPLACEMENT place;
    GetWindowPlacement(&place);
    place.showCmd = SW_MINIMIZE;
    SetWindowPlacement(&place);
}
//============================================================================


//============================================================================
// OnInitDialog() - Called once before the dialog box is displayed
//============================================================================
BOOL CMainDlg::OnInitDialog()
{
    // Let the base-class do it's thing
    CDialog::OnInitDialog();

    // Subclass the output window so we can control it
    m_OutputWindow.SubclassDlgItem(IDC_WINDOW, this);

    // Launch our "Kill ourselves upon command" thread
    SuicideWatch.Spawn(GetSafeHwnd());

    // Launch our message-reader thread;
    Reader.Spawn(GetSafeHwnd());

    // Launch the I/O system
    IOSystem.Spawn(GetSafeHwnd());

    // And minimize the window
    OnHide();

    // Return control to MFC
    return FALSE; 
}
//============================================================================


//============================================================================
// PreTranslateMessage() - Perform special handling for the escape key
//============================================================================
BOOL CMainDlg::PreTranslateMessage(MSG* pMsg) 
{
    // Throw away "Escape-key down" messages
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 27) return true;

    // If "OnPreTranslateMessage" returned false, let CDialog do
    // normal message translation and processing
	return CDialog::PreTranslateMessage(pMsg);
}
//============================================================================



//============================================================================
// wPrintf() - Displays site-specific printf() style data into the output
//             window.
//
// Passed:  iColor = a CLR constant from CephUtilsImp.h
//          fmt    = print style formatting and data
//============================================================================
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
    m_OutputWindow.Display(CUTime::Now() + " - "+ out, iColor);
}
//============================================================================


//============================================================================
// ThPrintf() - Handler for the WM_TH_PRINT message
//
// Notes: "Value" is actually a pointer to a CThPrintfMsg object
//============================================================================
LONG CMainDlg::ThPrintf(UINT iSite, LPARAM Value)
{
    // Make "Value" an auto_ptr to a CThPrintfMsg object
    auto_ptr<CThPrintfMsg> pMsg((CThPrintfMsg*)Value);

    // Fetch the text that we'll be displaying
    CString sText = pMsg->sText;

    // Print the string in the window
    wPrintf(pMsg->iColor, "%s", sText);

    // This return value is just to keep the compiler happy
    return 0;
}
//============================================================================




//============================================================================
// ShutdownMon - Waits for a datagram, and exits the program when a 
//               "SHUTDOWN" message arrives.
//============================================================================
void CShutdownMon::Main(int P1, int P2, int P3)
{
    char    buffer[100];
    NetSock sock;

    // Create the socket
    sock.Create(SUICIDE_PORT, SOCK_DGRAM);

    while (true)
    {
        sock.Receive(buffer, sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = 0;
        if (stricmp(buffer, "shutdown") == 0) exit(1);
        Printf(0, "SuicideWatch rcvd msg: \"%s\"", buffer);
    }
}
//============================================================================




//============================================================================
//============================================================================
//============================================================================
//============================================================================
//=============   THIS IS THE BEGINNING OF THE READER CLASS    ===============
//============================================================================
//============================================================================
//============================================================================
//============================================================================





//============================================================================
// Main() - Called when this thread starts
//
// Passed:  P1, P2, and P3 don't contain anything of interest to this 
//          program.
//
//
// This thread allows these commands always:
//
//    RS = Request Status                   (We respond with a 'ST')
//    RV = Request Version
//    XX = Exit program
//    DI = Define Input   <ID><Byte#><Mask>
//    SI = Simulate Input <ID><Value> (0xFF = stop simulating)
//    SF = Simulate Fault <Node><State><Status> (0xFF, 0xFF = stop simulating)
//    DO = Define Ouput   <ID><Byte#><Mask>
//    RC = Report Changes <ID><0|1>
//    ER = End Reporting
//    CD = Clear Definitions
//
// And we allow these commands only when the system state is SYS_RUNNING
//
//    BR = Begin Reporting
//    RI = Request Input  <ID>              (We respond with a 'IN')
//    RA = Request All                      (We respond with multiple 'IN', then 'AR')
//    SO = Set Output     <ID><value>
//
// Possible outputs are:
//
//    ??<cmd1><cmd2>        - Unrecognized command <cmd1><cmd2>
//    **<cmd1><cmd2>        - Must be running to do this 
//    UI<ID>                - Unknown ID on an RI or SO command
//    IN<ID><data>          - Data from the Daqboard inputs
//    SC<ID><data>          - State Change on a discrete input
//    ST<status>            - Status is one of the SYS_XXX constants
//    VN<msb><lsb>          - Version number
//    FS<node><err><status> - Fault Status
//    FC<node><err><status> - Fault Change
//    SS                    - Server Started
//    AR                    - All Reported ("RA" is complete")
//
//============================================================================
void CReader::Main(int P1, int P2, int P3)
{
    
    BYTE    dg[10], output[100];
    CString sCmd, s;
    NetSock InSock, OutSock;
    map<CString, int> LenMap;

    // Create a datagram socket
    InSock.Create(READER_PORT, SOCK_DGRAM);
    OutSock.Create(0, SOCK_DGRAM);

    // Store a pointer to the output socket so "Send" can get to it
    m_pOutSock = &OutSock;

	// If we have a spec-file, fetch it
	if (!g_sSpecFile.IsEmpty()) FetchSpecs();

    // Build the map that shows how many data bytes follow each command
    LenMap["DI"] = 3;
    LenMap["DO"] = 3;
    LenMap["RI"] = 1;
    LenMap["SO"] = 2;
    LenMap["SI"] = 2;
    LenMap["SF"] = 3;
    LenMap["RC"] = 2;
    LenMap["WT"] = 1;

    // Tell any listening client that IOServer is in da house!
    Send("SS");


WaitForCommand:

    // For for a UDP datagram to arrive
    InSock.Receive(dg, sizeof(dg));

    // Turn the first two bytes into a CString
    sCmd.Empty(); sCmd+=dg[0]; sCmd+=dg[1];

    // Fill in the output with our input command, just in case
    // we need it in order to return an error
    output[0] = dg[0]; output[1] = dg[1];

    // If we're dumping debug data, show the incoming command
    if (g_DEBUG)
    {
        s.Format("---> %s -", sCmd);
        int iCount = LenMap[sCmd];
        for (int i=0; i<iCount; ++i) s += toCString(" %02X", dg[i+2]);
        Printf(0, s);
    }

    // If this is a "Request Status"...
    if (sCmd == "RS")
    {
        output[0] = g_iSystemState;
        Send("ST", output, 1);
        goto WaitForCommand;
    }


    // If this is a "Request Version"...
    else if (sCmd == "RV")
    {
        output[0] = (VERSION_BUILD >> 8);
        output[1] = (VERSION_BUILD & 0xFF);
        Send("VN", output, 2);
        goto WaitForCommand;
    }
    
    // If this was a "Clear Definitions"
    else if (sCmd == "CD")
    {
        g_MapAccess.Lock();
        InputMap.clear();
        OutputMap.clear();
        g_MapAccess.Unlock();
        goto WaitForCommand;
    }
    
    // If this is a "Define Input"...
    else if (sCmd == "DI")
    {
		DefineInput(dg[2], dg[3], dg[4]);
        goto WaitForCommand;
    }
    
    // If this is a "Simulate Input"...
    else if (sCmd == "SI")
    {
        g_MapAccess.Lock();
        int ID     = dg[2];
        int iValue = dg[3];
        if (iValue != 0xFF)
            Simulated[ID] = iValue;
        else
            Simulated.erase(ID);
        g_MapAccess.Unlock();
        goto WaitForCommand;
    }


    // If this is a "Simulate Fault", do nothing
    else if (sCmd == "SF")
    {
        goto WaitForCommand;
    }


    // If this is a "Define Output"...
    else if (sCmd == "DO")
    {
        DefineOutput(dg[2], dg[3], dg[4]);
        goto WaitForCommand;
    }

    // If this is "Report Changes", do nothing
	else if (sCmd == "RC")
	{
        goto WaitForCommand;
	}


    // If this is a "End Reporting"
    else if (sCmd == "ER")
    {
        g_bReporting = false;
        goto WaitForCommand;
    }


    // If this is a "XX" command, call it quits!
    else if (sCmd == "XX")
    {
       exit(1);
    }

    //-------------------------------------------------------------------------
    // From here down, these commands require the system state to be "running"
    //-------------------------------------------------------------------------    

    if (g_iSystemState != SYS_RUNNING) 
    {
        Send("**", output, 2);
        goto WaitForCommand;
    }
    

    // If this is a "Begin Reporting"...
    if (sCmd == "BR")
    {
        // Make the most recent input data the "previous" data
        SimulateInputs();
        memcpy(g_PrevInp, g_InpData, INP_SIZE);
        g_bReporting = true;
    }


    // If this is a "Request Input"...
    else if (sCmd == "RI")
    {
        // Fetch the ID of the input object
        int ID = dg[2];
        
        // Find it in the map
        IOMapIt it = InputMap.find(ID);

        // Send either a "Unknown ID" or a "Input Value"
        if (it == InputMap.end())
            Send("UI", dg+2, 1);
        else
        {
            output[0] = ID;
            output[1] = it->second.GetValue();
            Send("IN", output, 2);
        }
    }

    // If this is "Request All (Inputs & Faults)"...
    else if (sCmd == "RA")
    {
        // Make sure all the simulate inputs are represented in the input-map
        SimulateInputs();

        // Loop through each input, sending its value
        for (IOMapIt it = InputMap.begin(); it != InputMap.end(); ++it)
        {
            output[0] = it->first;
            output[1] = it->second.GetValue();
            Send("IN", output, 2);
        }

		// Send the marker that says all inputs have been sent
		Send("AR");
    }


    // If this is a "Set Output"...
    else if (sCmd == "SO")
    {
        // Fetch the ID of and value of the output object
        int ID = dg[2];
        int iValue = dg[3];
        
        // Find it in the map
        IOMapIt it = OutputMap.find(ID);

        // Either send an "Unknown ID", or send the data to the DaqBoard
        if (it == OutputMap.end())
            Send("UI", dg+2, 1);
        else
            WriteByte(it, iValue);
    }

    // If this is a "Wait Time"...
    else if (sCmd == "WT")
    {
        // Fetch the number of milliseconds to sleep for
        int iMilliseconds = dg[2];

        // Don't let the user specify "Wait forever"
        if (iMilliseconds == 0) iMilliseconds = 1;

        // And do nothing for the specified period of time
        Sleep(iMilliseconds);
    }

    // If we get here, send the message that says we didn't recognize the command
    else Send("??", output, 2);

    // And go wait for the next command to arrive
    goto WaitForCommand;
}
//============================================================================


//============================================================================
// WriteByte() - Writes a value to a physical output port
//
// Passed:  it     = iterator to an Output-Object
//          iValue = the value that Output-Object should take on
//============================================================================
void CReader::WriteByte(IOMapIt it, int iValue)
{
    // Place the desired value into our output bitmap
    it->second.SetValue(iValue);
    
    // Find the byte offset of this Output-Object
    int offset = it->second.offset;
    
    // Find the 8-bits that should be written out to the port
    BYTE data  = g_OutData[offset];
    
    // And write the data to the physical output port
    g_HardwareAccess.Lock();
    WritePort(offset, data);
    g_HardwareAccess.Unlock();
}
//============================================================================



//============================================================================
// FetchSpecs() - Read in the specfile with input and output definitions
//============================================================================
void CReader::FetchSpecs()
{
	CScript		script;
	CSpecFile	specFile;
	CString		sLine;

	// If we can't read the spec-file, quit
	if (!specFile.Read(g_sSpecFile)) exit(1);

	// Read the script that holds all of our commands
	specFile.Get("COMMANDS", &script);

	// Loop through each line of our script...
	while (script.GetNextLine(&sLine))
	{
		// Fetch the command name
		CString sCmd = script.GetNextWord(true);

		// Fetch the parameters
		int p1 = script.GetNextInt();
		int p2 = script.GetNextInt();
		int p3 = script.GetNextInt();

		// Handle "DI" (Define Input)
		if (sCmd == "DI")
		{
			DefineInput(p1, p2, p3);
		}
		
		// Handle "DO" (Define Output)
		else if (sCmd == "DO")
		{
			DefineOutput(p1, p2, p3);
		}
		
		// Anything else is a syntax error
		else Printf(CLR_RED, "Syntax Error: %s", sLine);
	}
}
//============================================================================


//============================================================================
// DefineInput() - Adds an Input Object to the Input map
//============================================================================
void CReader::DefineInput(int ID, int offset, int mask)
{
    IO_Obj  obj;
	
    obj.Define(offset, mask);
    g_MapAccess.Lock();
    InputMap[ID] = obj;
    g_MapAccess.Unlock();
}
//============================================================================


//============================================================================
// DefineOutput() - Adds an Output Object to the Output map
//============================================================================
void CReader::DefineOutput(int ID, int offset, int mask)
{
    IO_Obj  obj;
	
    obj.Define(offset, mask);
    OutputMap[ID] = obj;
}
//============================================================================




//============================================================================
//============================================================================
//============================================================================
//============================================================================
//============   THIS IS THE BEGINNING OF THE IOSYSTEM CLASS    ==============
//============================================================================
//============================================================================
//============================================================================
//============================================================================


//============================================================================
// Main() - Called when this thread starts
//
// Passed:  P1, P2, and P3 don't contain anything of interest to this 
//          program.
//
//
//============================================================================
void CIOSys::Main(int P1, int P2, int P3)
{
   
    NetSock             OutSock;

    // Tell the user we're workin' on it!
    Printf(0, "Booting...");

    // Create a datagram socket
    OutSock.Create(0, SOCK_DGRAM);

    // Store a pointer to the output socket so "Send" can get to it
    m_pOutSock = &OutSock;

    // If we can't open the connection, bitch.
    if (!ConnectToDAQ())
    {
        Printf(CLR_RED, "Can't connect to PCI-6514");
        g_iSystemState = SYS_FAULTED;
        Sleep(INFINITE);
    }

    // Fetch the initial state of all the inputs
    ReadAllInputs();

    // Tell the world "Its Aliiiiiivvvvee!"
    g_iSystemState = SYS_RUNNING;
    Printf(0, "Running");

    // We're gonna loop forever, reporting state changes
    while (true)
    {
        // Fetch all of the discrete inputs
        g_HardwareAccess.Lock();
        ReadAllInputs();
        SimulateInputs();
        g_HardwareAccess.Unlock();

        // If we should be reporting state-changes, do so
        if (g_bReporting) ReportChangeOfState();

        // Make the most recent input data the "previous" data
        memcpy(g_PrevInp, g_InpData, INP_SIZE);

        // Spend some time doing nothing
        Sleep(g_iRefreshPeriod);
    }
}
//============================================================================


//============================================================================
// ReadAllInputs() - Reads all discrete inputs
//============================================================================
void CIOSys::ReadAllInputs()
{
    BYTE  value;    
    int32 dummy;

    // Loop through each input port...
    for (int i=0; i<INP_SIZE; ++i)
    {
        // Fetch the input data from this port
        DAQmxReadDigitalU8(Dig_In_Port[i], 1, 10.0, DAQmx_Val_GroupByChannel,
                           &value, 1, &dummy, NULL);
        
        // Stuff the data into our array that contains all the input data
        g_InpData[i] = value;
    }
}
//============================================================================

//============================================================================
// ReportChangeOfState() - Reports the change of state for any input
//                         the client is interested in seeing
//
// On Entry: g_InpData[] = Most recently read input data
//           g_PrevInp[] = The set of input data just before this one
//           InputMap    = map of input objects specifying bit-patterns
//                         to watch for a change (and report on if it happens)
//============================================================================
void CIOSys::ReportChangeOfState()
{
    BYTE    stateChange[1000];
    BYTE    buffer[2];

    // Tell the other thread that we're using the input-map right now
    g_MapAccess.Lock();

    // Calulculate which input bits have changed since the previous read
    for (int i=0; i<INP_SIZE; ++i) stateChange[i] = g_InpData[i] ^ g_PrevInp[i];

    // Loop through each item on the watch-list
    for (IOMapIt it = InputMap.begin(); it != InputMap.end(); ++it)
    {        
        // Fetch the input ID
        int ID = it->first;

        // Get a handy reference to the IO Object
        IO_Obj& o = it->second;
        
        // If this input object has changed its state since last time...
        if (stateChange[o.offset] & o.mask)
        {
            buffer[0] = ID;
            buffer[1] = o.GetValue();
            Send("SC", buffer, 2);
        }
    }

    // Tell the other thread that he may now modify the map to his hearts content
    g_MapAccess.Unlock();
}
//============================================================================



//============================================================================
// Send() - Sends a datagram to the client
//============================================================================
void CDatagramWriter::Send(char *cmd, BYTE *p, int iCount)
{
    CString  s;
    char     output[500];

    // Stuff the command into the output buffer
    output[0] = cmd[0];
    output[1] = cmd[1];

    // Stuff the data into output buffer
    memcpy(output+2, p, iCount);
    
    // And send it!
    m_pOutSock->Send(output, iCount+2, WRITER_PORT, "localhost");

    // If we're debugging, show was just got sent
    if (g_DEBUG)
    {
        s.Format("<--- %s -", cmd);
        for (int i=0; i<iCount; ++i) s += toCString(" %02X", p[i]);
        Printf(0, s);
    }

}
//============================================================================



