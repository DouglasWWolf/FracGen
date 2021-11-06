#pragma once
#include "WinUtilsImp.h"
#include "typedefs.h"

//============================================================================
// Disable the "long symbol" warning generated by the STL map template
//============================================================================
#pragma warning( disable : 4786 )
//============================================================================


//============================================================================
// Struct Range - Expresses a range of values
//============================================================================
struct Range
{
            Range()
            {Set(0,0);}

            Range(double min, double max)
            {Set(min, max);}
    
    void    Set(double min, double max);
    
    bool    Contains(double val);
    bool    Excludes(double val)
            {return !Contains(val);}

    CString S(char* fmt = NULL);

    int      Min,  Max;
    double  fMin, fMax;
};
//============================================================================








//============================================================================
// Class CScript  - Objects of this type represent a script that was read
//                  from a spec-file
//============================================================================
class CScript
{

public:

    // The Constructor
    CScript();

    // Call this to add a line to the script
    void    AddLine(CString sLine);

    // Call this to fetch the next (or first) line
    bool    GetNextLine(CString* p = NULL);

    // After a successfull "GetNextLine()", use these to fetch
    // words and numbers from the line
    CString GetNextWord(bool bMakeUpper = false);
    int     GetNextInt();
    double  GetNextFloat();

    // Call this to "reset" the script to the top so that the
    // next call to "GetNextLine()" fetches the next line
    void    Reset() {m_iCurrentLine = 0;}

    // Call this to erase the contents of a script
    void    MakeEmpty() {m_Line.clear(); Reset();}

//============================================================================
private:

    // This is a list of all the lines in the script
    std::vector<CString> m_Line; 

    // This is the line number we're about to fetch from m_Line
    U32             m_iCurrentLine;

    // This is a vector of words parsed from the current line
    std::vector<CString> m_Word;

    // This is the word number we're about to fetch from m_Word
    U32           m_iCurrentWord;

    // Parses a line into discreet words
    void            Parse(CString sIn);
};
//============================================================================


//============================================================================
// This is a WinUtils exception... the base type for all other exceptions
// that can be thrown by CephUtils classes.
//============================================================================
class WUException
{
public:

    // Constructor, sets the error message string (optionally)
    WUException(CString s = L"") {m_sErrorString = s;}

    // Returns the error message
    CString Message() {return m_sErrorString;}

protected:

    // Lets a derived class set its error message text
    void SetErrorMessage(CString s) {m_sErrorString = s;}

private:

    CString m_sErrorString;

};
//============================================================================


//============================================================================
// Class CSpecFileErr - These can be thrown by any of the Get functions
//============================================================================
class CSpecFileErr : public WUException
{
public: CSpecFileErr(CString s) : WUException(s) {};
};
//============================================================================


//============================================================================
// Class CSpecFile  - Represents a specification file
//============================================================================
class CSpecFile  
{
    
public:

    // The constructor makes sure we start with zero specs
    CSpecFile();

    // Call this to read in a spec file
    bool    Read(CString sFilename, bool bDialogOnFail = true); 

    // After reading, call this to find out if the spec-file was cloaked
    bool    IsCloaked() {return m_bIsCloaked;}

    // Call this is you'd like the "Get()" functions
    // to throw errors when they can't find a spec name.
    void    ThrowErrors(bool bFlag = true) {m_bThrow = bFlag;}

    // Call these *after* Read(), but before "Get()"
    void    EnableSection(CString sType, CString sValue);
    void    EnableSection(CString sType, int     iValue);

    // Call this to check to see if a spec exists
    bool    Exists(CString sSpecName);

    // Call this to fetch a spec into a Range object
    void    Get(CString SpecName, Range* r);

    // Call this to fetch a script spec
    void    Get(CString SpecName, CScript* script);

