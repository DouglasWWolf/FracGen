//============================================================================
//                             #includes
//============================================================================
#include <stdlib.h>
#include <stdio.h>
#include "stdafx.h"
#include "SpecFile.h"
#include <vector>
using std::vector;
//============================================================================


//============================================================================
// Constructor() - Reserves some space in the vectors and turns off 
//                 exception throwing
//============================================================================
CSpecFile::CSpecFile()
{
    m_Line.reserve(100);
    m_bThrow = false;
}
//============================================================================




//============================================================================
// Read() - Read in all the lines of a spec file
//
// On Exit: m_iSectionCount = 0;
//          m_Line vector contains spec records
//============================================================================
bool CSpecFile::Read(CString sFilename, bool bDialogOnFail) 
{
    char         ascii[500];
    TCHAR        inbuf[1000], *in, *p;
    SpecLine     line;
    CString      currentMacro;
    U32          i;
    FILE*        istream;

    // This is a vector of spec-lines
    typedef vector<SpecLine> SpecLineV;
    
    // This maps a macro-name to a SpecLine vector    
    map<CString, SpecLineV> MacroMap;    
    
    // This is an empty SpecLine vector
    SpecLineV    emptyMacro;

    // Memorize our filename for later use
    m_sFilename = sFilename;

    // Start out assuming that we're not in the middle of reading in
    // a script
    bool        bInScript = false;

    // Start out assuming there are no specs yet.
    m_Line.clear();
    m_Section.clear();
   
    // If we couldn't open the file, warn the user.
    if (_wfopen_s(&istream, sFilename, L"r") != 0)
    {
        if (bDialogOnFail) Popup(L"\nFailed to open file \"%s\"", sFilename);
		return false;
    }

	// Loop through each line of the spec file, one line at a time...
    while (fgets(ascii, sizeof ascii, istream))
	{
        // Covert the line from ASCII to Unicode
        mbstowcs_s(NULL, inbuf, sizeof(inbuf) / 2, ascii, _TRUNCATE);

        // Convert any tabs to spaces
        in = inbuf;
        while ((in = wcschr(in, 9)) != NULL) *in++ = ' ';

        // Point to the text line we just read from the file, with
        // any terminating carriage return or linefeed chopped off
        in = Chomp(inbuf);
        
        // Skip over leading spaces
        while (*in == 32) ++in;

        // If the line is blank, ignore it
        if (in[0] == 0) continue;

        // Ignore any line that begins with //
        if (in[0] == '/' && in[1] == '/') continue;

        // If this is a macro reference
        if (in[0] == '#')
        {
            // Bump the input pointer past the pound sign
            ++in;
            
            // Fetch a normalized version of the macro name
            CString macroName = in;
            macroName.TrimLeft();
            macroName.TrimRight();
            macroName.MakeUpper();

            // Get a reference to the spec-lines for this macro name
            SpecLineV& src = MacroMap[macroName];

            // If we're in the middle of a macro definition, the referenced macro will
            // be expanded into the current macro definition.  If we're not in the
            // middle of a macro definition, the referenced macro will be expanded
            // into the main "m_Line" vector
            SpecLineV *dst = (currentMacro.IsEmpty() ? &m_Line : &MacroMap[currentMacro]);

            // Append all the lines of this macro into the destination vector.  While
            // we're appending, make sure we stamp the current sectiom type and section
            // value into the lines being appended
            for (i=0; i<src.size(); ++i) 
            {
                SpecLine temp  = src[i];
                temp.SectType  = line.SectType;
                temp.SectValue = line.SectValue;
                dst->push_back(temp);
            }

            // And go fetch the next line of specs
            continue;
        }

        // If the line begins with an open brace, it's a marker that
        // we're about to start reading in a script
        if (in[0] == '{')
        {
            bInScript = true;
            continue;
        }

        // If this line begins with an open-bracket, it means it's a 
        // section type and section name (as opposed to a spec)
        if (in[0] == '[')
        {
            // Bump the input pointer past the open-bracket
            ++in;

            // If we can find a closing bracket, consider that to be
            // the end of the line
            p = wcschr(in, ']'); if (p) *p = 0;

            // Fetch the section type and section name
            Split(in, ':', &line.SectType, &line.SectValue);

            // Make the section type and section name uppercase
            line.SectType.MakeUpper();
            line.SectValue.MakeUpper();

            // And go process the next line
            continue;
        }

        // If this line begins with a '<', it either begins or ends a macro
        if (in[0] == '<')
        {
            // Bump the input pointer past the open-bracket
            ++in;

            // If we can find a closing bracket, consider it to be the
            // end of the line
            p = wcschr(in, '>'); if (p) *p = 0;

            // Fetch the name of the current macro, trim off the leading and 
            // trailing spaces, and convert it to uppercase
            currentMacro = in;
            currentMacro.TrimLeft();
            currentMacro.TrimRight();
            currentMacro.MakeUpper();

            // If there is a macro name, create a blank entry for this macro
            if (!currentMacro.IsEmpty()) MacroMap[currentMacro] = emptyMacro;

            // And go process the next line
            continue;
        }

        // If this is a line from a script...
        if (bInScript)
        {
            line.Key = "";
            line.Value = in;
            if (in[0] == '}') bInScript = false;
        }

        // Otherwise, it's a regular parameter line
        else Split(in, '=', &line.Key, &line.Value);

        // Make the spec-name field upper-case
        line.Key.MakeUpper();
        
        // And save this spec-line
        if (currentMacro.IsEmpty())
            m_Line.push_back(line);
        else
            MacroMap[currentMacro].push_back(line);
	}

    // Close the spec file
    fclose(istream);

    // Tell the caller that all is well
    return true;
}
//============================================================================




