#pragma once
#include "stdafx.h"
#include "typedefs.h"

//=========================================================================================================
// class CStitcher - Stitches bitmap files together
//=========================================================================================================
class CStitcher
{
public:

    // Default constructor
    CStitcher() {Init();}

    // Call this to initialize for a new session
    void    Init();

    // Call this to add a file
    void    AddFile(CString fn);

    // Call this to stitch those files together
    bool    Stitch(CString new_fn);

    // Call this to delete files that have been added via AddFile()
    void    Cleanup() {CloseFiles(true);}

protected:

    // Call this to open all of the files
    bool    OpenFiles();

    // Call this to close all of the files
    void    CloseFiles(bool erase = false);

    // Create the output file, including writing the header
    FILE*   CreateOutputFile(CString fn, U32 cols, U32 rows);

    // Number of rows and columns in the output file
    U32     m_out_cols;
    U32     m_out_rows;

    // The length (in bytes) of a row of pixels, including padding bytes
    U32     m_padded_row_length;
};
//=========================================================================================================