    //-------------------------------------------------------------------------------
    // Call this to fetch a CString, bool, double, or int specification
    //-------------------------------------------------------------------------------
    template<class T> void Get(CString SpecName, T* p1=NULL, T* p2=NULL, T* p3=NULL,
                                                 T* p4=NULL, T* p5=NULL, T* p6=NULL,
                                                 T* p7=NULL, T* p8=NULL, T* p9=NULL)
    {
        GetV(SpecName, (T*)GetFormat, p1, p2, p3, p4, p5, p6, p7, p8, p9);
    }
    //-------------------------------------------------------------------------------

    //-------------------------------------------------------------------------------
    // Get a variable length list of parameters from a spec.  T can
    // be an int, a double, or a CString.
    //-------------------------------------------------------------------------------
    template<class T> void GetList(CString SpecName, vector<T>* vec)
    {
        T buffer[MAX_LIST];

        // Fetch the specs into a temporary array
        int iCount = GetV(SpecName, (T*)GetListFormat, buffer);

        // Empty our output vector and make enough space to hold the list
        vec->clear();
        vec->reserve(iCount);

        // Copy the array into the vector
        for (int i=0; i<iCount; ++i) vec->push_back(buffer[i]);
    }
    //-------------------------------------------------------------------------------

    // Call this to get a multi-part spec where each part is
    // a different type.  (This is *rarely* useful)
    int     GetV(CString tag, void* fmt,
					 void* p1=NULL, void* p2=NULL, void* p3=NULL,
					 void* p4=NULL, void* p5=NULL, void* p6=NULL,
					 void* p7=NULL, void* p8=NULL, void* p9=NULL);
    

//============================================================================
private:

    //========================================================================
    // Struct SpecLine - Holds information that represents a line from a
    //                   spec-file.  The CSpecFile class has an array of these
    //                   structures that represents the entire file.
    //========================================================================
    struct SpecLine
    {
        // The section this spec belongs to
        CString SectType;

        // The section value for this spec
        CString SectValue;
        
        // Everything on the line before the equal-sign.
        CString Key;
    
        // Everything on the line after the equal-sign
        CString Value;

        // Constructor
        SpecLine() {Key = Value = SectType = SectValue = "";}
    };
    //========================================================================

    // This is the maximum number of items allowed in a list spec
    enum        {MAX_LIST = 100};

    // Call this to find they value string of a spec
    TCHAR*       Find(CString sSpecName, bool bAbortOnFail = true);

    // Used to extract a string from a buffer
    void        FetchStringSpec(void* dst, void* src);        

    // Used to split a string into two halves at a delimiter
    void        Split(TCHAR* in, int iDelim, CString* sLeft, CString* sRight);

    // Checks to see if a given section type:name is enabled
    bool        SectionEnabled(CString sType, CString sVlaueName);

    // Call this to either throw an error or popup a message box
    void        ReportError(CString fmt, ...);

    // This is true if "FindSpec" should throw errors
    bool        m_bThrow;

    // This is true if the spec-file we read in was cloaked
    bool        m_bIsCloaked;

    // This is the name of our spec-file
    CString     m_sFilename;

    // These are the parameters and their values
    vector<SpecLine> m_Line;
    
    // This is a hash-table of enabled sections
    map<CString, CString> m_Section;

    // This casts a data type into a pointer to a "Get()" format string
    struct CGetFormat
    {
        operator int    *() {return (int    *) "iiiiiiiii";}
        operator double *() {return (double *) "fffffffff";}
        operator CString*() {return (CString*) "sssssssss";}
        operator char   *() {return (char   *) "ccccccccc";}
        operator bool   *() {return (bool   *) "bbbbbbbbb";}

    } GetFormat;

    // This casts a data type into a pointer to a "GetList()" format string
    struct CGetListFormat
    {
        operator int*    () {return (int    *) "vI";}
        operator double* () {return (double *) "vF";}
        operator CString*() {return (CString*) "vS";}
        operator bool*   () {return (bool   *) "vB";}
    } GetListFormat;

//============================================================================


};
//============================================================================