//============================================================================
// FindSpec() - Find the value string associated with a particular spec
//
// Passed:  sSpecName    = Specification name
//          bAbortOnFail = true if routine should abort of spec not found
//
// Returns: char* to value string
//
//          if spec not found and bAbortOnFail = true, this routine
//          never returns.
//          
//          if spec not found and bAbortOnFail = false, this routine
//          returns NULL
//
//============================================================================
TCHAR* CSpecFile::Find(CString sSpecName, bool bAbortOnFail)
{
    // Spec names should always be in upper-case  (this makes all
    // the string compares case-insensitive)
    sSpecName.MakeUpper();

    // Loop through each spec string we know about
    for (U32 i=0; i<m_Line.size(); i++)
    {
        // If this is the one we're searching for...
        if (m_Line[i].Key == sSpecName)
        {
            // If this section in the spec-file is enabled...
            if (SectionEnabled(m_Line[i].SectType, m_Line[i].SectValue))
            {
                // Return a pointer to its associated values
                return m_Line[i].Value.GetBuffer(1);
            }
        }
    }

    // If we're supposed to abort on a failure, do so.
    if (bAbortOnFail) ReportError(L"Missing spec %s in %s", sSpecName, m_sFilename);

    // Otherwise, tell the caller we couldn't find his spec.
    return NULL;
}
//============================================================================



//============================================================================
// Get() - Fetches a script from the spec file
//
// Passed:  SpecName = Name of spec we're looking for
//
// Returns: CScript object.
//
//          if spec not found this routine never returns.
//
//============================================================================
void CSpecFile::Get(CString sSpecName, CScript* retval)
{
    U32     i;

    // Spec names should always be in upper-case  (this makes all
    // the string compares case-insensitive)
    sSpecName.MakeUpper();

    // Erase any currently existing script data
    retval->MakeEmpty();

    // Loop through each spec string we know about
    for (i=0; i<m_Line.size(); i++)
    {
        // If this is the one we're searching for...
        if (m_Line[i].Key == sSpecName)
        {
            // If this section in the spec-file is enabled...
            if (SectionEnabled(m_Line[i].SectType, m_Line[i].SectValue))
            {
                // Stop looking.  We found our script.
                break;                
            }
        }
    }
    
    // If the spec name didn't exist, bitch.  (ReportError() never returns)
    if (i == m_Line.size())
    {
        ReportError(L"Missing spec %s in %s", sSpecName, m_sFilename);
    }

    // Build the script from our specs
    while (m_Line[++i].Value.Left(1) != "}" && i<m_Line.size())
    {
        retval->AddLine(m_Line[i].Value);
    }

}
//============================================================================







