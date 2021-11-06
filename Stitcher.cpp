#include "stdafx.h"
#include "Stitcher.h"
#include "Globals.h"
#include "WinUtilsImp.h"
#include <vector>
using std::vector;


//=========================================================================================================
// This is the structure of the header for an image file in the BMP format
//=========================================================================================================
#pragma pack(push, 1)
struct BITMAPHDR
{
    U8  magic[2];
    U32 total_size;
    U32 reserved1;
    U32 pixel_offset;

    U32 hdr_size;
    U32 width;
    S32 height;
    U16 planes;
    U16 bitcount;
    U8  reserved2[24];
};
#pragma pack(pop)
//=========================================================================================================


//=========================================================================================================
// This class manages a vector full of these objects
//=========================================================================================================
struct sfile
{
    CString fn;
    FILE*   ifile;
    U32     pixel_bytes;
    U32     padding_bytes;
};
//=========================================================================================================


//=========================================================================================================
// Globals local to this file
//=========================================================================================================
static vector<sfile> fvec;
//=========================================================================================================


//=========================================================================================================
// Init() - Initializes before starting a new stitching job
//=========================================================================================================
void CStitcher::Init()
{
    // Make sure all files are closed
    CloseFiles();

    // Get rid of any existing sfile records
    fvec.clear();
}
//=========================================================================================================



//=========================================================================================================
// AddFile() - Adds a file to our list, for later processing
//=========================================================================================================
void CStitcher::AddFile(CString fn)
{
    // Create an sfile record from the filename we were passed
    sfile sf = {fn, nullptr, 0};

    // And add this to the list of files we are going to stitch together
    fvec.push_back(sf);
}
//=========================================================================================================


//=========================================================================================================
// CloseFiles() - Closes any open files
//=========================================================================================================
void CStitcher::CloseFiles(bool erase)
{
    // Loop through each sfile record...
    for (unsigned i = 0; i < fvec.size(); ++i)
    {
        // Get a handy reference to this sfile record
        sfile& sf = fvec[i];

        // If the file is open, close it
        if (sf.ifile)
        {
            fclose(sf.ifile);
            sf.ifile = nullptr;
        }

        // If we're supposed to erase the panel files, do so
        if (erase) DeleteFile(sf.fn);
    }
}
//=========================================================================================================


//=========================================================================================================
// OpenFiles() - Opens all of the input files and reads their headers
//=========================================================================================================
bool CStitcher::OpenFiles()
{
    BITMAPHDR   bmp;

    // Make sure that none of the input files is already open
    CloseFiles(false);

    // We will be keeping track of the total number of columns in all input file
    m_out_cols = 0;

    // Loop through each sfile record...
    for (unsigned i = 0; i < fvec.size(); ++i)
    {
        // Get a handy reference to this sfile record
        sfile& sf = fvec[i];

        // Try to open this file
        if (_wfopen_s(&sf.ifile, sf.fn, L"rb") != 0)
        {
            sf.ifile = nullptr;
            CloseFiles();
            return false;
        }

        // Read in the bitmap header
        fread(&bmp, 1, sizeof bmp, sf.ifile);

        // Compute how many bytes of pixels each row has
        sf.pixel_bytes = bmp.width * 3;

        // Compute how many bytes of padding each row has
        sf.padding_bytes = (4 - sf.pixel_bytes % 4) % 4;

        // Keep track of the total number of columns in our eventual output file
        m_out_cols += bmp.width;

        // Keep track of how many rows our output file will have
        m_out_rows = bmp.height;
    }


    // Compute the number of bytes in a row, including padding
    m_padded_row_length = m_out_cols * 3;
    while (m_padded_row_length % 4) ++m_padded_row_length;

    // Tell the caller that all of the input files are open
    return TRUE;
}
//=========================================================================================================



//=========================================================================================================
// CreateOutputFile() - Creates and output BMP file and writes its header
//=========================================================================================================
FILE* CStitcher::CreateOutputFile(CString fn, U32 cols, U32 rows)
{
    BITMAPHDR   hdr;
    FILE*       ofile = nullptr;

    // Create the output file;
    if (_wfopen_s(&ofile, fn, L"wb") != 0) return nullptr;

    // Create the bitmap header we're about to write
    memset(&hdr, 0, sizeof hdr);

    // Fill in the signature
    hdr.magic[0] = 'B';
    hdr.magic[1] = 'M';

    // Fill in the total size of the file
    hdr.total_size = sizeof(hdr) + m_padded_row_length * rows;

    // Offset (in the file) to the pixel data
    hdr.pixel_offset = sizeof hdr;

    // Fill in the dimensions of the image
    hdr.width = cols;
    hdr.height = rows;

    // We're using the standard 40-byte BMP image header
    hdr.hdr_size = 40;

    // Only 1 bitplane
    hdr.planes = 1;

    // We're writing 24 bits per pixel
    hdr.bitcount = 24;

    // Write the file header to the file
    fwrite(&hdr, 1, sizeof hdr , ofile);

    // Hand the output FILE* to the caller
    return ofile;
}
//=========================================================================================================


//=========================================================================================================
// Stitch() - Stitches together all of the input files into a single output file
//=========================================================================================================
bool CStitcher::Stitch(CString output_fn)
{
    // If there's only one input file, no stitching required
    if (fvec.size() == 1)
    {
        DeleteFile(output_fn);
        MoveFile(fvec[0].fn, output_fn);
        return true;
    }

    // Open all of the input files
    if (!OpenFiles()) return FALSE;

    // Create the output file
    FILE* ofile = CreateOutputFile(output_fn, m_out_cols, m_out_rows);

    // If we couldn't create the output file, close the inputs and tell the caller
    if (ofile == nullptr)
    {
        CloseFiles(true);
        return FALSE;
    }

    // Loop through each row of the bitmap
    for (U32 row = 0; row < m_out_rows; ++row)
    {
        // Point to the buffer we're we will collect pixels
        U8* p = (U8*)panel;
       
        // Loop through each input file
        for (U32 file = 0; file < fvec.size(); ++file)
        {
            // Get a handy reference to this sfile record
            sfile& sf = fvec[file];

            // Read in an entire row of pixels (plus padding bytes)
            fread(p, 1, sf.pixel_bytes + sf.padding_bytes, sf.ifile);

            // Move our pointer to the end of the pixels we just read in
            p += sf.pixel_bytes;
        }

        // Append some padding bytes to the end of the row of pixels
        *p++ = 0;
        *p++ = 0;
        *p++ = 0;

        // And write this row of pixels to the output file
        fwrite(panel, 1, m_padded_row_length, ofile);
    }

    // Close the output file
    fclose(ofile);

    // Close all of the input files
    CloseFiles(true);

    // And tell the caller that we have just stitched together an output file
    return true;
}
//=========================================================================================================
