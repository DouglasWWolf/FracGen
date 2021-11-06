//=========================================================================================================
// CHueIndicator.h - Defines a bitmap control that shows a hue selection 
//=========================================================================================================
#pragma once
#include "stdafx.h"
#include "typedefs.h"

const int HI_IMAGE_W = 240;
const int HI_IMAGE_H = 30;

//=========================================================================================================
// CHueIndicator() - Displays a hue graph with a marker indicated the selected hue
//=========================================================================================================
class CHueIndicator : public CStatic
{
public:

    CHueIndicator();

    // Call this to indicate the currently selected hue
    void    SetHue(double hue);

private:
    
    // Called whenever the control needs to be repainted
    void OnPaint();

    // The currently selected hue
    double  m_hue;

    // The hue bitmap
    CBitmap m_bmp;

    DECLARE_MESSAGE_MAP()

};
//=========================================================================================================

