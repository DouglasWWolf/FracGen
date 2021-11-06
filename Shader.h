//=========================================================================================================
// Shader.h - Describes a pixel shader
//=========================================================================================================
#pragma once
#include "typedefs.h"

//=========================================================================================================
// Color schemes for use with "SetColorScheme"
//=========================================================================================================
enum
{
    CS_DEFAULT      = 0,
    CS_FIXED_HUE    = 1,
    CS_OBW_LINEAR   = 2,
    CS_MONOCHROME   = 3,
    CS_OBW_GRADIENT = 4
};
//=========================================================================================================


//=========================================================================================================
// CShader - Defines the pixel shader
//=========================================================================================================
class CShader
{
public:

    // Constructor
    CShader();

    // Sets the color scheme
    void    SetScheme(int scheme);

    // Fetches the color scheme
    int     GetScheme() {return m_scheme;}

    // Returns a color that corresponds to the current color scheme
    pixel   GetColor(frac_value& v);

    // Call this to set the fixed-hue for fixed-hue color schemes
    void    SetFixedHue(double hue);

    // Initializes the scheme names into a combo-box
    void    InitSchemeNames(CComboBox* pCB);

    // Dumps a .bmp file of the palette for debugging purposes
    void    DumpPalette();
    
protected:

    // Creates a palette for the fancy shading styles
    pixel   palette0(double d);
    pixel   palette2(double d);

    // Initializes the OBW_PALETTE color scheme
    void    Init_OBW_LINEAR();

    // Initializes the Orange/Blue/White gradient
    void    Init_OBW_Gradient();
    
    // Routines for translating an mandelbrot escape value to a pixel shade
    pixel   GetRawColor (escape& e);
    pixel   GetRawColor0(escape& e);
    pixel   GetRawColor1(escape& e);
    pixel   GetRawColor2(escape& e);
    pixel   GetRawColor3(escape& e);

    // Current color scheme ID
    int     m_scheme;

    // The fixed hue for fixed-hue coloring schemes
    double  m_fixed_hue;

    // Current palette
    pixel   m_palette[2000];

    // The orange/blue/white gradient
    pixel   m_obw_gradient[2048];

    // Number of entries in current palette
    U32     m_palette_size;
};
//=========================================================================================================