//============================================================================
// FetchStringSpec() - Fetches and space-trims a string that could be
//                     terminated with a 0, a comma, or a linefeed.
//
// Passed:  void* = Destination byte array
//          void* = Source byte array
//============================================================================
void CSpecFile::FetchStringSpec(void* dst, void* src)
{
    // Get a pointer to the source
    char* in = (char*)src;

    // Get a pointer to the destination
    char* out = (char*)dst;
    
    // Skip over any leading spaces
    while (*in == 32) ++in;

    // Move the data until we hit a string terminator
    while (!(*in == 0 || *in == ',' || *in == '\n')) *out++ = *in++;

    // Trim trailing spaces and tabs.
    while (out > dst && (*(out-1) == 32)) --out;

    // Terminate the string with a null byte
    *out = 0;
}
//============================================================================


//============================================================================
// GetV() - Fetchs the values of a "variable field" spec
//
// Passed: char* = ASCII name of parameter in spec-file
//
//         void* = Format specifier for each variable.
//                 Format "F" gets decoded as a "double float"
//                 Format "I" gets decoded as an integer
//                 Format "C" gets decoded as an char array
//                 Format "S" gets decoded as a CString object
//                 Format "B" gets decoded as a boolean
//
//                 Format "V" means "Variable length list spec"
//                 with the next character in the format specifier
//                 indicating what type of list it is.
//
//         void* = Pointers to the output fields.  All unused output fields
//                 should have their pointers set to NULL.  (This is done
//                 by the function prototype in the class)
//
//============================================================================
int CSpecFile::GetV(CString tag, void* vpFormat,
                        void* p1, void* p2, void* p3, void* p4, void* p5,
                        void* p6, void* p7, void* p8, void* p9)
{
    int     i;
    int     iSpecType, iListSpec = 0, iItemCount = 0;
    CString sv;
    TCHAR   buffer[400];
    void*   pointers[MAX_LIST];

    // Get a char* pointer to the format string
    TCHAR* fmt = (TCHAR*)vpFormat;

    // Make an array out of our pointers
    void* callerPointers[] = {p1, p2, p3, p4, p5, p6, p7, p8, p9, NULL};

    // Make "out" point to the list of pointers specified by the caller
    void** out = callerPointers;

    // If the format string indicated that this is a variable length 
    // "List Spec"...
    if (fmt[0] == 'v' || fmt[0] == 'V')
    {
        // We're going to use one single format specifier
        // (indicated by the 2nd character of the format string)
        // for all parameters.        
        iListSpec = toupper(fmt[1]);

        // Build a list of pointers to the entries in the array
        // given to us by the caller.
        for (i=0; i<sizeofarray(pointers); i++)
        {
            pointers[i] = ((void**)p1) + i;
        }

        // And our output array is going to be the memory region
        // that was specified by the caller
        out = pointers;
    }

    // This is our array index
    i = -1;

    // Find the spec name supplied by the caller
    TCHAR* pbuf = Find(tag);
    
    // Loop through each output field until we hit a NULL
    while (out[++i])
    {
        // The spec type is our "List Spec" (if there is one)
        // or the uppercase spec parameter provided in the 
        // format string that was passed in by the caller.
        iSpecType = (iListSpec)? iListSpec : toupper(fmt[i]);        
        
        // Take a look at our format specifier...
        switch (iSpecType)
        {
                        
        // If the format specifier is "boolean"...
        case 'B':
            sv = pbuf;
            sv.MakeUpper();
            *(bool*)out[i] = (sv == L"TRUE" || sv == L"ON" || _wtoi(pbuf) != 0);
            break;

        // If the format specifier is "floating point",
        // decode this value as a "double"
        case 'F':
            *(double*)out[i] = _wtof(pbuf);
            break;

        // If the format specifier is "integer"
        // decode this value as an integer
        case 'I':
            while (*pbuf == ' ') ++pbuf;
            if (pbuf[0] == '0' && toupper(pbuf[1]) == 'X')
                swscanf_s(pbuf+2, L"%X", (unsigned*)out[i]);
            else
                *(int*)out[i] =_wtoi(pbuf);
            break;

        // If the format specifier is "char array"
        // decode this value as a string of ASCII characters
        case 'C':
            FetchStringSpec(out[i], pbuf);
            break;

        // If the format specifier is "String"
        // decode this value as a CString
        case 'S':
            FetchStringSpec(buffer, pbuf);
            *(CString*)out[i] = buffer;
            break;

        };

        // We've just decoded one item from the list
        ++iItemCount;

        // Find the next comma
        pbuf = wcschr(pbuf, L',');

        // If we're processing a variable length list spec
        // and if there are no more commas, we're done.
        if (iListSpec && pbuf == 0) break;
         
        // If we found a comma, point to the next character.
        // If there wasn't a comma, point to an empty string.
        pbuf = (pbuf)? pbuf+1 : L"";

    }

    // Tell the caller how many items were attached to this spec
    return iItemCount;
}
//============================================================================




