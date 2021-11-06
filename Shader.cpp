//=========================================================================================================
// Shader.cpp - Implements the pixel shader
//=========================================================================================================
#include "stdafx.h"
#include "Shader.h"
#include "typedefs.h"
#include "Globals.h"
#include "WinUtilsImp.h"
#include <math.h>

//=========================================================================================================
// Handy constants
//=========================================================================================================
static pixel black = {0, 0, 0, 255};
const double ONE_OVER_LOG2 = 1.44269504;
//=========================================================================================================


//=========================================================================================================
// hsl2rgb() - Converts hue, saturation, and luminosity values (0 to 1) to RGB values (0 thru 255)
//=========================================================================================================
pixel hsl2rgb(double h, double sl, double l)
{
 
    double v;
    double r,g,b;
    pixel result;

    // Default to grey
    r = g = b = l;  

    v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
    
    if (v > 0)
    {
        double m;
        double sv;
        int sextant;
        double fract, vsf, mid1, mid2;

        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;

        switch (sextant)
        {
            case 0:
                r = v;
                g = mid1;
                b = m;
                break;

            case 1:
                r = mid2;
                g = v;
                b = m;
                break;

            case 2:
                r = m;
                g = v;
                b = mid1;
                break;
 
            case 3:
                r = m;
                g = mid2;
                b = v;
                break;

            case 4:
                r = mid1;
                g = m;
                b = v;
                break;

            case 5:
                r = v;
                g = m;
                b = mid2;
                break;
        }
    }

    
    result.r = (U8)(r * 255);
    result.g = (U8)(g * 255);
    result.b = (U8)(b * 255);
    result.a = 255;
    return result;
}
//=========================================================================================================


//=========================================================================================================
// hslvrgb() - Converts hue, saturation, and value (0 to 1) to RGB values (0 thru 255)
//=========================================================================================================
static pixel hsv2rgb(double h, double s, double v)
{
    pixel result;
    double r, g, b;
    
    h -= floor(h);
    h *= 6.0;
    if (s < 0)  s = 0; 
    if (s > 1)  s = 1; 
    if (v < 0)  v = 0; 
    if (v > 1)  v = 1; 
    
    U32 floor_of_hue = (U32)floor(h);

    double f = h - floor_of_hue;

    switch (floor_of_hue)
    {
        case 0:
            r = 1;
            g = 1 - (1 - f) * s;
            b = 1 - s;
            break;
        case 1:
            r = 1 - s * f;
            g = 1;
            b = 1 - s;
            break;
        case 2:
            r = 1 - s;
            g = 1;
            b = 1 - (1 - f) * s;
            break;
        case 3:
            r = 1 - s;
            g = 1 - s * f;
            b = 1;
            break;
        case 4:
            r = 1 - (1 - f) * s;
            g = 1 - s;
            b = 1;
            break;
        default:
            r = 1;
            g = 1 - s;
            b = 1 - s * f;
    }

    r = r * v;
    g = g * v;
    b = b * v;

    if (r <= 0)
        result.r = 0;
    else if (r >= 0.999)
        result.r = 255;
    else 
        result.r = (U8) (r * 256);

    if (g <= 0)
        result.g = 0;
    else if (g >= 0.999)
        result.g = 255;
    else 
        result.g = (U8) (g * 256);

    if (b <= 0)
        result.b = 0;
    else if (b >= 0.999)
        result.b = 255;
    else 
        result.b = (U8) (b * 256);

    // Alpha channel is "fully opaque"
    result.a = 255;

    // Hand the resulting pixel to the caller
    return result;
}
//=========================================================================================================


//=========================================================================================================
// CShader Constructor
//=========================================================================================================
CShader::CShader()
{
    Init_OBW_Gradient();
}
//=========================================================================================================


//=========================================================================================================
// Init_OBW_Gradient() - Initializes the gradient for the Orange/Blue/White color scheme
//=========================================================================================================
void CShader::Init_OBW_Gradient()
{
    pixel p;

    // Find out how many entries are in the orange/blue/white gradient
    unsigned gradient_length = sizeofa(m_obw_gradient);

    // These are the default gradient settings from "UltraFractal".  I like their color scheme :-)
    double x[6] = {0.0000, 0.1600, 0.4200, 0.6425, 0.8575, 1.0000};
    double y_red[6] = {  0,  32, 237, 255, 0,   0};
    double y_grn[6] = {  7, 107, 255, 170, 2,   7};
    double y_blu[6] = {100, 203, 255,   0, 0, 100};

    // Create the cubic splines that define the gradient
    spline_red.  CreateInterpolant(x, y_red, 6);
    spline_green.CreateInterpolant(x, y_grn, 6);
    spline_blue. CreateInterpolant(x, y_blu, 6);

    // Build the gradient
    for (unsigned i = 0; i<gradient_length; ++i)
    {
        // Determine how far along our gradient we are (between 0 and 1)
        double ratio = i / (double)(gradient_length - 1);

        // Compute the colors for this position in the gradient
        p.r = (U8)(spline_red  .Interpolate(ratio));
        p.g = (U8)(spline_green.Interpolate(ratio));
        p.b = (U8)(spline_blue .Interpolate(ratio));
        p.a = 255;

        // Store the pixel color for this position in the gradient
        m_obw_gradient[i] = p;
    }

#if 0
    pixel* image = new pixel[2048 * 30];
    for (int y=0; y<30; ++y) memcpy(&image[y * 2048], m_obw_gradient, 2048 * sizeof pixel);
    WriteBmp(L"gradient.bmp", image, 2048, 30);
    delete[] image;
#endif
}
//=========================================================================================================


