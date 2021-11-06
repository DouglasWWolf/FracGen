#include "stdafx.h"
#include "cmspline.h"

//=========================================================================================================
// CreateInterpolant() - Creates the interpolating coefficients
//=========================================================================================================
void CubicMonoSpline::CreateInterpolant(double* xs, double* ys, unsigned length)
{
    unsigned i;

    vector<double>  dxs;  // Delta X's
    vector<double>  dys;  // Delta Y's
    vector<double>  ms;   // Slopes;

    // For efficiency, pre-reserve enough room in our arrays
    dxs.reserve(length);
    dys.reserve(length);
    ms.reserve(length);
    m_xs.reserve(length);
    m_ys.reserve(length);
    m_c1s.clear();
    m_c2s.clear();
    m_c3s.clear();

    // Get a copy of our inputs
    m_xs.resize(length, 0);
    m_ys.resize(length, 0);
    memcpy(m_xs.data(), xs, length * sizeof xs[0]);
    memcpy(m_ys.data(), ys, length * sizeof ys[0]);

    // Get consecutive differences and slopes
    for (i = 0; i < length - 1; i++)
    {
        double dx = xs[i + 1] - xs[i];
        double dy = ys[i + 1] - ys[i];
        dxs.push_back(dx);
        dys.push_back(dy);
        ms.push_back(dy / dx);
    }

    // Get degree-1 coefficients
    m_c1s.push_back(ms[0]);
    for (i = 0; i < (unsigned) dxs.size() - 1; i++)
    {
        double m = ms[i], mNext = ms[i + 1];
        if (m*mNext <= 0)
        {
            m_c1s.push_back(0);
        }
        else
        {
            double dx_ = dxs[i], dxNext = dxs[i + 1], common = dx_ + dxNext;
            m_c1s.push_back(3 * common / ((common + dxNext) / m + (common + dx_) / mNext));
        }
    }
    m_c1s.push_back(ms[ms.size() - 1]);

    // Get degree-2 and degree-3 coefficients
    for (i = 0; i < m_c1s.size() - 1; i++)
    {
        double c1 = m_c1s[i];
        double m_ = ms[i];
        double invDx = 1 / dxs[i];
        double common_ = c1 + m_c1s[i + 1] - m_ - m_;
        m_c2s.push_back((m_ - c1 - common_) * invDx);
        m_c3s.push_back(common_ * invDx * invDx);
    }
}
//=========================================================================================================


//=========================================================================================================
// Interpolate() - Interpolates a Y value from the specified X value
//=========================================================================================================
double CubicMonoSpline::Interpolate(double x)
{
    // The rightmost point in the dataset should give an exact result
    unsigned i = (unsigned) m_xs.size() - 1;

    // If we were passed in the last 'X' value of our definition, return the corresponding 'Y'
    if (x == m_xs[i])  return m_ys[i];

    // Search for the interval x is in, returning the corresponding y if x is one of the original xs
    unsigned  low = 0, mid, high = (unsigned)m_c3s.size() - 1;
    while (low <= high)
    {
        mid = (low + high) / 2;
        double xHere = m_xs[mid];
        if (xHere < x)
            low = mid + 1;
        else if (xHere > x)
            high = mid - 1;
        else
            return m_ys[mid];
    }
    i = max(0, high);

    // Interpolate
    double diff = x - m_xs[i];
    double diffSq = diff*diff;
    return m_ys[i] + m_c1s[i] * diff + m_c2s[i] * diffSq + m_c3s[i] * diff*diffSq;
};
//=========================================================================================================