//============================================================================
// Get() - Fetches the values of a Range spec
//============================================================================
void CSpecFile::Get(CString tag, Range* r)
{
    double  float1, float2;
    Get(tag, &float1, &float2);
    r->Set(float1, float2);
}
//============================================================================




//============================================================================
// Split() - Looks for a delimiter in an input string and splits it into
//           two space-trimmed output strings.
//
// Passed:  in     = Pointer to input string
//          iDelim = Delimiter to split on
//          sLeft  = Pointer to "left side of delimiter" output string
//          sRight = Pointer to "right side of delimiter" output string
//
//============================================================================
void CSpecFile::Split(TCHAR* in, int iDelim, CString* sLeft, CString* sRight)
{
    TCHAR   buffer[1000];

    // Assume for the moment that both output strings will be blank
    *sLeft = *sRight = "";

    // Point to the output buffer that will temporarily
    // hold the string we're parsing
    TCHAR *out = buffer;

    // Point to the string that will receive the text we're parsing
    CString* pString = sLeft;

    // Parse characters indefinitely...
    while (1)
    {
        // Fetch the next character
        int c = *in++;

        // If we've hit the end of the line, stop parsing
        if (c == 0 || c == '\n') break;

        // If we're parsing on the left side of the delimiter
        // and we just hit the delimiter...
        if (c == iDelim && pString == sLeft)
        {
            // Terminate the character string in the buffer
            *out = 0;
            
            // Copy the buffer into our "left side" output string
            *sLeft = TrimAll(buffer);

            // Now prepare to parse the "right side" output string
            out = buffer;
            pString = sRight;

            // Continue parsing with the next character
            continue;
        }

        // Place this character into the output buffer
        *out++ = c;
    }

    // Terminate the character string in the buffer
    *out = 0;
    
    // Save the buffered text to the appropriate output string
    *pString = TrimAll(buffer);
}
//============================================================================


//============================================================================
// EnableSection() - Declares a spec section to be "enabled"
//
// Passed:  sType  = Section type
//          iValue = Numeric section value
//============================================================================
void CSpecFile::EnableSection(CString sType, int iValue)
{
    CString ASCII;
    ASCII.Format(_T("%i"), iValue);
    EnableSection(sType, ASCII);
}
//============================================================================




