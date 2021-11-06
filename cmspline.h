#pragma once
#include <vector>
using std::vector;

//=========================================================================================================
// class CubicMonoSpline - Monotonic Cubic Spline
//=========================================================================================================
class CubicMonoSpline
{
public:

    // Call this to create the interpolation coefficients
    void    CreateInterpolant(double* xs, double* ys, unsigned length);

    // Call this find the interpolated Y value that corresponds to the given X value
    double  Interpolate(double x);

protected:

    vector<double>  m_xs;
    vector<double>  m_ys;
    vector<double>  m_c1s;
    vector<double>  m_c2s;
    vector<double>  m_c3s;
};
//=========================================================================================================