//=========================================================================================================
// ColorSpan() - Generates a spectrum of colors by varying the luminosity of a specified hue and
//               saturation.
//=========================================================================================================
static pixel* ColorSpan(double hue, double sat, double lum_start, double lum_end, int steps, pixel* dest)
{
    // How big is a color step?
    double step_size = (lum_end - lum_start) / steps;
 
    // Loop through each color we're supposed to create
    for (int i=0; i<steps; ++i)
    {
        // Compute this luminosity
        double lum = lum_start + i * step_size;

        // Generate a pixel of that color
        *dest++ = hsl2rgb(hue, sat, lum);
    }

    // Tell the caller where the pixel will be stored
    return dest;
}
//=========================================================================================================


//=========================================================================================================
// interpolate() - Interpolates a new pixel between to specified pixels
//=========================================================================================================
static pixel interpolate(pixel a, pixel b, double t)
{
    pixel result;

    result.r = (U8)(a.r + t * (b.r - a.r));
    result.g = (U8)(a.g + t * (b.g - a.g));
    result.b = (U8)(a.b + t * (b.b - a.b));
    return result;
};
//=========================================================================================================


//=========================================================================================================
// palette0() - Returns a color for the default color scheme
//=========================================================================================================
pixel CShader::palette0(double d)
{
    double h,s,v;

    if (m_scheme == CS_FIXED_HUE)
        h = m_fixed_hue;
    else
        h = d * 0.00521336;
        
    v = sin(d * 0.1847969) * 0.5 + 0.5;

    s = (sin(d * 0.162012467) * 0.5 + 0.5) * (1 - v);

    return hsv2rgb(h, s, v);
}
//=========================================================================================================


//=========================================================================================================
// palette2() - Returns a monochrome color
//=========================================================================================================
pixel CShader::palette2(double d)
{
    U8 shade = (U8)(fabs(sin(d * 0.1)) * 255);
    pixel px = { shade, shade, shade, 255 };
    return px;
}
//=========================================================================================================



//=========================================================================================================
// SetScheme() - Declares which color scheme to use when "GetPixelColor()" is called
//=========================================================================================================
void CShader::SetScheme(int scheme_id)
{
    // Store the current scheme ID for posterity
    m_scheme = scheme_id;

    switch (scheme_id)
    {
        case CS_OBW_LINEAR:
            Init_OBW_LINEAR();
            break;
    }
}
//=========================================================================================================

//=========================================================================================================
// SetFixedHue() - Sets the fixed-hue value and updates the indicator
//=========================================================================================================
void CShader::SetFixedHue(double hue)
{
    // Save the hue for future use
    m_fixed_hue = hue;

    // And update the on-screen indicator
    fixed_hue_indicator.SetHue(hue);
}
//=========================================================================================================



//=========================================================================================================
// Init_OBW_PALETTE() - Initializes the Orange/Blue/White palette
//=========================================================================================================
void CShader::Init_OBW_LINEAR()
{
    pixel* next;

    // White to blue to black
    next = ColorSpan(.6, .5, 0, 1, 120, m_palette);

    // Black to orange to white
    next = ColorSpan(.1, 1, 1, 0, 100, next);

    // White to blue to black
    next = ColorSpan(.6, .9, 0, 1, 100, next);

    // And keep track of how many pixels are in this palette
    m_palette_size = (U32)(next - m_palette);
}
//=========================================================================================================


//=========================================================================================================
// DumpPalette() - Dumps the palette to a bitmap file for debugging
//=========================================================================================================
void CShader::DumpPalette()
{
    pixel* image = new pixel[m_palette_size * 30];

    pixel* p = image;

    for (int y=0; y<30; ++y)
    {
        memcpy(p, m_palette, m_palette_size * sizeof pixel);
        p += m_palette_size;
    }

    WriteBmp(L"palette.bmp", image, m_palette_size, 30);
    
    delete[] image;
}
//=========================================================================================================


