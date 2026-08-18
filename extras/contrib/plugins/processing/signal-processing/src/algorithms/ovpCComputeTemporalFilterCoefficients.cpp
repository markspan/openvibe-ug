#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCComputeTemporalFilterCoefficients.h"

#include <limits>
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

// Add 2complexes
void CComputeTemporalFilterCoefficients::addComplex(cmplex* a, cmplex* b, cmplex* c)
{
    c->real = b->real + a->real;
    c->imag = b->imag + a->imag;
}

// Substract 2 complex
void CComputeTemporalFilterCoefficients::subComplex(cmplex* a, cmplex* b, cmplex* c)
{
    c->real = b->real - a->real;
    c->imag = b->imag - a->imag;
}

// Multiply 2 complexes
void CComputeTemporalFilterCoefficients::mulComplex(cmplex* a, cmplex* b, cmplex* c)
{
    const double y = b->real * a->real - b->imag * a->imag;
    c->imag        = b->real * a->imag + b->imag * a->real;
    c->real        = y;
}

// Divide 2 complex numbers
void CComputeTemporalFilterCoefficients::divComplex(cmplex* a, cmplex* b, cmplex* c) const
{
    const double y = a->real * a->real + a->imag * a->imag;
    const double p = b->real * a->real + b->imag * a->imag;
    const double q = b->imag * a->real - b->real * a->imag;

    if (y < 1.0)
    {
        const double w = MAXNUM * y;
        if ((fabs(p) > w) || (fabs(q) > w) || (y == 0.0))
        {
            c->real = MAXNUM;
            c->imag = MAXNUM;
            std::cout << "divCOMPLEX: OVERFLOW" << std::endl;
            return;
        }
    }
    c->real = p / y;
    c->imag = q / y;
}

// Compute abs of a complex
double CComputeTemporalFilterCoefficients::absComplex(cmplex* z) const
{
    int ex, ey;

    const double re = fabs(z->real);
    const double im = fabs(z->imag);

    if (re == 0.0) { return (im); }
    if (im == 0.0) { return (re); }

    // Get the exponents of the numbers
    frexp(re, &ex);
    frexp(im, &ey);

    // Check if one number is tiny compared to the other
    int e = ex - ey;
    if (e > PREC) { return (re); }
    if (e < -PREC) { return (im); }

    // Find approximate exponent e of the geometric mean.
    e = (ex + ey) >> 1;

    // Rescale so mean is about 1
    const double x = ldexp(re, -e);
    double y       = ldexp(im, -e);

    // Hypotenuse of the right triangle
    double b = sqrt(x * x + y * y);

    // Compute the exponent of the answer.
    y  = frexp(b, &ey);
    ey = e + ey;

    // Check it for overflow and underflow.
    if (ey > MAXEXP)
    {
        std::cout << "absCOMPLEX: OVERFLOW" << std::endl;
        return (std::numeric_limits<double>::infinity());
    }
    if (ey < MINEXP) { return (0.0); }

    // Undo the scaling
    b = ldexp(b, e);
    return (b);
}

// Compute sqrt of a complex number
void CComputeTemporalFilterCoefficients::sqrtComplex(cmplex* z, cmplex* w) const
{
    cmplex q, s;
    double r, t;

    const double x = z->real;
    const double y = z->imag;

    if (y == 0.0)
    {
        if (x < 0.0)
        {
            w->real = 0.0;
            w->imag = sqrt(-x);
            return;
        }
        w->real = sqrt(x);
        w->imag = 0.0;
        return;
    }

    if (x == 0.0)
    {
        r = fabs(y);
        r = sqrt(0.5 * r);
        if (y > 0) { w->real = r; }
        else { w->real = -r; }
        w->imag = r;
        return;
    }

    // Approximate  sqrt(x^2+y^2) - x  =  y^2/2x - y^4/24x^3 + ... .
    // The relative error in the first term is approximately y^2/12x^2 .
    if ((fabs(y) < 2.e-4 * fabs(x)) && (x > 0)) { t = 0.25 * y * (y / x); }
    else
    {
        r = absComplex(z);
        t = 0.5 * (r - x);
    }
    r      = sqrt(t);
    q.imag = r;
    q.real = y / (2.0 * r);
    // Heron iteration in complex arithmetic
    divComplex(&q, z, &s);
    addComplex(&q, &s, w);
    w->real *= 0.5;
    w->imag *= 0.5;
}