//============================================================================
// EnableSection() - Declares a spec section to be "enabled"
//
// Passed:  sType = Section Type
//          sName = Section Name
//============================================================================
void CSpecFile::EnableSection(CString sType, CString sValue)
{
    // Map this value to this section type
    m_Section[MakeUpper(sType)] = MakeUpper(sValue);
}
//============================================================================


//============================================================================
// SectionEnabled() - Checks to see if a section is enabled
//
// Passed:  sType  = Section type
//          sValue = Value of section
//
// Returns: true if this section type is set to this value
//============================================================================
bool CSpecFile::SectionEnabled(CString sType, CString sValue)
{
    // If they passed us a blank section type, return true
    // (Because the "empty section" is *always* enabled)
    if (sType.IsEmpty()) return true;

    // Make both input values uppercase (since that's the way
    // they're stored in the map)
    sType.MakeUpper();
    sValue.MakeUpper();

    // If the section type is in our has table, tell the caller
    // whether the section value matches the he specified
    if (m_Section.find(sType) != m_Section.end())
    {
        return (m_Section[sType] == sValue);
    }

    // If we get here, the section type wasn't in the table.
    return false;
}
//============================================================================

//============================================================================
// ReportError() - Notify the user or caller that an error has occured
//============================================================================
void CSpecFile::ReportError(CString fmt, ...)
{
    CString msg;

    va_list FirstArg;
    va_start(FirstArg, fmt);
    msg.FormatV(fmt, FirstArg);
    va_end(FirstArg);
    
    // If we're supposed to throw errors, do so
    if (m_bThrow) throw CSpecFileErr(msg);

    // Otherwise popup a box and warn the user
    Popup(msg);

    // And call it quits
    exit(1);
}
//============================================================================


//============================================================================
// Exists() - Checks to see if a spec-name exists in the spec-file
//============================================================================
bool CSpecFile::Exists(CString sSpecName)
{
    // Tell the caller whether the spec-name is found or not
    return (Find(sSpecName, false) != NULL);
}
//============================================================================

//============================================================================
// AddLine() - Adds a line to a CScript object.  Internally, these lines
//             are stored as a vector.
//============================================================================
void CScript::AddLine(CString sLine)
{
    // Add this string to our vector
    m_Line.push_back(sLine);
}
//============================================================================


//============================================================================
// GetNextLine() - Moves the internal cursor to the next script line
//
// Passed: Option CString pointer to receive the line of text
//
// Returns: 'true' if there is a script line available
//============================================================================
bool CScript::GetNextLine(CString* p)
{
    // If we've fetched all the lines, tell the caller there
    // are no more lines available.
    if (m_iCurrentLine >= m_Line.size())
    {
        if (p) *p = "";
        return false;
    }
    
    // Reset the index of the next word to be fetched via
    // GetNextWord() or GetNextInt()
    m_iCurrentWord = 0;

    // If the caller handed us a pointer, hand him back his line
    if (p)
    {
        *p = TrimAll(m_Line[m_iCurrentLine]);
    }

    // Split this line up into words
    Parse(m_Line[m_iCurrentLine++]);

    // Tell the caller we have a line for him.
    return true;
}
//============================================================================



//============================================================================
// Constructor() - Reserves some space in the vectors
//============================================================================
CScript::CScript()
{
    // Reserve some space in the vectors
    m_Line.reserve(100);
    m_Word.reserve(10);
 
    // And make the script empty
    MakeEmpty();
}
//============================================================================