//=========================================================================================================
// InitSchemeNames() - Inserts color scheme names into the ComboBox in the correct order
//=========================================================================================================
void CShader::InitSchemeNames(CComboBox* pCB)
{
    pCB->AddString(L" Default (Earthtones)");
    pCB->AddString(L" Fixed Hue");
    pCB->AddString(L" Blue / Orange / White Linear");
    pCB->AddString(L" Monochrome");
    pCB->AddString(L" Blue / Orange / White Gradient");
    pCB->SetCurSel(0);
}
//=========================================================================================================


//=========================================================================================================
// GetRawColor() - Returns a pixel based upon the color scheme and escape value passed in
//=========================================================================================================
pixel CShader::GetRawColor(escape& e)
{
    pixel result;

    // Apply the appropriate color scheme
    switch(m_scheme)
    {
        
        case CS_DEFAULT:
        case CS_FIXED_HUE:
            result = GetRawColor0(e);
            break;

        case CS_OBW_LINEAR:
            result = GetRawColor1(e);
            break;

        case CS_MONOCHROME:
            result = GetRawColor2(e);
            break;

        case CS_OBW_GRADIENT:
            result = GetRawColor3(e);
            break;
    }

    // Perform Color Inversions
    if (invert_r) result.r = 255 - result.r;
    if (invert_g) result.g = 255 - result.g;
    if (invert_b) result.b = 255 - result.b;

    // Convert to greyscale if that option is selected
    if (greyscale)
    {
        U8 shade = (U8)(result.r * .299 + result.g * .587 + result.b * .114);
        result.r = result.g = result.b = shade;
    }


    return result;
}
//=========================================================================================================



//=========================================================================================================
// GetColor - Returns the RGB pixel that corresponds to the specified value and current color_scheme
//=========================================================================================================
pixel CShader::GetColor(frac_value& v)
{
    pixel result;

    // If we aren't oversampled, return the ordinary color
    if (ps.oversample == 0) return GetRawColor(v.e[0]);

    // Initialize accumulators for the red, green, and blue channels
    U32 r=0, g=0, b=0;

    for (U32 i = 0; i < ps.oversample; ++i)
    {
        // Get the raw color for this sub-sample
        result = GetRawColor(v.e[i]);

        // Accumulate the sum of each color channel
        r += result.r;
        g += result.g;
        b += result.b;
    }

    // Find the average color-channel values of all of the subpixels
    result.r = r / ps.oversample;
    result.g = g / ps.oversample;
    result.b = b / ps.oversample;

    // And that's the final color of our pixel
    return result;
}
//=========================================================================================================


//=========================================================================================================
// GetRawColor0() - Translates an escape-time to a pixel-shade
//=========================================================================================================
pixel CShader::GetRawColor0(escape& e)
{
    if (e.iter == 0) return black;

    double smoothed = log(log(e.distance) * 0.5) * ONE_OVER_LOG2;
    double d = e.iter + 10.0 - smoothed;

    d += 50;
    d = log(d);
    d *= 100;
    double p = floor(d);
    return interpolate(palette0(p), palette0(p+1), d - p);
}
//=========================================================================================================


//=========================================================================================================
// GetRawColor1() - Translates an escape-time to a pixel-shade
//=========================================================================================================
pixel CShader::GetRawColor1(escape& e)
{
    if (e.iter == 0) return black;


    double smoothed = log(log(e.distance) * 0.5) * ONE_OVER_LOG2;
    double d = e.iter + 10.0 - smoothed;

    double zero_to_one = fabs(sin(d * .001));
    return m_palette[(U32)(zero_to_one * m_palette_size)];
}
//=========================================================================================================


//=========================================================================================================
// GetRawColor2() - Translates an escape-time to a pixel-shade
//=========================================================================================================
pixel CShader::GetRawColor2(escape& e)
{
    if (e.iter == 0) return black;

    double smoothed = log(log(e.distance) * 0.5) * ONE_OVER_LOG2;
    double d = e.iter + 10.0 - smoothed;

    d += 50;
    d = log(d);
    d *= 100;
    double p = floor(d);
    return interpolate(palette2(p), palette2(p + 1), d - p);
}
//=========================================================================================================


//=========================================================================================================
// GetRawColor3() - Translates an escape-time to a pixel-shade
//=========================================================================================================
pixel CShader::GetRawColor3(escape& e)
{
    if (e.iter == 0) return black;

    double smoothed = log(log(e.distance * ONE_OVER_LOG2)) * ONE_OVER_LOG2;
    double d = sqrt(e.iter + 1 - smoothed);

    int colorI = (int) (d * 256) % sizeofa(m_obw_gradient);
    return m_obw_gradient[colorI];
}
//=========================================================================================================