// compute s plane poles and zeros
void CComputeTemporalFilterCoefficients::findSPlanePolesAndZeros()
{
    m_nPoles = (m_filterOrder + 1) / 2;
    m_nZeros = 0;
    m_zs     = itpp::zeros(m_arraySize);

    double dm;
    size_t ii = 0;
    double db;

    if (m_filterMethod == EFilterMethod::Butterworth)//poles equally spaced around the unit circle
    {
        if (m_filterOrder & 1) { dm = 0.0; }
        else { dm = itpp::pi / (2.0 * double(m_filterOrder)); }

        for (size_t i = 0; i < m_nPoles; ++i)// poles
        {
            const size_t lr = i + i;
            m_zs[lr]        = -cos(dm);
            m_zs[lr + 1]    = sin(dm);
            dm += itpp::pi / double(m_filterOrder);
        }

        if (m_filterType == EFilterType::HighPass || m_filterType == EFilterType::BandStop) // high pass or band reject
        {
            // map s => 1/s
            for (size_t j = 0; j < m_nPoles; ++j)
            {
                const size_t ir = j + j;
                ii              = ir + 1;
                db              = m_zs[ir] * m_zs[ir] + m_zs[ii] * m_zs[ii];
                m_zs[ir]        = m_zs[ir] / db;
                m_zs[ii]        = m_zs[ii] / db;
            }

            // The zeros at infinity map to the origin.
            m_nZeros = m_nPoles;
            if (m_filterType == EFilterType::BandStop) { m_nZeros += m_filterOrder / 2; }
            for (size_t j = 0; j < m_nZeros; ++j)
            {
                const size_t ir = ii + 1;
                ii              = ir + 1;
                m_zs[ir]        = 0.0;
                m_zs[ii]        = 0.0;
            }
        }
    }

    if (m_filterMethod == EFilterMethod::Chebyshev)
    {
        //For Chebyshev, find radii of two Butterworth circles (See Gold & Rader, page 60)
        m_rho           = (m_phi - 1.0) * (m_phi + 1);  // m_rho = m_eps^2 = {sqrt(1+m_eps^2)}^2 - 1
        m_eps           = sqrt(m_rho);                  // sqrt( 1 + 1/m_eps^2 ) + 1/m_eps  = {sqrt(1 + m_eps^2)  +  1} / m_eps
        m_phi           = (m_phi + 1.0) / m_eps;
        m_phi           = pow(m_phi, double(1.0) / m_filterOrder);  // raise to the 1/n power
        db              = 0.5 * (m_phi + 1.0 / m_phi);      // y coordinates are on this circle
        const double da = 0.5 * (m_phi - 1.0 / m_phi);      // x coordinates are on this circle
        if (m_filterOrder & 1) { dm = 0.0; }
        else { dm = itpp::pi / (2.0 * double(m_filterOrder)); }

        for (size_t i = 0; i < m_nPoles; ++i)// poles
        {
            const size_t lr = i + i;
            m_zs[lr]        = -da * cos(dm);
            m_zs[lr + 1]    = db * sin(dm);
            dm += itpp::pi / double(m_filterOrder);
        }

        if (m_filterType == EFilterType::HighPass || m_filterType == EFilterType::BandStop)// high pass or band reject
        {
            // map s => 1/s
            for (size_t j = 0; j < m_nPoles; ++j)
            {
                const size_t ir = j + j;
                ii              = ir + 1;
                db              = m_zs[ir] * m_zs[ir] + m_zs[ii] * m_zs[ii];
                m_zs[ir]        = m_zs[ir] / db;
                m_zs[ii]        = m_zs[ii] / db;
            }
            // The zeros at infinity map to the origin.
            m_nZeros = m_nPoles;
            if (m_filterType == EFilterType::BandStop) { m_nZeros += m_filterOrder / 2; }

            for (size_t j = 0; j < m_nZeros; ++j)
            {
                const size_t ir = ii + 1;
                ii              = ir + 1;
                m_zs[ir]        = 0.0;
                m_zs[ii]        = 0.0;
            }
        }
    }
}