//============================================================================
// Parse() - Tear a line apart into words
//
// On Exit: vector m_Word is filled with space-trimmed words
//============================================================================
void CScript::Parse(CString sIn)
{
    int     c, APPEND = 10000;

    // Get a pointer to our input string.
    TCHAR* in = sIn.GetBuffer(1);

    // Erase the vector of words
    m_Word.clear();

NextWord:

    // The word we're parsing starts out empty
    CString sWord = _T("");

    // Skip over spaces and commas
    while (*in == ' ' || *in == ',') ++in;

    // Set a flag that says whether or not this string is quoted
    bool bQuoted = (*in == 34);

    // If this is a quoted string, skip over the quotation marks
    if (bQuoted) ++in;

    // Loop continuously through input characters...
    while (1)
    {
        // Fetch the next character
        c = *in++;
        
        // If we've hit the end of the string, we're done with this word
        if (c == 0) break;
        
        // If we've hit a separator, we're done with this word.
        if (bQuoted)
        {
            if (c == 34) break;
        }
        else
        {
            if (c == ',' || c == ' ') break;
        }

        // Append this character to the word we're building
        sWord.Insert(APPEND, c);
    }

    // Trim this word of spaces and add it to our vector
    sWord.TrimRight();
    sWord.TrimLeft();
    m_Word.push_back(sWord);

    // If the last word terminator was not a nul, go process
    // the next word.
    if (c) goto NextWord;

}
//============================================================================


//============================================================================
// GetNextWord() - Returns the next word from the current line
//============================================================================
CString CScript::GetNextWord(bool bMakeUpper)
{
    CString sWord;
    
    // If the caller is fetching off the end of the vector,
    // hand him an empty string.
    if (m_iCurrentWord >= m_Word.size()) return _T("");
    
    // Otherwise, fetch the next word
    sWord = m_Word[m_iCurrentWord++];

    // If we were told to make it uppercase, do so
    if (bMakeUpper) sWord.MakeUpper();

    // And hand it to the caller;
    return sWord;
}
//============================================================================


//============================================================================
// GetNextInt() - Returns the next integer from the current line
//============================================================================
int CScript::GetNextInt()
{
    int value;

    // Fetch the next word
    CString sWord = GetNextWord();

    // Make it lower-case
    sWord.MakeLower();

    // If the first two characters are "0x"...
    if (sWord.Left(2) == _T("0x"))
    {
        // Assume this value is in ASCII HEX
        swscanf_s(sWord.GetBuffer()+2, _T("%X"), &value);
    }
   
    // Otherwise assume this value is in ASCII decimal.
    else value = _wtoi(sWord.GetBuffer());

    // Hand the integer back to the caller
    return value;
}
//============================================================================


//============================================================================
// GetNextFloat() - Returns the floating point value from the current line
//============================================================================
double CScript::GetNextFloat()
{
    // Return the floating point value of the next word
    return _wtof(GetNextWord());
}
//============================================================================




//============================================================================
// Set() - Sets the values of a range
//============================================================================
void Range::Set(double min, double max)
{
    fMin =       min; fMax =       max;
     Min = (int) min;  Max = (int) max;
}
//============================================================================

//============================================================================
// Contains() - Returns 'true' if the range includes the specified value
//============================================================================
bool Range::Contains(double val)
{
    return (val >= fMin && val <= fMax);
}
//============================================================================


//============================================================================
// S() - Returns a string version of the range
//
// Passed: fmt = optional formatting parameter for min and max values
//============================================================================
CString Range::S(char* fmt)
{
    CString ret, format;

    // If the range is an integer range...
    if (Min == fMin && Max == fMax)
    {
        if (fmt == NULL) fmt = "%i";
        format.Format(_T("%s and %s"), fmt, fmt);
        ret.Format(format, Min, Max);
    }

    // Otherwise, it's a floating point range
    else
    {
        if (fmt == NULL) fmt = "%1.2lf";
        format.Format(_T("%s and %s"), fmt, fmt);
        ret.Format(format, fMin, fMax);
    }

    // Hand the caller the resultant string
    return ret;
}
//============================================================================
