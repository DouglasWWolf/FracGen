//=========================================================================================================
// Image.cpp - Provides infrastructure for writing bitmap files, raw files, etc.
//=========================================================================================================
#include "stdafx.h"
#include "typedefs.h"

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
// WriteBmp() - Writes a bitmap image file
//=========================================================================================================
bool WriteBmp(CString fn, pixel* image, U32 cols, U32 rows, U32 panel_width)
{
    BITMAPHDR hdr;
    FILE*     ofile;
     
    // Create and open the output file
    if (_wfopen_s(&ofile, fn, L"wb") != 0) return false;
    
    // This is a row of pixels
    U8*  row = new U8[cols * 3 + 3];

    // If no panel-width was defined, the panel width is width of our data
    if (panel_width == 0) panel_width = cols;

    // We're going to store 3 bytes per pixel (R, G, and B)
    int bytes_per_pixel = 3;

    // Find out the raw number of bytes per row 
    int raw_row_length = bytes_per_pixel * cols;

    // In the file, a row must be padded such that it's length is divisble by 4
    int padding = 4 - (raw_row_length % 4);  
    if (padding == 4) padding = 0;

    // Compute the number of bytes in a row of pixels (in the file)
    int padded_row_length = raw_row_length + padding;

    // Clear the header to all zeros
    memset(&hdr, 0, sizeof hdr);

    // Fill in the signature
    hdr.magic[0] = 'B';
    hdr.magic[1] = 'M';

    // Fill in the total size of the file
    hdr.total_size = sizeof(hdr) + padded_row_length * rows;

    // Offset (in the file) to the pixel data
    hdr.pixel_offset = sizeof hdr;

    // Fill in the dimensions of the image
    hdr.width  = cols;
    hdr.height = rows;

    // We're using the standard 40-byte BMP image header
    hdr.hdr_size = 40;

    // Only 1 bitplane
    hdr.planes = 1;

    // We're writing 24 bits per pixel
    hdr.bitcount = 24;

    // Write the file header to the file
    fwrite(&hdr, 1, sizeof(hdr), ofile);

    // Loop through each row of the image
    for (U32 y=0; y<rows; ++y)
    {
        // Point to the buffer where we create the row of pixels
        U8* out = row;

        // Compute the scanline.  We write rows from bottom to top    
        U32 scanline = rows - y - 1;

        // Point to the first pixel in that scanline
        pixel* p_pixel = image + scanline * panel_width;

        // Create the row of pixels
        for (U32 x=0; x<cols; ++x)
        {
            *out++ = p_pixel->b;
            *out++ = p_pixel->g;
            *out++ = p_pixel->r;
            ++p_pixel;
        }

        *out++ = 0;
        *out++ = 0;
        *out++ = 0;

        // Write this entire row to the file
        fwrite(row, 1, padded_row_length, ofile);
    }

    // We're done.  Close the file and tell the caller that all is well
    delete[] row;
    fclose(ofile);
    return true;
}
//=========================================================================================================