//convert s plane poles and zeros to the z plane.
void CComputeTemporalFilterCoefficients::convertSPlanePolesAndZerosToZPlane()
{
    // Vars
    cmplex r, cnum, cden, cwc, ca, cb, b4Ac;
    cmplex cone = { 1.0, 0.0 };
    cmplex* z   = new cmplex[m_arraySize];
    double* pp  = new double[m_arraySize];
    double* y   = new double[m_arraySize];
    double* aa  = new double[m_arraySize];

    double c  = 0.0, a = 0.0, b = 0.0, pn = 0.0, an = 0.0, gam = 0.0, ai = 0.0, cng = 0.0, gain = 0.0;
    size_t nc = 0, jt  = 0, ii  = 0, ir   = 0, jj   = 0, jh    = 0, jl   = 0, mh    = 0;

    c = m_tanAng;

    for (size_t i = 0; i < m_arraySize; ++i)
    {
        z[i].real = 0.0;
        z[i].imag = 0.0;
    }

    nc = m_nPoles;
    jt = -1;
    ii = -1;

    for (size_t icnt = 0; icnt < 2; ++icnt)
    {
        do
        {
            ir = ii + 1;
            ii = ir + 1;

            r.real = m_zs[ir];
            r.imag = m_zs[ii];

            if (m_filterType == EFilterType::LowPass || m_filterType == EFilterType::HighPass)
            {
                // Substitute  s - r  =  s/wc - r = (1/wc)(z-1)/(z+1) - r
                //
                //     1  1 - r wc (       1 + r wc )
                // =  --- -------- ( z  -  -------- )
                //    z+1    wc    (       1 - r wc )
                //
                // giving the root in the z plane.
                cnum.real = 1 + c * r.real;
                cnum.imag = c * r.imag;
                cden.real = 1 - c * r.real;
                cden.imag = -c * r.imag;
                jt += 1;
                divComplex(&cden, &cnum, &z[jt]);

                if (r.imag != 0.0)
                {
                    // fill in complex conjugate root
                    jt += 1;
                    z[jt].real = z[jt - 1].real;
                    z[jt].imag = -z[jt - 1].imag;
                }
            }

            if (m_filterType == EFilterType::BandPass || m_filterType == EFilterType::BandStop)
            {
                // Substitute  s - r  =>  s/wc - r
                //
                //     z^2 - 2 z cgam + 1
                // =>  ------------------  -  r
                //         (z^2 + 1) wc
                //
                //         1
                // =  ------------  [ (1 - r wc) z^2  - 2 cgam z  +  1 + r wc ]
                //    (z^2 + 1) wc
                //
                // and solve for the roots in  the z plane.

                if (m_filterMethod == EFilterMethod::Chebyshev) { cwc.real = m_cbp; }
                else { cwc.real = m_tanAng; }
                cwc.imag = 0.0;

                // r * wc //
                mulComplex(&r, &cwc, &cnum);
                // a = 1 - r wc //
                subComplex(&cnum, &cone, &ca);
                // 1 - (r wc)^2 //
                mulComplex(&cnum, &cnum, &b4Ac);
                subComplex(&b4Ac, &cone, &b4Ac);
                // 4ac //
                b4Ac.real *= 4.0;
                b4Ac.imag *= 4.0;
                // b //
                cb.real = -2.0 * m_cosGam;
                cb.imag = 0.0;
                // b^2 //
                mulComplex(&cb, &cb, &cnum);
                // b^2 - 4ac//
                subComplex(&b4Ac, &cnum, &b4Ac);
                // sqrt() //
                sqrtComplex(&b4Ac, &b4Ac);
                // -b //
                cb.real = -cb.real;
                cb.imag = -cb.imag;
                // 2a //
                ca.real *= 2.0;
                ca.imag *= 2.0;
                // -b +sqrt(b^2-4ac) //
                addComplex(&b4Ac, &cb, &cnum);
                // ... /2a //
                divComplex(&ca, &cnum, &cnum);
                jt += 1;

                z[jt].real = cnum.real;
                z[jt].imag = cnum.imag;

                if (cnum.imag != 0.0)
                {
                    jt += 1;
                    z[jt].real = cnum.real;
                    z[jt].imag = -cnum.imag;
                }
                if ((r.imag != 0.0) || cnum.imag == 0.0)
                {
                    // -b - sqrt( b^2 - 4ac) //
                    subComplex(&b4Ac, &cb, &cnum);
                    // ... /2a //
                    divComplex(&ca, &cnum, &cnum);

                    jt += 1;
                    z[jt].real = cnum.real;
                    z[jt].imag = cnum.imag;

                    if (cnum.imag != 0.0)
                    {
                        jt += 1;
                        z[jt].real = cnum.real;
                        z[jt].imag = -cnum.imag;
                    }
                }
            }
        } while (--nc > 0);

        if (icnt == 0)
        {
            m_zOrd = jt + 1;
            if (m_nZeros <= 0) { icnt = 2; }
        }
        nc = m_nZeros;
    }

    // Generate the remaining zeros
    while (2 * m_zOrd - 1 > jt)
    {
        if (m_filterType != EFilterType::HighPass)
        {
            jt += 1;
            z[jt].real = -1.0;
            z[jt].imag = 0.0;
        }

        if (m_filterType == EFilterType::BandPass || m_filterType == EFilterType::HighPass)
        {
            jt += 1;
            z[jt].real = 1.0;
            z[jt].imag = 0.0;
        }
    }

    // Expand the poles and zeros into numerator and denominator polynomials
    for (size_t j = 0; j < m_arraySize; ++j) { aa[j] = 0.0; }
    for (size_t icnt = 0; icnt < 2; ++icnt)
    {
        for (size_t j = 0; j < m_arraySize; ++j)
        {
            pp[j] = 0.0;
            y[j]  = 0.0;
        }
        pp[0] = 1.0;

        for (size_t j = 0; j < m_zOrd; ++j)
        {
            jj = j;
            if (icnt) { jj += m_zOrd; }

            a = z[jj].real;
            b = z[jj].imag;
            for (size_t k = 0; k <= j; ++k)
            {
                jh         = j - k;
                pp[jh + 1] = pp[jh + 1] - a * pp[jh] + b * y[jh];
                y[jh + 1]  = y[jh + 1] - b * pp[jh] - a * y[jh];
            }
        }

        if (icnt == 0) { for (size_t j = 0; j <= m_zOrd; ++j) { aa[j] = pp[j]; } }
    }

    // Scale factors of the pole and zero polynomials
    if (m_filterType == EFilterType::HighPass) { a = -1.0; }
    else { a = 1.0; }

    if (m_filterType == EFilterType::HighPass || m_filterType == EFilterType::LowPass || m_filterType ==
        EFilterType::BandStop)
    {
        pn = 1.0;
        an = 1.0;
        for (size_t j = 1; j <= m_zOrd; ++j)
        {
            pn = a * pn + pp[j];
            an = a * an + aa[j];
        }
    }

    if (m_filterType == EFilterType::BandPass)
    {
        gam = itpp::pi / 2.0 - asin(m_cosGam);  // = acos( cgam ) //
        mh  = m_zOrd / 2;
        pn  = pp[mh];
        an  = aa[mh];
        ai  = 0.0;
        if (mh > ((m_zOrd / 4) * 2))
        {
            ai = 1.0;
            pn = 0.0;
            an = 0.0;
        }
        for (size_t j = 1; j <= mh; ++j)
        {
            a   = gam * j - ai * itpp::pi / 2.0;
            cng = cos(a);
            jh  = mh + j;
            jl  = mh - j;
            pn  = pn + cng * (pp[jh] + (1.0 - 2.0 * ai) * pp[jl]);
            an  = an + cng * (aa[jh] + (1.0 - 2.0 * ai) * aa[jl]);
        }
    }

    gain = an / (pn * m_scale);
    if (pn == 0.0) { gain = 1.0; }

    for (size_t j = 0; j <= m_zOrd; ++j) { pp[j] = gain * pp[j]; }

    for (size_t j = 0; j <= m_zOrd; ++j)
    {
        m_coefFilterDen[j] = pp[j];
        m_coefFilterNum[j] = aa[j];
    }

    delete [] z;
    delete [] pp;
    delete [] y;
    delete [] aa;
}

