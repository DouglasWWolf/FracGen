//=========================================================================================================
// CHueIndicator.cpp - Implements a bitmap control that shows a hue selection 
//=========================================================================================================
#include "stdafx.h"
#include "HueIndicator.h"

extern pixel hsl2rgb(double hue, double saturation, double luminosity);

const int COLS = 240;
const int ROWS = 30;

//=========================================================================================================
// Message Map for the main dialog
//=========================================================================================================
BEGIN_MESSAGE_MAP(CHueIndicator, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()
//=========================================================================================================


//=========================================================================================================
// HueIndicator() - Constructor
//=========================================================================================================
CHueIndicator::CHueIndicator() :  CStatic()
{
    pixel image[ROWS * COLS];
    
    // Make sure our initial hue is a valid value
    m_hue = 0;

    // The bitmap starts out all black
    memset(image, 0, sizeof image);

    // Loop through each horizontal pixel in the image
    for (int x = 0; x < COLS; ++x)
    {
        // Compute the hue that corresponds to this horizontal location
        double hue = (double)x / (COLS - 1);

        // Convert that to rgb
        pixel rgb = hsl2rgb(hue, .5, .5);

        // Fill in this column of pixels with the RGB value that corresponds to this hue
        for (int y = 10; y < HI_IMAGE_H; ++y) image[y * COLS + x] = rgb;
    }

    // Create a bitmap that is compatible with the screen
    m_bmp.CreateBitmap(COLS, ROWS, 1, 32, image);
}
//=========================================================================================================



//=========================================================================================================
// OnPaint() - Called anytime the indicator image must be repainted
//=========================================================================================================
void CHueIndicator::OnPaint()
{
    CPaintDC paintDC(this);
    CDC      memDC;

    // Create a device-context in memory and stuff it with our bitmap
    memDC.CreateCompatibleDC(&paintDC);
    
    // Copy our bitmap into our device context
    CBitmap* pOldBitmap = memDC.SelectObject(&m_bmp);

    // BitBlt the bitmap to the device context
    paintDC.BitBlt(0, 0, HI_IMAGE_W, HI_IMAGE_H, &memDC, 0, 0, SRCCOPY);

    // Restore the previous contents of the DC
    memDC.SelectObject(pOldBitmap);

    // Compute the place where the indicator should be drawn
    int marker = (int)(m_hue * (COLS - 1));

    // Draw the indicator line
    CPen pen(PS_DOT, 1, RGB(255, 255, 255));
    paintDC.SelectObject(&pen);
    paintDC.MoveTo(marker, 0);
    paintDC.LineTo(marker, 15);
}
//=========================================================================================================



//=========================================================================================================
// SetHue() - Sets the indicator to a hue specified by a number between 0 and 1
//=========================================================================================================
void CHueIndicator::SetHue(double hue)
{
    // Store our hue for future reference
    m_hue = hue;

    // And force a repaint of the window
    Invalidate();
}
//=========================================================================================================