bool CComputeTemporalFilterCoefficients::initialize()
{
    ip_sampling.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_Sampling));
    ip_filterMethod.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterMethod));
    ip_filterType.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterType));
    ip_filterOrder.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterOrder));
    ip_lowCutFrequency.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency));
    ip_highCutFrequency.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency));
    ip_bandPassRipple.initialize(getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_BandPassRipple));

    op_matrix.initialize(getOutputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_OutputParameterId_Matrix));

    return true;
}

bool CComputeTemporalFilterCoefficients::uninitialize()
{
    op_matrix.uninitialize();

    ip_bandPassRipple.uninitialize();
    ip_highCutFrequency.uninitialize();
    ip_lowCutFrequency.uninitialize();
    ip_filterOrder.uninitialize();
    ip_filterType.uninitialize();
    ip_filterMethod.uninitialize();
    ip_sampling.uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CComputeTemporalFilterCoefficients::process()
{
    if (isInputTriggerActive(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_Initialize))
    {
        m_sampling         = size_t(ip_sampling);
        m_filterMethod     = EFilterMethod(uint64_t(ip_filterMethod));
        m_filterType       = EFilterType(uint64_t(ip_filterType));
        m_filterOrder      = size_t(ip_filterOrder);
        m_lowPassBandEdge  = ip_lowCutFrequency;
        m_highPassBandEdge = ip_highCutFrequency;
        m_passBandRipple   = ip_bandPassRipple;

        m_arraySize = 4 * m_filterOrder; // Maximum size of array involved in computation
    }

    if (isInputTriggerActive(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_ComputeCoefs))
    {
        if (m_filterMethod == EFilterMethod::Butterworth || m_filterMethod == EFilterMethod::Chebyshev)
        {
            if (m_filterType == EFilterType::LowPass || m_filterType == EFilterType::HighPass)
            {
                m_dimSize       = m_filterOrder + 1;
                m_coefFilterDen = itpp::zeros(m_dimSize);
                m_coefFilterNum = itpp::zeros(m_dimSize);
            }
            else
            {
                m_dimSize       = 2 * m_filterOrder + 1;
                m_coefFilterDen = itpp::zeros(m_dimSize);
                m_coefFilterNum = itpp::zeros(m_dimSize);
            }

            if (m_filterMethod == EFilterMethod::Chebyshev)
            {
                // For Chebyshev filter, ripples go from 1.0 to 1/sqrt(1+m_eps^2)
                m_phi = exp(0.5 * m_passBandRipple / (10.0 / log(10.0)));

                if ((m_filterOrder & 1) == 0) { m_scale = m_phi; }
                else { m_scale = 1.0; }
            }

            m_nyquist = m_sampling / 2;

            //locate edges
            if (m_filterType == EFilterType::LowPass) { m_lowPassBandEdge = 0.0; }

            //local variables
            double bandWidth, highFrequencyEdge;

            if (m_filterType == EFilterType::HighPass)
            {
                bandWidth         = m_highPassBandEdge;
                highFrequencyEdge = double(m_nyquist);
            }
            else
            {
                bandWidth         = m_highPassBandEdge - m_lowPassBandEdge;
                highFrequencyEdge = m_highPassBandEdge;
            }

            //convert to Frequency correspondence for bilinear transformation
            // Wanalog = tan( 2 pi Fdigital T / 2 )
            // where T = 1/fs
            const double ang    = double(bandWidth) * itpp::pi / double(m_sampling);
            const double cosAng = cos(ang);
            m_tanAng            = sin(ang) / cosAng; // Wanalog

            // Transformation from low-pass to band-pass critical frequencies
            // Center frequency
            //                      cos( 1/2 (Whigh+Wlow) T )
            //  cos( Wcenter T ) =  -------------------------
            //                      cos( 1/2 (Whigh-Wlow) T )
            //
            // Band edges
            //            cos( Wcenter T) - cos( Wdigital T )
            //  Wanalog = -----------------------------------
            //                     sin( Wdigital T )

            highFrequencyEdge = itpp::pi * (highFrequencyEdge + m_lowPassBandEdge) / double(m_sampling);
            m_cosGam          = cos(highFrequencyEdge) / cosAng;
            highFrequencyEdge = 2.0 * itpp::pi * m_highPassBandEdge / double(m_sampling);
            m_cbp             = (m_cosGam - cos(highFrequencyEdge)) / sin(highFrequencyEdge);

            if (m_filterMethod == EFilterMethod::Butterworth) { m_scale = 1.0; }

            findSPlanePolesAndZeros();
            convertSPlanePolesAndZerosToZPlane();
        }

        CMatrix* oMatrix = op_matrix;
        oMatrix->resize(m_dimSize, 2);              // Push m_coefFilterDen and m_coefFilterNum as two column vectors m_dimension rows

        double* filterCoefMatrix = oMatrix->getBuffer();
        for (size_t i = 0; i < m_dimSize; ++i)
        {
            filterCoefMatrix[i]             = m_coefFilterDen[i];
            filterCoefMatrix[m_dimSize + i] = m_coefFilterNum[i];
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
