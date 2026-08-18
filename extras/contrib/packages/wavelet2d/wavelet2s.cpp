//============================================================================
// Name        : 1D/2D Wavelet Transform
// Author      : Rafat Hussain
// Version     :
// Copyright   : GNU Lesser GPL License
// Description : Wavelet Library
//============================================================================
/*Copyright (C) 2011 Rafat Hussain

 * This program is free software; you can redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by the Free Software Foundation; version 2 or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
*/
#if defined(TARGET_HAS_ThirdPartyFFTW3)

#include "wavelet2s.h"
#include <iostream>
#include <complex>
#include <utility>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include "fftw3.h"

// extern "C" int _get_output_format( void ){ return 0; }

fftw_plan plan_forward_inp, plan_forward_filt, plan_backward;
static std::size_t fftTransientSize = 0;


void* per_ext2d(std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& temp2, const int a)
{
    const std::size_t rows  = signal.size();
    const std::size_t cols  = signal[0].size();
    const std::size_t cols2 = (cols % 2) != 0 ? cols + 1 : cols;
    std::vector<std::vector<double>> tempVec(rows, std::vector<double>(cols2 + 2 * a));

    for (std::size_t i = 0; i < rows; ++i)
    {
        std::vector<double> sig;
        for (std::size_t j = 0; j < cols; ++j)
        {
            double temp = signal[i][j];
            sig.push_back(temp);
        }
        per_ext(sig, a);
        for (std::size_t j = 0; j < sig.size(); ++j) { tempVec[i][j] = sig[j]; }
    }
    for (std::size_t j = 0; j < tempVec[0].size(); ++j)
    {
        std::vector<double> sig;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = tempVec[i][j];
            sig.push_back(temp);
        }
        per_ext(sig, a);
        for (std::size_t i = 0; i < sig.size(); ++i) { temp2[i][j] = sig[i]; }
    }


    return nullptr;
}

void* swt_2d(std::vector<std::vector<double>>& sig, int J, const std::string& nm, std::vector<double>& swtOutput)
{
    std::size_t m = sig.size(); // No. of rows
    std::size_t n = sig[0].size(); //No. of columns

    //std::vector<std::vector<double>> sig2 = sig;

    std::size_t nRows = m;
    std::size_t nCols = n;
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(nm, lp1, hp1, lp2, hp2);

    for (std::size_t it = 0; it < std::size_t(J); ++it)
    {
        int U = int(pow(2.0, double(it)));
        std::vector<double> lowPass, highPass;
        if (it > 0)
        {
            upsamp(lp1, U, lowPass);
            upsamp(hp1, U, highPass);
        }
        else
        {
            lowPass  = lp1;
            highPass = hp1;
        }
        int lf = lowPass.size();

        if ((sig.size() % 2) == 0) { nRows = sig.size(); }
        else { nRows = sig.size() + 1; }

        if ((sig[0].size() % 2) == 0) { nCols = sig[0].size(); }
        else { nCols = sig[0].size() + 1; }

        std::vector<std::vector<double>> signal(nRows + lf, std::vector<double>(nCols + lf));
        //    per_ext2d(sig,signal,lf/2); Edit per_ext if you want to use per_ext2d. Remove
        // the even indexing.

        per_ext2d(sig, signal, lf / 2);
        std::size_t lenX = signal.size();
        std::size_t lenY = signal[0].size();
        std::vector<std::vector<double>> sigL(nRows + lf, std::vector<double>(nCols));
        std::vector<std::vector<double>> sigH(nRows + lf, std::vector<double>(nCols));
        std::vector<std::vector<double>> cA(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cH(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cV(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cD(nRows, std::vector<double>(nCols));

        for (std::size_t i = 0; i < lenX; ++i)
        {
            std::vector<double> tempRow;
            for (std::size_t j = 0; j < lenY; ++j) { tempRow.push_back(signal[i][j]); }

            // ------------------Low Pass Branch--------------------------
            std::vector<double> oup;
            convfftm(tempRow, lowPass, oup);
            oup.erase(oup.begin(), oup.begin() + lf);
            oup.erase(oup.begin() + nCols, oup.end());

            // ------------------High Pass Branch--------------------------
            std::vector<double> oup2;
            convfftm(tempRow, highPass, oup2);
            oup2.erase(oup2.begin(), oup2.begin() + lf);
            oup2.erase(oup2.begin() + nCols, oup2.end());

            tempRow.clear();

            for (std::size_t j = 0; j < oup.size(); ++j)
            {
                sigL[i][j] = oup[j];
                sigH[i][j] = oup2[j];
            }
        }

        for (std::size_t j = 0; j < nCols; ++j)
        {
            std::vector<double> tempRow;
            for (std::size_t i = 0; i < lenX; ++i) { tempRow.push_back(sigL[i][j]); }

            // ------------------Low Pass Branch--------------------------


            std::vector<double> oup;
            convfftm(tempRow, lowPass, oup);
            oup.erase(oup.begin(), oup.begin() + lf);
            oup.erase(oup.begin() + nRows, oup.end());

            // ------------------High Pass Branch--------------------------

            std::vector<double> oup2;
            convfftm(tempRow, highPass, oup2);
            oup2.erase(oup2.begin(), oup2.begin() + lf);
            oup2.erase(oup2.begin() + nRows, oup2.end());

            tempRow.clear();


            for (std::size_t i = 0; i < oup.size(); ++i) { cA[i][j] = oup[i]; }
            for (std::size_t i = 0; i < oup2.size(); ++i) { cH[i][j] = oup2[i]; }
        }

        for (std::size_t j = 0; j < nCols; ++j)
        {
            std::vector<double> tempRow;
            for (std::size_t i = 0; i < lenX; ++i) { tempRow.push_back(sigH[i][j]); }

            // ------------------Low Pass Branch--------------------------
            std::vector<double> oup;
            convfftm(tempRow, lowPass, oup);
            oup.erase(oup.begin(), oup.begin() + lf);
            oup.erase(oup.begin() + nRows, oup.end());

            // ------------------High Pass Branch--------------------------
            std::vector<double> oup2;
            convfftm(tempRow, highPass, oup2);
            oup2.erase(oup2.begin(), oup2.begin() + lf);
            oup2.erase(oup2.begin() + nRows, oup2.end());

            tempRow.clear();


            for (std::size_t i = 0; i < oup.size(); ++i) { cV[i][j] = oup[i]; }
            for (std::size_t i = 0; i < oup2.size(); ++i) { cD[i][j] = oup2[i]; }
        }

        sig = cA;
        std::vector<double> tempSig2;

        if (it == std::size_t(J - 1)) { for (std::size_t i = 0; i < nRows; ++i) { for (std::size_t j = 0; j < nCols; ++j) { tempSig2.push_back(cA[i][j]); } } }
        for (std::size_t i = 0; i < nRows; ++i) { for (std::size_t j = nCols; j < nCols * 2; ++j) { tempSig2.push_back(cH[i][j - nCols]); } }
        for (std::size_t i = nRows; i < nRows * 2; ++i) { for (std::size_t j = 0; j < nCols; ++j) { tempSig2.push_back(cV[i - nRows][j]); } }
        for (std::size_t i = nRows; i < nRows * 2; ++i) { for (std::size_t j = nCols; j < nCols * 2; ++j) { tempSig2.push_back(cD[i - nRows][j - nCols]); } }

        swtOutput.insert(swtOutput.begin(), tempSig2.begin(), tempSig2.end());
    }

    return nullptr;
}


void* per_ext(std::vector<double>& sig, const int a)
{
    std::size_t len = sig.size();
    if ((len % 2) != 0)
    {
        sig.push_back(sig[len - 1]);
        len = sig.size();
    }

    for (std::size_t i = 0; i < std::size_t(a); ++i)
    {
        double temp1 = sig[2 * i];
        double temp2 = sig[len - 1];
        sig.insert(sig.begin(), temp2);
        sig.insert(sig.end(), temp1);
    }

    return nullptr;
}


void* iswt(std::vector<double>& swtop, int J, const std::string& nm, std::vector<double>& iswtOutput)
{
    std::size_t n = swtop.size() / (J + 1);

    std::vector<double> lpd, hpd, lpr, hpr;
    filtcoef(nm, lpd, hpd, lpr, hpr);

    std::vector<double> appxSig;

    std::vector<double> lowPass  = lpr;
    std::vector<double> highPass = hpr;
    int lf                       = lowPass.size();

    for (std::size_t iter = 0; iter < std::size_t(J); ++iter)
    {
        std::vector<double> detSig;
        if (iter == 0)
        {
            for (std::size_t i = 0; i < n; ++i)
            {
                double temp = swtop[i];
                appxSig.push_back(temp);
                double temp1 = swtop[(iter + 1) * n + i];
                detSig.push_back(temp1);
            }
        }
        else
        {
            for (std::size_t i = 0; i < n; ++i)
            {
                double temp1 = swtop[(iter + 1) * n + i];
                detSig.push_back(temp1);
            }
        }


        std::size_t value = std::size_t(pow(2.0, double(J - 1 - iter)));
        iswtOutput.assign(n, 0.0);

        for (std::size_t count = 0; count < value; ++count)
        {
            std::vector<double> appx1, det1;
            for (std::size_t index = count; index < n; index += value)
            {
                double temp = appxSig[index];
                appx1.push_back(temp);
                double temp1 = detSig[index];
                det1.push_back(temp1);
            }
            std::size_t len = appx1.size();

            // Shift = 0

            std::vector<double> appx2, det2;

            for (std::size_t i = 0; i < len; i += 2)
            {
                double temp = appx1[i];
                appx2.push_back(temp);
                double temp1 = det1[i];
                det2.push_back(temp1);
            }

            int U = 2; // Upsampling Factor

            std::vector<double> cL0, cH0;
            upsamp(appx2, U, cL0);
            upsamp(det2, U, cH0);
            per_ext(cL0, lf / 2);
            per_ext(cH0, lf / 2);

            std::vector<double> oup00L, oup00H, oup00;
            convfft(cL0, lowPass, oup00L);
            convfft(cH0, highPass, oup00H);

            oup00L.erase(oup00L.begin(), oup00L.begin() + lf - 1);
            oup00L.erase(oup00L.begin() + len, oup00L.end());
            oup00H.erase(oup00H.begin(), oup00H.begin() + lf - 1);
            oup00H.erase(oup00H.begin() + len, oup00H.end());

            vecsum(oup00L, oup00H, oup00);

            // Shift = 1

            std::vector<double> appx3, det3;

            for (std::size_t i = 1; i < len; i += 2)
            {
                double temp = appx1[i];
                appx3.push_back(temp);
                double temp1 = det1[i];
                det3.push_back(temp1);
            }


            std::vector<double> cL1, cH1;
            upsamp(appx3, U, cL1);
            upsamp(det3, U, cH1);
            per_ext(cL1, lf / 2);
            per_ext(cH1, lf / 2);

            std::vector<double> oup01L, oup01H, oup01;
            convfft(cL1, lowPass, oup01L);
            convfft(cH1, highPass, oup01H);

            oup01L.erase(oup01L.begin(), oup01L.begin() + lf - 1);
            oup01L.erase(oup01L.begin() + len, oup01L.end());
            oup01H.erase(oup01H.begin(), oup01H.begin() + lf - 1);
            oup01H.erase(oup01H.begin() + len, oup01H.end());

            vecsum(oup01L, oup01H, oup01);
            circshift(oup01, -1);

            //   Continue
            std::size_t index2 = 0;
            for (std::size_t index = count; index < n; index += value)
            {
                double temp          = oup00[index2] + oup01[index2];
                iswtOutput.at(index) = temp / 2;
                index2++;
            }
        }
        appxSig = iswtOutput;
    }
    return nullptr;
}

void* swt(std::vector<double>& signal1, const int J, const std::string& nm, std::vector<double>& swtOutput, int& length)
{
    std::vector<double> lpd, hpd, lpr, hpr;
    std::vector<double> sig = signal1;

    const std::size_t n = sig.size();
    length              = int(n);

    filtcoef(nm, lpd, hpd, lpr, hpr);

    for (std::size_t iter = 0; iter < std::size_t(J); ++iter)
    {
        std::vector<double> lowPass;
        std::vector<double> highPass;
        if (iter > 0)
        {
            const int m = int(pow(2.0, iter));
            upsamp(lpd, m, lowPass);
            upsamp(hpd, m, highPass);
        }
        else
        {
            lowPass  = lpd;
            highPass = hpd;
        }

        const std::size_t lenFilt = lowPass.size();
        per_ext(sig, int(lenFilt / 2));

        std::vector<double> cA;
        convfft(sig, lowPass, cA);
        std::vector<double> cD;
        convfft(sig, highPass, cD);
        // Resize cA and cD
        cA.erase(cA.begin(), cA.begin() + lenFilt);
        cA.erase(cA.begin() + n, cA.end());
        cD.erase(cD.begin(), cD.begin() + lenFilt);
        cD.erase(cD.begin() + n, cD.end());
        // Reset signal value;

        sig = cA;

        if (iter == std::size_t(J - 1))
        {
            swtOutput.insert(swtOutput.begin(), cD.begin(), cD.end());
            swtOutput.insert(swtOutput.begin(), cA.begin(), cA.end());
        }
        else { swtOutput.insert(swtOutput.begin(), cD.begin(), cD.end()); }
    }

    return nullptr;
}

void* DwtOutputDimSym(std::vector<std::size_t>& length, std::vector<std::size_t>& length2, const int J)
{
    const std::size_t sz = length.size();
    std::size_t rows     = length[sz - 2];
    std::size_t cols     = length[sz - 1];
    for (std::size_t i = 0; i < std::size_t(J); ++i)
    {
        rows = std::size_t(ceil(double(rows) / 2.0));
        cols = std::size_t(ceil(double(cols) / 2.0));
    }
    for (std::size_t i = 0; i < std::size_t(J + 1); ++i)
    {
        length2.push_back(rows);
        length2.push_back(cols);
        rows = rows * 2;
        cols = cols * 2;
    }
    return nullptr;
}

void* dwt_output_dim2(std::vector<std::size_t>& length, std::vector<std::size_t>& length2, const int j)
{
    std::size_t row = length[0];
    std::size_t col = length[1];

    for (std::size_t i = 0; i < std::size_t(j + 1); ++i)
    {
        length2.push_back(row);
        length2.push_back(col);
        row = row * 2;
        col = col * 2;
    }


    return nullptr;
}

void* dispDWT(std::vector<double>& output, std::vector<std::vector<double>>& dwtdisp, std::vector<std::size_t>& length, std::vector<std::size_t>& length2, const int J)
{
    std::size_t sum = 0;

    for (std::size_t it = 0; it < std::size_t(J); ++it)
    {
        const int dRows = int(length[2 * it] - length2[2 * it]);
        const int dCols = int(length[2 * it + 1] - length2[2 * it + 1]);

        const std::size_t nRows = length[2 * it];
        const std::size_t nCols = length[2 * it + 1];
        std::vector<std::vector<double>> oDwt(2 * nRows, std::vector<double>(2 * nCols));
        if (it == 0)
        {
            for (std::size_t i = 0; i < nRows; ++i) { for (std::size_t j = 0; j < nCols; ++j) { oDwt[i][j] = output[i * nCols + j]; } }

            for (std::size_t i = 0; i < nRows; ++i)
            {
                for (std::size_t j = nCols; j < nCols * 2; ++j) { oDwt[i][j] = output[nRows * nCols + i * nCols + (j - nCols)]; }
            }

            for (std::size_t i = nRows; i < nRows * 2; ++i)
            {
                for (std::size_t j = 0; j < nCols; ++j) { oDwt[i][j] = output[2 * nRows * nCols + (i - nRows) * nCols + j]; }
            }


            for (std::size_t i = nRows; i < nRows * 2; ++i)
            {
                for (std::size_t j = nCols; j < nCols * 2; ++j) { oDwt[i][j] = output[3 * nRows * nCols + (i - nRows) * nCols + (j - nCols)]; }
            }
        }
        else
        {
            for (std::size_t i = 0; i < nRows; ++i) { for (std::size_t j = nCols; j < nCols * 2; ++j) { oDwt[i][j] = output[sum + i * nCols + (j - nCols)]; } }

            for (std::size_t i = nRows; i < nRows * 2; ++i)
            {
                for (std::size_t j = 0; j < nCols; ++j) { oDwt[i][j] = output[sum + nRows * nCols + (i - nRows) * nCols + j]; }
            }


            for (std::size_t i = nRows; i < nRows * 2; ++i)
            {
                for (std::size_t j = nCols; j < nCols * 2; ++j) { oDwt[i][j] = output[sum + 2 * nRows * nCols + (i - nRows) * nCols + (j - nCols)]; }
            }
        }

        const std::size_t rowsX = length2[2 * it];
        const std::size_t colsX = length2[2 * it + 1];

        const int dCols2 = int(ceil(double(dCols - 1) / 2.0));
        const int dRows2 = int(ceil(double(dRows - 1) / 2.0));
        if (it == 0)
        {
            for (std::size_t i = 0; i < rowsX; ++i)
            {
                for (std::size_t j = 0; j < colsX; ++j)
                {
                    if (i + dRows - 1 < 0) { dwtdisp[i][j] = 0; }
                    else if (j + dCols - 1 < 0) { dwtdisp[i][j] = 0; }
                    else { dwtdisp[i][j] = oDwt[i + dRows - 1][j + dCols - 1]; }
                }
            }
        }
        for (std::size_t i = 0; i < rowsX; ++i)
        {
            for (std::size_t j = colsX; j < colsX * 2; ++j)
            {
                if (i + dRows2 < 0) { dwtdisp[i][j] = 0; }
                else if (int(j + 2 * (dCols - 1) + 1) > signed(oDwt[0].size()) - 1) { dwtdisp[i][j] = 0; }
                else { dwtdisp[i][j] = oDwt[i + dRows2][j + 2 * (dCols - 1) + 1]; }
            }
        }

        for (std::size_t i = rowsX; i < rowsX * 2; ++i)
        {
            for (std::size_t j = 0; j < colsX; ++j)
            {
                if (int(i + 2 * (dRows - 1) + 1) > signed(oDwt.size()) - 1) { dwtdisp[i][j] = 0; }
                else if (j + dCols2 < 0) { dwtdisp[i][j] = 0; }
                else { dwtdisp[i][j] = oDwt[i + 2 * (dRows - 1) + 1][j + dCols2]; }
            }
        }

        for (std::size_t i = rowsX; i < rowsX * 2; ++i)
        {
            for (std::size_t j = colsX; j < colsX * 2; ++j)
            {
                if (int(i + (dRows - 1) + 1 + dRows2) > signed(oDwt.size()) - 1) { dwtdisp[i][j] = 0; }
                else if (int(j + (dCols - 1) + 1 + dCols2) > signed(oDwt[0].size()) - 1) { dwtdisp[i][j] = 0; }
                else { dwtdisp[i][j] = oDwt[i + (dRows - 1) + 1 + dRows2][j + (dCols - 1) + 1 + dCols2]; }
            }
        }
        if (it == 0) { sum += 4 * nRows * nCols; }
        else { sum += 3 * nRows * nCols; }
    }

    return nullptr;
}

void symm_ext2d(std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& temp2, const int a)
{
    const std::size_t rows = signal.size();
    const std::size_t cols = signal[0].size();
    std::vector<std::vector<double>> tempVec(rows, std::vector<double>(cols + 2 * a));

    for (std::size_t i = 0; i < rows; ++i)
    {
        std::vector<double> sig;
        for (std::size_t j = 0; j < cols; ++j)
        {
            double temp = signal[i][j];
            sig.push_back(temp);
        }
        symm_ext(sig, a);
        for (std::size_t j = 0; j < sig.size(); ++j) { tempVec[i][j] = sig[j]; }
    }
    for (std::size_t j = 0; j < tempVec[0].size(); ++j)
    {
        std::vector<double> sig;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = tempVec[i][j];
            sig.push_back(temp);
        }
        symm_ext(sig, a);
        for (std::size_t i = 0; i < sig.size(); ++i) { temp2[i][j] = sig[i]; }
    }
}

void* circshift2d(std::vector<std::vector<double>>& signal, const int x, const int y)
{
    const std::size_t rows = signal.size();
    const std::size_t cols = signal[0].size();
    std::vector<std::vector<double>> tempVec(rows, std::vector<double>(cols));

    for (std::size_t i = 0; i < rows; ++i)
    {
        std::vector<double> sig;
        for (std::size_t j = 0; j < cols; ++j)
        {
            double temp = signal[i][j];
            sig.push_back(temp);
        }
        circshift(sig, x);
        for (std::size_t j = 0; j < cols; ++j) { tempVec[i][j] = sig[j]; }
    }

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> sig;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = tempVec[i][j];
            sig.push_back(temp);
        }
        circshift(sig, y);
        for (std::size_t i = 0; i < rows; ++i) { signal[i][j] = sig[i]; }
    }
    return nullptr;
}

void* idwt_2d_sym(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<std::vector<double>>& idwtOutput, std::vector<std::size_t>& length)
{
    int J            = int(flag[0]);
    std::size_t rows = length[0];
    std::size_t cols = length[1];

    std::size_t sumCoef = 0;
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(nm, lp1, hp1, lp2, hp2);
    std::size_t lf = lp1.size();
    std::vector<std::vector<double>> cLL(rows, std::vector<double>(cols));


    for (std::size_t it = 0; it < std::size_t(J); ++it)
    {
        std::size_t nRows = length[2 * it];
        std::size_t nCols = length[2 * it + 1];

        std::vector<std::vector<double>> cLH(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cHL(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cHH(nRows, std::vector<double>(nCols));

        for (std::size_t i = 0; i < nRows; ++i)
        {
            for (std::size_t j = 0; j < nCols; ++j)
            {
                if (it == 0)
                {
                    cLL[i][j] = dwtop[sumCoef + i * nCols + j];
                    cLH[i][j] = dwtop[sumCoef + nRows * nCols + i * nCols + j];
                    cHL[i][j] = dwtop[sumCoef + 2 * nRows * nCols + i * nCols + j];
                    cHH[i][j] = dwtop[sumCoef + 3 * nRows * nCols + i * nCols + j];
                }
                else
                {
                    cLH[i][j] = dwtop[sumCoef + i * nCols + j];
                    cHL[i][j] = dwtop[sumCoef + nRows * nCols + i * nCols + j];
                    cHH[i][j] = dwtop[sumCoef + 2 * nRows * nCols + i * nCols + j];
                }
            }
        }


        //      temp_A = cLL;
        //      idwt2_sym(nm,idwtOutput2, cA, cH,cV,cD);

        std::size_t lenX = cLH.size();
        std::size_t lenY = cLH[0].size();

        // Row Upsampling and Column Filtering at the first LP Stage
        std::vector<std::vector<double>> cL(2 * lenX - lf + 2, std::vector<double>(lenY));
        std::vector<std::vector<double>> cH(2 * lenX - lf + 2, std::vector<double>(lenY));

        if (it == 0)
        {
            for (std::size_t j = 0; j < lenY; ++j)
            {
                std::vector<double> sigLL, sigLH, oup;

                for (std::size_t i = 0; i < lenX; ++i)
                {
                    double temp1 = cLL[i][j];
                    double temp2 = cLH[i][j];
                    sigLL.push_back(temp1);
                    sigLH.push_back(temp2);
                }
                idwt1_sym_m(nm, oup, sigLL, sigLH);

                for (std::size_t i = 0; i < oup.size(); ++i) { cL[i][j] = oup[i]; }
            }
        }
        else
        {
            std::size_t rows1 = cLH.size();
            std::size_t cols1 = cLH[0].size();

            for (std::size_t j = 0; j < cols1; ++j)
            {
                std::vector<double> tempL1, tempL2, oup;
                for (std::size_t i = 0; i < rows1; ++i)
                {
                    double temp = cLL[i][j];
                    tempL1.push_back(temp);

                    double temp2 = cLH[i][j];
                    tempL2.push_back(temp2);
                }
                idwt1_sym_m(nm, oup, tempL1, tempL2);

                for (std::size_t i = 0; i < oup.size(); ++i) { cL[i][j] = oup[i]; }
            }
        }


        for (std::size_t j = 0; j < lenY; ++j)
        {
            std::vector<double> sigHL, sigHH, oup2;
            for (std::size_t i = 0; i < lenX; ++i)
            {
                double temp3 = cHL[i][j];
                double temp4 = cHH[i][j];
                sigHL.push_back(temp3);
                sigHH.push_back(temp4);
            }
            idwt1_sym_m(nm, oup2, sigHL, sigHH);

            for (std::size_t i = 0; i < oup2.size(); ++i) { cH[i][j] = oup2[i]; }
        }

        std::vector<std::vector<double>> signal(2 * lenX - lf + 2, std::vector<double>(2 * lenY - lf + 2));
        for (std::size_t i = 0; i < 2 * lenX - lf + 2; ++i)
        {
            std::vector<double> sigL, sigH, oup;
            for (std::size_t j = 0; j < lenY; ++j)
            {
                double temp5 = cL[i][j];
                double temp6 = cH[i][j];
                sigL.push_back(temp5);
                sigH.push_back(temp6);
            }
            idwt1_sym_m(nm, oup, sigL, sigH);

            for (std::size_t j = 0; j < oup.size(); ++j) { signal[i][j] = oup[j]; }
        }


        idwtOutput = signal;


        if (it == 0) { sumCoef += 4 * nRows * nCols; }
        else { sumCoef += 3 * nRows * nCols; }
        cLL = signal;
    }


    return nullptr;
}


void* dwt2_sym(const std::string& name, std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& cLL, std::vector<std::vector<double>>& cLH,
               std::vector<std::vector<double>>& cHL, std::vector<std::vector<double>>& cHH)
{
    //Analysis
    const std::size_t rows    = signal.size();
    std::size_t cols          = signal[0].size();
    const std::size_t colsLp1 = cLL[0].size();
    const std::size_t colsHp1 = cLL[0].size();
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(name, lp1, hp1, lp2, hp2);
    std::vector<std::vector<double>> lpDn1(rows, std::vector<double>(colsLp1));
    std::vector<std::vector<double>> hpDn1(rows, std::vector<double>(colsHp1));

    // Implementing row filtering and column downsampling in each branch.
    for (std::size_t i = 0; i < rows; ++i)
    {
        std::vector<double> tempRow, oupLp, oupHp;
        for (std::size_t j = 0; j < cols; ++j)
        {
            double temp = signal[i][j];
            tempRow.push_back(temp);
        }
        dwt1_sym_m(name, tempRow, oupLp, oupHp);

        for (std::size_t j = 0; j < oupLp.size(); ++j)
        {
            lpDn1[i][j] = oupLp[j];
            hpDn1[i][j] = oupHp[j];
        }
    }


    cols = colsLp1;
    // Implementing column filtering and row downsampling in Low Pass branch.

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> tempRow3, oupLp, oupHp;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = lpDn1[i][j];
            tempRow3.push_back(temp);
        }
        dwt1_sym_m(name, tempRow3, oupLp, oupHp);

        for (std::size_t i = 0; i < oupLp.size(); ++i)
        {
            cLL[i][j] = oupLp[i];
            cLH[i][j] = oupHp[i];
        }
    }


    // Implementing column filtering and row downsampling in High Pass branch.

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> tempRow5, oupLp, oupHp;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = hpDn1[i][j];
            tempRow5.push_back(temp);
        }
        dwt1_sym_m(name, tempRow5, oupLp, oupHp);

        for (std::size_t i = 0; i < oupLp.size(); ++i)
        {
            cHL[i][j] = oupLp[i];
            cHH[i][j] = oupHp[i];
        }
    }
    return nullptr;
}


void* dwt_2d_sym(std::vector<std::vector<double>>& origsig, const int J, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag, std::vector<std::size_t>& length)
{
    // flag will contain

    std::vector<std::vector<double>> sig          = origsig;
    std::size_t nRows                             = sig.size(); // No. of rows
    std::size_t nCols                             = sig[0].size(); //No. of columns
    std::vector<std::vector<double>> originalCopy = sig;
    const int maxIter                             = std::min(int(ceil(log(double(sig.size())) / log(2.0))), int(ceil(log(double(sig[0].size())) / log(2.0))));
    if (maxIter < J)
    {
        std::cout << J << " Iterations are not possible with signals of this dimension " << std::endl;
        exit(1);
    }
    std::vector<double> lp1, hp1, lp2, hp2;

    flag.push_back(double(J));


    length.insert(length.begin(), nCols);
    length.insert(length.begin(), nRows);


    // Flag Values
    /*
       double temp = (double) (sig2.size() - sig.size()); // Number of zeropad rows
       flag.push_back(temp);
       double temp2 = (double) (sig2[0].size() - sig[0].size());// Number of zpad cols
       flag.push_back(temp2);
       flag.push_back((double) J); // Number of Iterations
       */
    for (std::size_t iter = 0; iter < std::size_t(J); ++iter)
    {
        filtcoef(nm, lp1, hp1, lp2, hp2);
        const std::size_t lf = lp1.size();

        nRows = std::size_t(floor(double(nRows + lf - 1) / 2));
        nCols = std::size_t(floor(double(nCols + lf - 1) / 2));
        length.insert(length.begin(), nCols);
        length.insert(length.begin(), nRows);

        std::vector<std::vector<double>> cA(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cH(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cV(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cD(nRows, std::vector<double>(nCols));

        if (iter == 0) { dwt2_sym(nm, originalCopy, cA, cH, cV, cD); }
        else { dwt2_sym(nm, originalCopy, cA, cH, cV, cD); }
        std::vector<double> tempSig2;

        originalCopy = cA;
        if (iter == std::size_t(J - 1))
        {
            for (std::size_t i = 0; i < nRows; ++i)
            {
                for (std::size_t j = 0; j < nCols; ++j)
                {
                    double temp = cA[i][j];
                    tempSig2.push_back(temp);
                }
            }
        }
        for (std::size_t i = 0; i < nRows; ++i)
        {
            for (std::size_t j = nCols; j < nCols * 2; ++j)
            {
                double temp = cH[i][j - nCols];
                tempSig2.push_back(temp);
            }
        }

        for (std::size_t i = nRows; i < nRows * 2; ++i)
        {
            for (std::size_t j = 0; j < nCols; ++j)
            {
                double temp = cV[i - nRows][j];
                tempSig2.push_back(temp);
            }
        }

        for (std::size_t i = nRows; i < nRows * 2; ++i)
        {
            for (std::size_t j = nCols; j < nCols * 2; ++j)
            {
                double temp = cD[i - nRows][j - nCols];
                tempSig2.push_back(temp);
            }
        }

        dwtOutput.insert(dwtOutput.begin(), tempSig2.begin(), tempSig2.end());
    }
    /*
    ofstream dwt2out("dwt2out.dat");
    for (std::size_t i= 0; i < dwtOutput.size(); ++i){ dwt2out << dwtOutput[i] <<endl; }
    */

    return nullptr;
}


void* idwt1_sym(const std::string& wname, std::vector<double>& x, std::vector<double>& app, std::vector<double>& detail)
{
    std::vector<double> flag;
    std::vector<double> idwtOutput;
    std::vector<std::size_t> length;
    length[0]                 = app.size();
    length[1]                 = detail.size();
    std::vector<double> dwtop = app;
    dwtop.insert(dwtop.end(), detail.begin(), detail.end());
    flag.push_back(1);
    flag.push_back(0);
    idwt_sym(dwtop, flag, wname, idwtOutput, length);
    x = idwtOutput;

    return nullptr;
}

void* idwt1_sym_m(const std::string& wname, std::vector<double>& x, std::vector<double>& app, std::vector<double>& detail)
{
    const int U = 2; // Upsampling Factor
    std::vector<double> lpd1, hpd1, lpr1, hpr1;

    filtcoef(wname, lpd1, hpd1, lpr1, hpr1);
    const std::size_t lf = lpr1.size();

    // Operations in the Low Frequency branch of the Synthesis Filter Bank
    std::vector<double> xLp, cAUp;
    upsamp(app, U, cAUp);
    cAUp.pop_back();
    convfftm(cAUp, lpr1, xLp);


    // Operations in the High Frequency branch of the Synthesis Filter Bank
    std::vector<double> xHp, cDUp;
    upsamp(detail, U, cDUp);
    cDUp.pop_back();
    convfftm(cDUp, hpr1, xHp);


    vecsum(xLp, xHp, x);

    x.erase(x.begin(), x.begin() + lf - 2);
    x.erase(x.end() - (lf - 2), x.end());

    return nullptr;
}


void* symm_ext(std::vector<double>& sig, const int a)
{
    const std::size_t len = sig.size();
    for (std::size_t i = 0; i < std::size_t(a); ++i)
    {
        double temp1 = sig[i * 2];
        double temp2 = sig[len - 1];
        sig.insert(sig.begin(), temp1);
        sig.insert(sig.end(), temp2);
    }

    return nullptr;
}

void* idwt_sym(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<double>& idwtOutput, std::vector<std::size_t>& length)
{
    const int J = int(flag[1]);

    std::vector<double> app, detail;
    std::size_t appLen = length[0], detLen = length[1];

    const auto dwt = dwtop.begin();
    app.assign(dwt, dwtop.begin() + appLen);
    detail.assign(dwtop.begin() + appLen, dwtop.begin() + 2 * appLen);

    for (std::size_t i = 0; i < std::size_t(J); ++i)
    {
        const int U = 2; // Upsampling Factor
        std::vector<double> lpd1, hpd1, lpr1, hpr1;

        filtcoef(nm, lpd1, hpd1, lpr1, hpr1);
        const std::size_t lf = lpr1.size();

        // Operations in the Low Frequency branch of the Synthesis Filter Bank
        std::vector<double> xLp, cAUp;
        upsamp(app, U, cAUp);
        cAUp.pop_back();
        convfft(cAUp, lpr1, xLp);

        // Operations in the High Frequency branch of the Synthesis Filter Bank
        std::vector<double> xHp;
        std::vector<double> cDUp;
        upsamp(detail, U, cDUp);
        cDUp.pop_back();
        convfft(cDUp, hpr1, xHp);

        appLen += detLen;
        vecsum(xLp, xHp, idwtOutput);

        idwtOutput.erase(idwtOutput.begin(), idwtOutput.begin() + lf - 2);
        idwtOutput.erase(idwtOutput.end() - (lf - 2), idwtOutput.end());

        app.clear();
        detail.clear();
        if (i < std::size_t(J - 1))
        {
            detLen = length[i + 2];
            //        detail.assign(dwtop.begin()+app_len, dwtop.begin()+ det_len);

            for (std::size_t l = 0; l < detLen; ++l)
            {
                double temp = dwtop[appLen + l];
                detail.push_back(temp);
            }
        }
        app = idwtOutput;

        for (std::size_t iter1 = 0; iter1 < app.size() - detLen; iter1++) { app.pop_back(); }
    }


    // Remove ZeroPadding

    const int zerop = int(flag[0]);
    idwtOutput.erase(idwtOutput.end() - zerop, idwtOutput.end());
    return nullptr;
}

void* dwt1_sym(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD)
{
    std::vector<double> lp1, hp1, lp2, hp2;

    filtcoef(wname, lp1, hp1, lp2, hp2);
    const int d  = 2; // Downsampling Factor is 2
    const int lf = int(lp1.size());
    symm_ext(signal, lf - 1);

    std::vector<double> cAUndec;
    //sig value
    convfft(signal, lp1, cAUndec);
    cAUndec.erase(cAUndec.begin(), cAUndec.begin() + lf);
    cAUndec.erase(cAUndec.end() - lf + 1, cAUndec.end());
    downsamp(cAUndec, d, cA);
    //  cA.erase(cA.begin(),cA.begin()+(int) ceil(((double)lf-1.0)/2.0));
    //  cA.erase(cA.end()-(int) ceil(((double)lf-1.0)/2.0),cA.end());


    //High Pass Branch Computation

    std::vector<double> cDUndec;
    convfft(signal, hp1, cDUndec);
    cDUndec.erase(cDUndec.begin(), cDUndec.begin() + lf);
    cDUndec.erase(cDUndec.end() - lf + 1, cDUndec.end());
    downsamp(cDUndec, d, cD);
    //   cD.erase(cD.begin(),cD.begin()+(int) ceil(((double)lf-1.0)/2.0));
    //   cD.erase(cD.end()-(int) ceil(((double)lf-1.0)/2.0),cD.end());

    filtcoef(wname, lp1, hp1, lp2, hp2);

    return nullptr;
}

void* dwt1_sym_m(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD)
{
    std::vector<double> lp1, hp1, lp2, hp2;

    filtcoef(wname, lp1, hp1, lp2, hp2);
    const int d  = 2; // Downsampling Factor is 2
    const int lf = int(lp1.size());
    symm_ext(signal, lf - 1);

    std::vector<double> cAUndec;
    //sig value
    convfftm(signal, lp1, cAUndec);
    cAUndec.erase(cAUndec.begin(), cAUndec.begin() + lf);
    cAUndec.erase(cAUndec.end() - lf + 1, cAUndec.end());
    downsamp(cAUndec, d, cA);
    //  cA.erase(cA.begin(),cA.begin()+(int) ceil(((double)lf-1.0)/2.0));
    //  cA.erase(cA.end()-(int) ceil(((double)lf-1.0)/2.0),cA.end());


    //High Pass Branch Computation

    std::vector<double> cDUndec;
    convfftm(signal, hp1, cDUndec);
    cDUndec.erase(cDUndec.begin(), cDUndec.begin() + lf);
    cDUndec.erase(cDUndec.end() - lf + 1, cDUndec.end());
    downsamp(cDUndec, d, cD);
    //   cD.erase(cD.begin(),cD.begin()+(int) ceil(((double)lf-1.0)/2.0));
    //   cD.erase(cD.end()-(int) ceil(((double)lf-1.0)/2.0),cD.end());

    filtcoef(wname, lp1, hp1, lp2, hp2);

    return nullptr;
}

void* dwt_sym(std::vector<double>& signal, const int J, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag, std::vector<std::size_t>& length)
{
    std::size_t tempLen = signal.size();
    if ((tempLen % 2) != 0)
    {
        const double temp = signal[tempLen - 1];
        signal.push_back(temp);
        flag.push_back(1);
        tempLen++;
    }
    else { flag.push_back(0); }
    length.push_back(tempLen);
    flag.push_back(double(J));
    // flag[2] contains symmetric extension length


    std::vector<double> appxSig, detSig;
    const std::vector<double> originalCopy = signal;


    //  Storing Filter Values for GnuPlot
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(nm, lp1, hp1, lp2, hp2);
    for (std::size_t iter = 0; iter < std::size_t(J); ++iter)
    {
        dwt1_sym(nm, signal, appxSig, detSig);
        dwtOutput.insert(dwtOutput.begin(), detSig.begin(), detSig.end());
        std::size_t temp = detSig.size();
        length.insert(length.begin(), temp);

        if (iter == std::size_t(J - 1))
        {
            dwtOutput.insert(dwtOutput.begin(), appxSig.begin(), appxSig.end());
            length.insert(length.begin(), appxSig.size());
        }

        signal.clear();
        signal = appxSig;
        appxSig.clear();
        detSig.clear();
    }
    signal = originalCopy;

    return nullptr;
}


void* freq(std::vector<double>& sig, std::vector<double>& freqResp)
{
    const std::size_t k = sig.size();
    const std::size_t n = std::size_t(pow(2.0, ceil(log10(double(k)) / log10(2.0))));
    std::vector<std::complex<double>> fftOup;
    for (std::size_t i = 0; i < sig.size(); ++i)
    {
        double temp = sig[i];
        fftOup.push_back(std::complex<double>(temp, 0));
    }
    fft(fftOup, 1, n);

    for (std::size_t i = 0; i < n; ++i)
    {
        double temp = abs(fftOup[i]);
        freqResp.push_back(temp);
    }
    circshift(freqResp, int(n) / 2);
    return nullptr;
}

double convfft(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c)
{
    const std::size_t sz   = a.size() + b.size() - 1;
    fftw_complex* inpData  = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));
    fftw_complex* filtData = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));

    fftw_complex* inpFFT  = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));
    fftw_complex* filtFFT = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));

    fftw_complex* tempData = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));
    fftw_complex* tempIfft = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));

    fftw_plan plan_forward_inp  = fftw_plan_dft_1d(int(sz), inpData, inpFFT, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan plan_forward_filt = fftw_plan_dft_1d(int(sz), filtData, filtFFT, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan plan_backward     = fftw_plan_dft_1d(int(sz), tempData, tempIfft, FFTW_BACKWARD, FFTW_ESTIMATE);


    for (std::size_t i = 0; i < sz; ++i)
    {
        if (i < a.size()) { inpData[i][0] = a[i]; }
        else { inpData[i][0] = 0.0; }
        inpData[i][1] = 0.0;
        if (i < b.size()) { filtData[i][0] = b[i]; }
        else { filtData[i][0] = 0.0; }
        filtData[i][1] = 0.0;
    }


    fftw_execute(plan_forward_inp);

    fftw_execute(plan_forward_filt);

    for (std::size_t i = 0; i < sz; ++i)
    {
        tempData[i][0] = inpFFT[i][0] * filtFFT[i][0] - inpFFT[i][1] * filtFFT[i][1];
        tempData[i][1] = inpFFT[i][0] * filtFFT[i][1] + inpFFT[i][1] * filtFFT[i][0];
    }


    fftw_execute(plan_backward);

    for (std::size_t i = 0; i < sz; ++i)
    {
        double temp1;
        temp1 = tempIfft[i][0] / double(sz);
        c.push_back(temp1);
    }
    fftw_free(inpData);
    fftw_free(filtData);
    fftw_free(inpFFT);
    fftw_free(filtFFT);
    fftw_free(tempData);
    fftw_free(tempIfft);
    fftw_destroy_plan(plan_forward_inp);
    fftw_destroy_plan(plan_forward_filt);
    fftw_destroy_plan(plan_backward);

    return 0;
}

double convfftm(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c)
{
    const std::size_t sz   = a.size() + b.size() - 1;
    fftw_complex* inpData  = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));
    fftw_complex* filtData = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));

    fftw_complex* inpFFT  = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));
    fftw_complex* filtFFT = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));

    fftw_complex* tempData = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));
    fftw_complex* tempIfft = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sz));

    if (sz != fftTransientSize)
    {
        if (fftTransientSize != 0)
        {
            fftw_destroy_plan(plan_forward_inp);
            fftw_destroy_plan(plan_forward_filt);
            fftw_destroy_plan(plan_backward);
        }

        plan_forward_inp  = fftw_plan_dft_1d(int(sz), inpData, inpFFT, FFTW_FORWARD, FFTW_MEASURE);
        plan_forward_filt = fftw_plan_dft_1d(int(sz), filtData, filtFFT, FFTW_FORWARD, FFTW_MEASURE);
        plan_backward     = fftw_plan_dft_1d(int(sz), tempData, tempIfft, FFTW_BACKWARD, FFTW_MEASURE);
        fftTransientSize  = sz;
    }

    for (std::size_t i = 0; i < sz; ++i)
    {
        if (i < a.size()) { inpData[i][0] = a[i]; }
        else { inpData[i][0] = 0.0; }
        inpData[i][1] = 0.0;
        if (i < b.size()) { filtData[i][0] = b[i]; }
        else { filtData[i][0] = 0.0; }
        filtData[i][1] = 0.0;
    }


    fftw_execute_dft(plan_forward_inp, inpData, inpFFT);
    fftw_execute_dft(plan_forward_filt, filtData, filtFFT);


    for (std::size_t i = 0; i < sz; ++i)
    {
        tempData[i][0] = inpFFT[i][0] * filtFFT[i][0] - inpFFT[i][1] * filtFFT[i][1];
        tempData[i][1] = inpFFT[i][0] * filtFFT[i][1] + inpFFT[i][1] * filtFFT[i][0];
    }

    fftw_execute_dft(plan_backward, tempData, tempIfft);


    for (std::size_t i = 0; i < sz; ++i)
    {
        double temp1;
        temp1 = tempIfft[i][0] / double(sz);
        c.push_back(temp1);
    }
    fftw_free(inpData);
    fftw_free(filtData);
    fftw_free(inpFFT);
    fftw_free(filtFFT);
    fftw_free(tempData);
    fftw_free(tempIfft);

    return 0;
}

void* fft(std::vector<std::complex<double>>& data, const int sign, std::size_t n)
{
    double pi = - 3.14159265358979;
    if (sign == 1 || sign == -1) { pi = sign * pi; }
    else
    {
        std::cout << "Format fft(data, num), num = +1(fft) and num = -1 (Ifft)" << std::endl;
        exit(1);
    }
    const std::size_t len = data.size();
    const auto it         = data.end();
    if (len != n)
    {
        const std::size_t al = n - len;
        data.insert(it, al, std::complex<double>(0, 0));
    }

    const std::size_t k = std::size_t(pow(2.0, ceil(log10(double(n)) / log10(2.0))));
    const auto it1      = data.end();
    if (n < k)
    {
        const std::size_t al = k - n;
        data.insert(it1, al, std::complex<double>(0, 0));
        n = k;
    }

    bitreverse(data);

    //  radix2(data);
    for (std::size_t iter = 1; iter < n; iter <<= 1)
    {
        const std::size_t step = iter << 1;

        const double theta = pi / double(iter);

        double wtemp = sin(theta * .5);
        //   Multipliers
        const double wreal = -2 * wtemp * wtemp;
        const double wimag = sin(theta);

        //   Factors
        double wr = 1.0;
        double wi = 0.0;
        //   Iteration through two loops

        for (std::size_t m = 0; m < iter; ++m)
        {
            //   Iteration within m
            for (std::size_t i = m; i < n; i += step)
            {
                //   jth position
                const std::size_t j = i + iter;

                double tempr = wr * std::real(data[j]) - wi * std::imag(data[j]);
                double tempi = wr * std::imag(data[j]) + wi * std::real(data[j]);

                std::complex<double> temp(tempr, tempi);
                data[j] = data[i] - temp;
                data[i] += temp;
            }
            //   Twiddle Factors updated
            wtemp = wr;
            wr += wr * wreal - wi * wimag;
            wi += wi * wreal + wtemp * wimag;
        }
    }

    if (sign == -1)
    {
        const double scale = 1.0 / double(n);
        for (std::size_t i = 0; i < n; ++i) { data[i] *= scale; }
    }


    // Place holder
    return nullptr;
}


void* bitreverse(std::vector<std::complex<double>>& sig)
{
    const std::size_t len = sig.size();
    const std::size_t n   = std::size_t(pow(2.0, ceil(log10(double(len)) / log10(2.0))));
    std::size_t rev       = 0;
    //   Processing Input Data
    for (std::size_t iter = 0; iter < n; ++iter)
    {
        if (rev > iter)
        {
            //   Replacing current values with reversed values

            double tempr = std::real(sig[rev]);
            double tempi = std::imag(sig[rev]);
            const std::complex<double> temp(tempr, tempi);
            sig[rev]  = sig[iter];
            sig[iter] = temp;
        }
        //   Using filter "filt" such that the value of reverse changes with each iteration
        std::size_t filt = n;
        while (rev & (filt >>= 1)) { rev &= ~filt; }
        rev |= filt;
    }
    return nullptr;
}


void* dwt(std::vector<double>& sig, int j, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag, std::vector<std::size_t>& length)
{
    const int maxIter = int(ceil(log(double(sig.size())) / log(2.0))) - 2;

    if (maxIter < j) { j = maxIter; }

    std::vector<double> appxSig, detSig;
    const std::vector<double> originalCopy = sig;

    // Zero Pad the Signal to nearest 2^ M value ,where M is an integer.
    std::size_t tempLen = sig.size();
    if ((tempLen % 2) != 0)
    {
        const double temp = sig[tempLen - 1];
        sig.push_back(temp);
        flag.push_back(1);
        tempLen++;
    }
    else { flag.push_back(0); }
    length.push_back(tempLen);
    flag.push_back(double(j));

    std::vector<double> orig = sig;


    //  Storing Filter Values for GnuPlot
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(nm, lp1, hp1, lp2, hp2);


    for (std::size_t iter = 0; iter < std::size_t(j); ++iter)
    {
        dwt1(nm, orig, appxSig, detSig);
        dwtOutput.insert(dwtOutput.begin(), detSig.begin(), detSig.end());

        const int temp = detSig.size();
        length.insert(length.begin(), temp);

        if (iter == std::size_t(j - 1))
        {
            dwtOutput.insert(dwtOutput.begin(), appxSig.begin(), appxSig.end());
            std::size_t temp2 = appxSig.size();
            length.insert(length.begin(), temp2);
        }

        orig = appxSig;
        appxSig.clear();
        detSig.clear();
    }

    sig = originalCopy;
    return nullptr;
}


void circshift(std::vector<double>& sigCir, int l)
{
    if (abs(l) > int(sigCir.size())) { l = sign(l) * int(abs(l) % sigCir.size()); }

    if (l < 0)
    {
        l = int((sigCir.size() + l) % sigCir.size());
        // std::cout << "L" << L << std::endl;
    }
    for (std::size_t i = 0; i < std::size_t(l); ++i)
    {
        sigCir.push_back(sigCir[0]);
        sigCir.erase(sigCir.begin());
    }
}

double convol(std::vector<double>& a1, std::vector<double>& b1, std::vector<double>& c)
{
    const std::size_t lenC = a1.size() + b1.size() - 1;
    std::vector<double> a  = a1;
    std::vector<double> b  = b1;
    std::vector<double> oup(lenC);
    const auto itA  = a.end();
    const double al = double(lenC - a.size());
    a.insert(itA, al, 0);

    const auto itB  = b.end();
    const double bl = double(lenC - b.size());
    b.insert(itB, bl, 0);

    for (std::size_t ini = 0; ini < lenC; ++ini)
    {
        oup[ini]    = 0;
        double temp = 0;
        for (std::size_t jni = 0; jni <= ini; ++jni)
        {
            const double ou1 = a[jni] * b[ini - jni];
            oup[ini] += ou1;
        }
        temp = oup[ini];
        c.push_back(temp);
    }
    oup.clear();
    return 0;
}

void downsamp(std::vector<double>& sig, const int m, std::vector<double>& sigD)
{
    const std::size_t len = sig.size();
    const std::size_t n   = std::size_t(ceil(double(len) / double(m)));
    for (std::size_t i = 0; i < n; ++i)
    {
        double temp = sig[i * m];
        sigD.push_back(temp);
    }
}


void* dwt1(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD)
{
    std::vector<double> lpd, hpd, lpr, hpr;

    filtcoef(wname, lpd, hpd, lpr, hpr);

    const std::size_t lenLpfilt = lpd.size();
    const std::size_t lenHpfilt = hpd.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;
    const std::size_t lenSig    = 2 * std::size_t(ceil(double(signal.size()) / 2.0));

    // std::cout << lenLpfilt << "Filter" << std::endl;
    per_ext(signal, int(lenAvg / 2)); // Periodic Extension
    // computations designed to deal with boundary distortions

    // Low Pass Filtering Operations in the Analysis Filter Bank Section
    // int len_cA =(int)  floor(double (len_sig + lenLpfilt -1) / double (2));
    std::vector<double> cAUndec;
    // convolving signal with lpd, Low Pass Filter, and O/P is stored in cA_undec
    convfft(signal, lpd, cAUndec);
    const int d = 2; // Downsampling Factor is 2
    cAUndec.erase(cAUndec.begin(), cAUndec.begin() + lenAvg - 1);
    cAUndec.erase(cAUndec.end() - lenAvg + 1, cAUndec.end());
    cAUndec.erase(cAUndec.begin() + lenSig, cAUndec.end());
    cAUndec.erase(cAUndec.begin());


    // Downsampling by 2 gives cA
    downsamp(cAUndec, d, cA);

    //     cA.erase(cA.begin(),cA.begin()+len_avg/2);
    //      cA.erase(cA.end()-len_avg/2,cA.end());

    // High Pass Filtering Operations in the Analysis Filter Bank Section
    // int len_cA =(int)  floor(double (len_sig + lenLpfilt -1) / double (2));

    std::vector<double> cDUndec;
    // convolving signal with lpd, Low Pass Filter, and O/P is stored in cA_undec
    convfft(signal, hpd, cDUndec);
    cDUndec.erase(cDUndec.begin(), cDUndec.begin() + lenAvg - 1);
    cDUndec.erase(cDUndec.end() - lenAvg + 1, cDUndec.end());
    cDUndec.erase(cDUndec.begin() + lenSig, cDUndec.end());
    cDUndec.erase(cDUndec.begin());

    // Downsampling Factor is 2

    // Downsampling by 2 gives cA
    downsamp(cDUndec, d, cD);

    //            cD.erase(cD.begin(),cD.begin()+len_avg/2);
    //          cD.erase(cD.end()-len_avg/2,cD.end());

    filtcoef(wname, lpd, hpd, lpr, hpr);

    return nullptr;
}

void* dwt1_m(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD)
{
    std::vector<double> lpd, hpd, lpr, hpr;

    filtcoef(wname, lpd, hpd, lpr, hpr);

    const std::size_t lenLpfilt = lpd.size();
    const std::size_t lenHpfilt = hpd.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;
    const std::size_t lenSig    = 2 * std::size_t(ceil(double(signal.size()) / 2.0));

    // std::cout << lenLpfilt << "Filter" << std::endl;
    per_ext(signal, int(lenAvg / 2)); // Periodic Extension
    // computations designed to deal with boundary distortions

    // Low Pass Filtering Operations in the Analysis Filter Bank Section
    // int len_cA =(int)  floor(double (len_sig + lenLpfilt -1) / double (2));
    std::vector<double> cAUndec;
    // convolving signal with lpd, Low Pass Filter, and O/P is stored in cA_undec
    convfftm(signal, lpd, cAUndec);
    const int d = 2; // Downsampling Factor is 2
    cAUndec.erase(cAUndec.begin(), cAUndec.begin() + lenAvg - 1);
    cAUndec.erase(cAUndec.end() - lenAvg + 1, cAUndec.end());
    cAUndec.erase(cAUndec.begin() + lenSig, cAUndec.end());
    cAUndec.erase(cAUndec.begin());


    // Downsampling by 2 gives cA
    downsamp(cAUndec, d, cA);

    //     cA.erase(cA.begin(),cA.begin()+len_avg/2);
    //      cA.erase(cA.end()-len_avg/2,cA.end());

    // High Pass Filtering Operations in the Analysis Filter Bank Section
    // int len_cA =(int)  floor(double (len_sig + lenLpfilt -1) / double (2));

    std::vector<double> cDUndec;
    // convolving signal with lpd, Low Pass Filter, and O/P is stored in cA_undec
    convfftm(signal, hpd, cDUndec);
    cDUndec.erase(cDUndec.begin(), cDUndec.begin() + lenAvg - 1);
    cDUndec.erase(cDUndec.end() - lenAvg + 1, cDUndec.end());
    cDUndec.erase(cDUndec.begin() + lenSig, cDUndec.end());
    cDUndec.erase(cDUndec.begin());

    // Downsampling Factor is 2

    // Downsampling by 2 gives cA
    downsamp(cDUndec, d, cD);

    //            cD.erase(cD.begin(),cD.begin()+len_avg/2);
    //          cD.erase(cD.end()-len_avg/2,cD.end());

    filtcoef(wname, lpd, hpd, lpr, hpr);

    return nullptr;
}


void* dyadic_zpad_1d(std::vector<double>& signal)
{
    const std::size_t n = signal.size();
    const double m      = log10(double(n)) / log10(2.0);
    const int d         = int(ceil(m));
    const double intVal = pow(2.0, double(d)) - pow(2.0, m);

    const int z      = int(intVal);
    const auto itA   = signal.end();
    const double val = signal[n - 1];
    //  double val = 0;
    signal.insert(itA, z, val);
    return nullptr;
}


void* idwt(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<double>& idwtOutput, std::vector<std::size_t>& length)
{
    const std::size_t j = std::size_t(flag[1]);
    //       int zpad =(int) flag[0];

    std::vector<double> app;
    std::vector<double> detail;
    std::size_t appLen = length[0];
    std::size_t detLen = length[1];

    const auto dwt = dwtop.begin();
    app.assign(dwt, dwtop.begin() + appLen);
    detail.assign(dwtop.begin() + appLen, dwtop.begin() + 2 * appLen);

    for (std::size_t i = 0; i < j; ++i)
    {
        idwt1(nm, idwtOutput, app, detail);
        appLen += detLen;
        app.clear();
        detail.clear();
        if (i < j - 1)
        {
            detLen = length[i + 2];
            for (std::size_t l = 0; l < detLen; ++l)
            {
                double temp = dwtop[appLen + l];
                detail.push_back(temp);
            }
            app = idwtOutput;

            if (app.size() >= detail.size())
            {
                const std::size_t t    = app.size() - detail.size();
                const std::size_t lent = std::size_t(floor(double(t) / 2.0));
                app.erase(app.begin() + detail.size() + lent, app.end());
                app.erase(app.begin(), app.begin() + lent);
            }
        }

        //int value1 = (int) ceil(double(app.size() - det_len)/2.0);
        //int value2 = (int) floor(double(app.size() - det_len)/2.0);

        //app.erase(app.end() -value2,app.end());
        //app.erase(app.begin(),app.begin()+value1);
    }


    // Remove ZeroPadding

    const int zerop = int(flag[0]);
    idwtOutput.erase(idwtOutput.end() - zerop, idwtOutput.end());

    return nullptr;
}

void* idwt1_m(const std::string& wname, std::vector<double>& x, std::vector<double>& cA, std::vector<double>& cD)
{
    std::vector<double> lpd1, hpd1, lpr1, hpr1;

    filtcoef(wname, lpd1, hpd1, lpr1, hpr1);
    const std::size_t lenLpfilt = lpr1.size();
    const std::size_t lenHpfilt = hpr1.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;
    const std::size_t n         = 2 * cD.size();
    const int U                 = 2; // Upsampling Factor

    // Operations in the Low Frequency branch of the Synthesis Filter Bank

    std::vector<double> cAUp;
    std::vector<double> xLp;
    // int len1 = cAUp.size();
    upsamp(cA, U, cAUp);
    per_ext(cAUp, int(lenAvg / 2));
    convfftm(cAUp, lpr1, xLp);

    // Operations in the High Frequency branch of the Synthesis Filter Bank
    std::vector<double> cDUp, xHp;
    upsamp(cD, U, cDUp);
    per_ext(cDUp, int(lenAvg / 2));
    convfftm(cDUp, hpr1, xHp);

    // Remove periodic extension
    //   X.erase(X.begin(),X.begin()+len_avg+len_avg/2-1);
    //   X.erase(X.end()-len_avg-len_avg/2,X.end());
    xLp.erase(xLp.begin() + n + lenAvg - 1, xLp.end());
    xLp.erase(xLp.begin(), xLp.begin() + lenAvg - 1);
    xHp.erase(xHp.begin() + n + lenAvg - 1, xHp.end());
    xHp.erase(xHp.begin(), xHp.begin() + lenAvg - 1);
    vecsum(xLp, xHp, x);
    return nullptr;
}

void* idwt1(const std::string& wname, std::vector<double>& X, std::vector<double>& cA, std::vector<double>& cD)
{
    std::vector<double> lpd1, hpd1, lpr1, hpr1;

    filtcoef(wname, lpd1, hpd1, lpr1, hpr1);
    const std::size_t lenLpfilt = lpr1.size();
    const std::size_t lenHpfilt = hpr1.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;
    const std::size_t n         = 2 * cD.size();
    const int U                 = 2; // Upsampling Factor

    // Operations in the Low Frequency branch of the Synthesis Filter Bank

    std::vector<double> cAUp, xLp;
    // int len1 = cAUp.size();
    upsamp(cA, U, cAUp);

    per_ext(cAUp, int(lenAvg / 2));


    convfft(cAUp, lpr1, xLp);


    // Operations in the High Frequency branch of the Synthesis Filter Bank

    std::vector<double> cDUp, xHp;
    upsamp(cD, U, cDUp);
    per_ext(cDUp, int(lenAvg / 2));


    convfft(cDUp, hpr1, xHp);

    // Remove periodic extension

    //   X.erase(X.begin(),X.begin()+len_avg+len_avg/2-1);
    //   X.erase(X.end()-len_avg-len_avg/2,X.end());

    xLp.erase(xLp.begin() + n + lenAvg - 1, xLp.end());
    xLp.erase(xLp.begin(), xLp.begin() + lenAvg - 1);

    xHp.erase(xHp.begin() + n + lenAvg - 1, xHp.end());
    xHp.erase(xHp.begin(), xHp.begin() + lenAvg - 1);

    vecsum(xLp, xHp, X);

    return nullptr;
}

int sign(const int x)
{
    if (x >= 0) { return 1; }
    return -1;
}

void upsamp(std::vector<double>& sig, const int m, std::vector<double>& sigU)
{
    const std::size_t len = sig.size();
    const std::size_t n   = std::size_t(ceil(double(len) * double(m)));

    for (std::size_t i = 0; i < n; ++i)
    {
        if (i % m == 0)
        {
            double temp = sig[i / m];
            sigU.push_back(temp);
        }
        else { sigU.push_back(0); }
    }
}

double OPSum(const double i, const double j) { return (i + j); }

int vecsum(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c)
{
    c.resize(a.size());
    transform(a.begin(), a.end(), b.begin(), b.begin(), OPSum);
    c = b;
    return 0;
}

void* getcoeff2d(std::vector<std::vector<double>>& dwtoutput, std::vector<std::vector<double>>& cH, std::vector<std::vector<double>>& cV,
                 std::vector<std::vector<double>>& cD, std::vector<double>& flag, int& n)
{
    if (n > flag[2])
    {
        std::cout << "Signal is decimated only up to " << flag[2] << " levels" << std::endl;
        exit(1);
    }
    const std::size_t rows = dwtoutput.size();
    const std::size_t cols = dwtoutput[0].size();
    // Getting Horizontal Coefficients
    const std::size_t r = std::size_t(ceil(double(rows) / pow(2.0, n)));
    const std::size_t c = std::size_t(ceil(double(cols) / pow(2.0, n)));

    for (std::size_t i = 0; i < std::size_t(ceil(double(rows) / pow(2.0, n))); ++i)
    {
        for (std::size_t j = 0; j < std::size_t(ceil(double(cols) / pow(2.0, n))); ++j) { cH[i][j] = dwtoutput[i][c + j]; }
    }
    for (std::size_t i = 0; i < std::size_t(ceil(double(rows) / pow(2.0, n))); ++i)
    {
        for (std::size_t j = 0; j < std::size_t(ceil(double(cols) / pow(2.0, n))); ++j) { cV[i][j] = dwtoutput[i + r][j]; }
    }
    for (std::size_t i = 0; i < std::size_t(ceil(double(rows) / pow(2.0, n))); ++i)
    {
        for (std::size_t j = 0; j < std::size_t(ceil(double(cols) / pow(2.0, n))); ++j) { cD[i][j] = dwtoutput[i + r][c + j]; }
    }

    return nullptr;
}

void* zero_remove(std::vector<std::vector<double>>& input, std::vector<std::vector<double>>& output)
{
    const int zeroRows = int(output.size() - input.size());
    const int zeroCols = int(output[0].size() - input[0].size());

    auto row = output.end() - zeroRows;

    const std::size_t ousize = output.size();
    for (std::size_t i = input.size(); i < ousize; ++i)
    {
        output.erase(row);
        ++row;
    }

    // std::size_t ousize2 = output[0].size();


    for (std::size_t i = 0; i < ousize; ++i)
    {
        const auto col = output[i].end() - zeroCols;
        output[i].erase(col, output[i].end());
    }
    return nullptr;
}

void* dwt_output_dim(std::vector<std::vector<double>>& signal, int& r, int& c)
{
    const std::size_t rows = signal.size();
    const std::size_t cols = signal[0].size();

    const double mr        = log10(double(rows)) / log10(2.0);
    const int dr           = int(ceil(mr));
    const double intValRow = pow(2.0, double(dr));
    const int r1           = int(intValRow);

    const double mc         = log10(double(cols)) / log10(2.0);
    const int dc            = int(ceil(mc));
    const double intValCols = pow(2.0, double(dc));
    const int c1            = int(intValCols);
    r                       = std::max(r1, c1);
    c                       = std::max(r1, c1);

    return nullptr;
}

void* dyadic_zpad_2d(std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& mod)
{
    const std::size_t rows = signal.size();
    const std::size_t cols = signal[0].size();

    for (std::size_t i = 0; i < rows; ++i) { for (std::size_t j = 0; j < cols; ++j) { mod[i][j] = signal[i][j]; } }
    // Zeropadding the columns

    const double mr        = log10(double(rows)) / log10(2.0);
    const int dr           = int(ceil(mr));
    const double intValRow = pow(2.0, double(dr)) - pow(2.0, mr);

    const int zerosRow = int(intValRow);

    const double mc         = log10(double(cols)) / log10(2.0);
    const int dc            = int(ceil(mc));
    const double intValCols = pow(2.0, double(dc)) - pow(2.0, mc);

    const int zerosCols = int(intValCols);

    for (std::size_t i = 0; i < rows + zerosRow; ++i) { for (std::size_t j = cols; j < cols + zerosCols; ++j) { mod[i][j] = 0; } }
    for (std::size_t i = rows; i < rows + zerosRow; ++i) { for (std::size_t j = 0; j < cols + zerosCols; ++j) { mod[i][j] = 0; } }

    return nullptr;
}

void* idwt_2d(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<std::vector<double>>& idwtOutput, std::vector<std::size_t>& length)
{
    std::size_t J    = std::size_t(flag[0]);
    std::size_t rows = length[0];
    std::size_t cols = length[1];

    std::size_t sumCoef = 0;
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(nm, lp1, hp1, lp2, hp2);
    std::vector<std::vector<double>> cLL(rows, std::vector<double>(cols));


    for (std::size_t iter = 0; iter < J; ++iter)
    {
        std::size_t nRows = length[2 * iter];
        std::size_t nCols = length[2 * iter + 1];

        std::vector<std::vector<double>> cLH(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cHL(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cHH(nRows, std::vector<double>(nCols));

        for (std::size_t i = 0; i < nRows; ++i)
        {
            for (std::size_t j = 0; j < nCols; ++j)
            {
                if (iter == 0)
                {
                    cLL[i][j] = dwtop[sumCoef + i * nCols + j];
                    cLH[i][j] = dwtop[sumCoef + nRows * nCols + i * nCols + j];
                    cHL[i][j] = dwtop[sumCoef + 2 * nRows * nCols + i * nCols + j];
                    cHH[i][j] = dwtop[sumCoef + 3 * nRows * nCols + i * nCols + j];
                }
                else
                {
                    cLH[i][j] = dwtop[sumCoef + i * nCols + j];
                    cHL[i][j] = dwtop[sumCoef + nRows * nCols + i * nCols + j];
                    cHH[i][j] = dwtop[sumCoef + 2 * nRows * nCols + i * nCols + j];
                }
            }
        }


        //      temp_A = cLL;
        //      idwt2_sym(nm,idwtOutput2, cA, cH,cV,cD);

        std::size_t lenX = cLH.size();
        std::size_t lenY = cLH[0].size();

        // Row Upsampling and Column Filtering at the first LP Stage
        std::vector<std::vector<double>> cL(2 * lenX, std::vector<double>(lenY));
        std::vector<std::vector<double>> cH(2 * lenX, std::vector<double>(lenY));

        if (iter == 0)
        {
            for (std::size_t j = 0; j < lenY; ++j)
            {
                std::vector<double> sigLL, sigLH, oup;

                for (std::size_t i = 0; i < lenX; ++i)
                {
                    double temp1 = cLL[i][j];
                    double temp2 = cLH[i][j];
                    sigLL.push_back(temp1);
                    sigLH.push_back(temp2);
                }
                idwt1_m(nm, oup, sigLL, sigLH);

                for (std::size_t i = 0; i < oup.size(); ++i) { cL[i][j] = oup[i]; }
            }
        }
        else
        {
            std::size_t rows1 = cLH.size();
            std::size_t cols1 = cLH[0].size();

            for (std::size_t j = 0; j < cols1; ++j)
            {
                std::vector<double> tempL1, tempL2, oup;
                for (std::size_t i = 0; i < rows1; ++i)
                {
                    double temp = cLL[i][j];
                    tempL1.push_back(temp);

                    double temp2 = cLH[i][j];
                    tempL2.push_back(temp2);
                }
                idwt1_m(nm, oup, tempL1, tempL2);

                for (std::size_t i = 0; i < oup.size(); ++i) { cL[i][j] = oup[i]; }
            }
        }


        for (std::size_t j = 0; j < lenY; ++j)
        {
            std::vector<double> sigHL, sigHH, oup2;
            for (std::size_t i = 0; i < lenX; ++i)
            {
                double temp3 = cHL[i][j];
                double temp4 = cHH[i][j];
                sigHL.push_back(temp3);
                sigHH.push_back(temp4);
            }
            idwt1_m(nm, oup2, sigHL, sigHH);

            for (std::size_t i = 0; i < oup2.size(); ++i) { cH[i][j] = oup2[i]; }
        }

        std::vector<std::vector<double>> signal(2 * lenX, std::vector<double>(2 * lenY));
        for (std::size_t i = 0; i < 2 * lenX; ++i)
        {
            std::vector<double> sigL, sigH, oup;
            for (std::size_t j = 0; j < lenY; ++j)
            {
                double temp5 = cL[i][j];
                double temp6 = cH[i][j];
                sigL.push_back(temp5);
                sigH.push_back(temp6);
            }
            idwt1_m(nm, oup, sigL, sigH);

            for (std::size_t j = 0; j < oup.size(); ++j) { signal[i][j] = oup[j]; }
        }

        idwtOutput = signal;


        if (iter == 0) { sumCoef += 4 * nRows * nCols; }
        else { sumCoef += 3 * nRows * nCols; }
        cLL = signal;
    }


    return nullptr;
}


void* dwt_2d(std::vector<std::vector<double>>& origsig, const int J, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag, std::vector<std::size_t>& length)
{
    // flag will contain

    std::vector<std::vector<double>> sig          = origsig;
    std::size_t nRows                             = sig.size(); // No. of rows
    std::size_t nCols                             = sig[0].size(); //No. of columns
    std::vector<std::vector<double>> originalCopy = sig;
    const int maxIter                             = std::min(int(ceil(log(double(sig.size())) / log(2.0))), int(ceil(log(double(sig[0].size())) / log(2.0))));
    if (maxIter < J)
    {
        std::cout << J << " Iterations are not possible with signals of this dimension " << std::endl;
        exit(1);
    }
    std::vector<double> lp1, hp1, lp2, hp2;

    flag.push_back(double(J));
    flag.push_back(0);


    length.insert(length.begin(), nCols);
    length.insert(length.begin(), nRows);


    for (std::size_t iter = 0; iter < std::size_t(J); ++iter)
    {
        filtcoef(nm, lp1, hp1, lp2, hp2);

        nRows = int(ceil(double(nRows) / 2.0));
        nCols = int(ceil(double(nCols) / 2.0));
        length.insert(length.begin(), nCols);
        length.insert(length.begin(), nRows);

        std::vector<std::vector<double>> cA(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cH(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cV(nRows, std::vector<double>(nCols));
        std::vector<std::vector<double>> cD(nRows, std::vector<double>(nCols));

        if (iter == 0) { dwt2(nm, originalCopy, cA, cH, cV, cD); }
        else { dwt2(nm, originalCopy, cA, cH, cV, cD); }
        std::vector<double> tempSig2;

        originalCopy = cA;
        if (iter == std::size_t(J - 1))
        {
            for (std::size_t i = 0; i < nRows; ++i)
            {
                for (std::size_t j = 0; j < nCols; ++j)
                {
                    double temp = cA[i][j];
                    tempSig2.push_back(temp);
                }
            }
        }
        for (std::size_t i = 0; i < nRows; ++i)
        {
            for (std::size_t j = nCols; j < nCols * 2; ++j)
            {
                double temp = cH[i][j - nCols];
                tempSig2.push_back(temp);
            }
        }

        for (std::size_t i = nRows; i < nRows * 2; ++i)
        {
            for (std::size_t j = 0; j < nCols; ++j)
            {
                double temp = cV[i - nRows][j];
                tempSig2.push_back(temp);
            }
        }

        for (std::size_t i = nRows; i < nRows * 2; ++i)
        {
            for (std::size_t j = nCols; j < nCols * 2; ++j)
            {
                double temp = cD[i - nRows][j - nCols];
                tempSig2.push_back(temp);
            }
        }

        dwtOutput.insert(dwtOutput.begin(), tempSig2.begin(), tempSig2.end());
    }
    /*
    ofstream dwt2out("dwt2out.dat");
    for (std::size_t i= 0; i < dwtOutput.size(); ++i){ dwt2out << dwtOutput[i] <<endl; }
    */

    return nullptr;
}


void* branch_lp_hp_up(const std::string& wname, std::vector<double>& cA, std::vector<double>& cD, std::vector<double>& x)
{
    std::vector<double> lpd1, hpd1, lpr1, hpr1;

    filtcoef(wname, lpd1, hpd1, lpr1, hpr1);
    const std::size_t lenLpfilt = lpr1.size();
    const std::size_t lenHpfilt = hpr1.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;
    //std::size_t N = 2 * cA.size();
    const int U = 2; // Upsampling Factor

    // Operations in the Low Frequency branch of the Synthesis Filter Bank

    std::vector<double> cAUp;
    std::vector<double> xLp;
    per_ext(cA, int(lenAvg / 2));
    upsamp(cA, U, cAUp);
    convfftm(cAUp, lpr1, xLp);

    // Operations in the High Frequency branch of the Synthesis Filter Bank

    std::vector<double> cDUp;
    std::vector<double> xHp;
    per_ext(cD, int(lenAvg / 2));
    upsamp(cD, U, cDUp);
    convfftm(cDUp, hpr1, xHp);
    vecsum(xLp, xHp, x);
    // Remove periodic extension
    x.erase(x.begin(), x.begin() + lenAvg + lenAvg / 2 - 1);
    x.erase(x.end() - lenAvg - lenAvg / 2, x.end());

    return nullptr;
}

void* branch_hp_dn(const std::string& wname, std::vector<double>& signal, std::vector<double>& sigop)
{
    std::vector<double> lpd, hpd, lpr, hpr;

    filtcoef(wname, lpd, hpd, lpr, hpr);
    //for (std::size_t i = 0;  i < signal.size(); ++i) {
    // std::cout << signal[i] << std::endl;
    // out2 << signal[i] << std::endl;
    //}

    const std::size_t tempLen = signal.size();

    if ((tempLen % 2) != 0)
    {
        const double temp = signal[tempLen - 1];
        signal.push_back(temp);
    }

    const std::size_t lenLpfilt = lpd.size();
    const std::size_t lenHpfilt = hpd.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;

    // std::cout << lenLpfilt << "Filter" << std::endl;
    per_ext(signal, int(lenAvg / 2)); // Periodic Extension
    // computations designed to deal with boundary distortions

    // Low Pass Filtering Operations in the Analysis Filter Bank Section
    // int len_cA =(int)  floor(double (len_sig + lenLpfilt -1) / double (2));
    std::vector<double> cAUndec;
    // convolving signal with lpd, Low Pass Filter, and O/P is stored in cA_undec
    convfftm(signal, hpd, cAUndec);
    const int d = 2; // Downsampling Factor is 2

    // Downsampling by 2 gives cA
    downsamp(cAUndec, d, sigop);

    sigop.erase(sigop.begin(), sigop.begin() + lenAvg / 2);
    sigop.erase(sigop.end() - lenAvg / 2, sigop.end());
    return nullptr;
}

void* branch_lp_dn(const std::string& wname, std::vector<double>& signal, std::vector<double>& sigop)
{
    std::vector<double> lpd, hpd, lpr, hpr;

    filtcoef(wname, lpd, hpd, lpr, hpr);
    // for (std::size_t i = 0;  i < signal.size(); ++i) {
    // std::cout << signal[i] <<  endl;
    // out2 << signal[i] <<endl;
    // }

    const std::size_t tempLen = signal.size();

    if ((tempLen % 2) != 0)
    {
        const double temp = signal[tempLen - 1];
        signal.push_back(temp);
    }

    const std::size_t lenLpfilt = lpd.size();
    const std::size_t lenHpfilt = hpd.size();
    const std::size_t lenAvg    = (lenLpfilt + lenHpfilt) / 2;

    // std::cout << lenLpfilt << "Filter" << std::endl;
    per_ext(signal, int(lenAvg / 2)); // Periodic Extension
    // computations designed to deal with boundary distortions

    // Low Pass Filtering Operations in the Analysis Filter Bank Section
    // int len_cA =(int)  floor(double (len_sig + lenLpfilt -1) / double (2));
    std::vector<double> cAUndec;
    // convolving signal with lpd, Low Pass Filter, and O/P is stored in cA_undec
    convfftm(signal, lpd, cAUndec);
    const int d = 2; // Downsampling Factor is 2

    // Downsampling by 2 gives cA
    downsamp(cAUndec, d, sigop);

    sigop.erase(sigop.begin(), sigop.begin() + lenAvg / 2);
    sigop.erase(sigop.end() - lenAvg / 2, sigop.end());


    return nullptr;
}

void* idwt2(const std::string& name, std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& cLL, std::vector<std::vector<double>>& cLH,
            std::vector<std::vector<double>>& cHL, std::vector<std::vector<double>>& cHH)
{
    // Synthesis
    const std::size_t rows  = cLL.size();
    const std::size_t cols  = cLL[0].size();
    const std::size_t nRows = 2 * rows;
    // Row Upsampling and Column Filtering at the first LP Stage
    std::vector<std::vector<double>> cL(nRows, std::vector<double>(cols));
    std::vector<std::vector<double>> cH(nRows, std::vector<double>(cols));

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> sigLL;
        std::vector<double> sigLH;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp1 = cLL[i][j];
            double temp2 = cLH[i][j];
            sigLL.push_back(temp1);
            sigLH.push_back(temp2);
        }
        std::vector<double> oup;

        branch_lp_hp_up(name, sigLL, sigLH, oup);
        sigLL.clear();
        sigLH.clear();
        for (std::size_t i = 0; i < oup.size(); ++i) { cL[i][j] = oup[i]; }
    }

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> sigHL;
        std::vector<double> sigHH;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp3 = cHL[i][j];
            double temp4 = cHH[i][j];
            sigHL.push_back(temp3);
            sigHH.push_back(temp4);
        }
        std::vector<double> oup2;
        branch_lp_hp_up(name, sigHL, sigHH, oup2);
        sigHL.clear();
        sigHH.clear();

        for (std::size_t i = 0; i < oup2.size(); ++i) { cH[i][j] = oup2[i]; }
    }

    for (std::size_t i = 0; i < nRows; ++i)
    {
        std::vector<double> sigL;
        std::vector<double> sigH;
        for (std::size_t j = 0; j < cols; ++j)
        {
            double temp5 = cL[i][j];
            double temp6 = cH[i][j];
            sigL.push_back(temp5);
            sigH.push_back(temp6);
        }
        std::vector<double> oup3;
        branch_lp_hp_up(name, sigL, sigH, oup3);
        sigL.clear();
        sigH.clear();

        for (std::size_t j = 0; j < oup3.size(); ++j) { signal[i][j] = oup3[j]; }
    }
    return nullptr;
}

void* dwt2(const std::string& name, std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& cLL, std::vector<std::vector<double>>& cLH,
           std::vector<std::vector<double>>& cHL, std::vector<std::vector<double>>& cHH)
{
    //Analysis
    const std::size_t rows    = signal.size();
    std::size_t cols          = signal[0].size();
    const std::size_t colsLp1 = cLL[0].size();
    const std::size_t colsHp1 = cLL[0].size();
    std::vector<double> lp1, hp1, lp2, hp2;
    filtcoef(name, lp1, hp1, lp2, hp2);
    std::vector<std::vector<double>> lpDn1(rows, std::vector<double>(colsLp1));
    std::vector<std::vector<double>> hpDn1(rows, std::vector<double>(colsHp1));

    // Implementing row filtering and column downsampling in each branch.
    for (std::size_t i = 0; i < rows; ++i)
    {
        std::vector<double> tempRow, oupLp, oupHp;
        for (std::size_t j = 0; j < cols; ++j)
        {
            double temp = signal[i][j];
            tempRow.push_back(temp);
        }
        dwt1_m(name, tempRow, oupLp, oupHp);

        for (std::size_t j = 0; j < oupLp.size(); ++j)
        {
            lpDn1[i][j] = oupLp[j];
            hpDn1[i][j] = oupHp[j];
        }
    }


    cols = colsLp1;
    // Implementing column filtering and row downsampling in Low Pass branch.

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> tempRow3, oupLp, oupHp;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = lpDn1[i][j];
            tempRow3.push_back(temp);
        }
        dwt1_m(name, tempRow3, oupLp, oupHp);

        for (std::size_t i = 0; i < oupLp.size(); ++i)
        {
            cLL[i][j] = oupLp[i];
            cLH[i][j] = oupHp[i];
        }
    }


    // Implementing column filtering and row downsampling in High Pass branch.

    for (std::size_t j = 0; j < cols; ++j)
    {
        std::vector<double> tempRow5, oupLp, oupHp;
        for (std::size_t i = 0; i < rows; ++i)
        {
            double temp = hpDn1[i][j];
            tempRow5.push_back(temp);
        }
        dwt1_m(name, tempRow5, oupLp, oupHp);

        for (std::size_t i = 0; i < oupLp.size(); ++i)
        {
            cHL[i][j] = oupLp[i];
            cHH[i][j] = oupHp[i];
        }
    }
    return nullptr;
}


void* downsamp2(std::vector<std::vector<double>>& vec1, std::vector<std::vector<double>>& vec2, const int rowsDn, const int colsDn)
{
    const std::size_t rows  = vec1.size();
    const std::size_t cols  = vec1[0].size();
    const std::size_t nRows = std::size_t(ceil(double(rows) / double(rowsDn)));
    const std::size_t nCols = std::size_t(ceil(double(cols) / double(colsDn)));
    for (std::size_t i = 0; i < nRows; ++i) { for (std::size_t j = 0; j < nCols; ++j) { vec2[i][j] = vec1[i * rowsDn][j * colsDn]; } }
    return nullptr;
}

void* upsamp2(std::vector<std::vector<double>>& vec1, std::vector<std::vector<double>>& vec2, const int rowsUp, const int colsUp)
{
    const std::size_t rows  = vec1.size();
    const std::size_t cols  = vec1[0].size();
    const std::size_t nRows = rows * rowsUp;
    const std::size_t nCols = cols * colsUp;
    for (std::size_t i = 0; i < nRows; ++i)
    {
        for (std::size_t j = 0; j < nCols; ++j)
        {
            if (i % rowsUp == 0 && j % colsUp == 0) { vec2[i][j] = vec1[i / rowsUp][j / colsUp]; }
            else { vec2[i][j] = 0; }
        }
    }
    return nullptr;
}


int filtcoef(const std::string& name, std::vector<double>& lp1, std::vector<double>& hp1, std::vector<double>& lp2, std::vector<double>& hp2)
{
    if (name == "haar" || name == "db1")
    {
        lp1.push_back(0.7071);
        lp1.push_back(0.7071);
        hp1.push_back(-0.7071);
        hp1.push_back(0.7071);
        lp2.push_back(0.7071);
        lp2.push_back(0.7071);
        hp2.push_back(0.7071);
        hp2.push_back(-0.7071);
        //      std::cout << lp2[1] << std::endl;
        //      hpd = {-0.7071, 0.7071};
        //      lpr = {0.7071, 0.7071};
        //      hpr = {0.7071, -0.7071};
        return 0;
    }
    if (name == "db2")
    {
        lp1 = { -0.12940952255092145, 0.22414386804185735, 0.83651630373746899, 0.48296291314469025 };
        hp1 = { -0.48296291314469025, 0.83651630373746899, -0.22414386804185735, -0.12940952255092145 };
        lp2 = { 0.48296291314469025, 0.83651630373746899, 0.22414386804185735, -0.12940952255092145 };
        hp2 = { -0.12940952255092145, -0.22414386804185735, 0.83651630373746899, -0.48296291314469025 };
        return 0;
    }
    if (name == "db3")
    {
        lp1 = { 0.035226291882100656, -0.085441273882241486, -0.13501102001039084, 0.45987750211933132, 0.80689150931333875, 0.33267055295095688 };
        hp1 = { -0.33267055295095688, 0.80689150931333875, -0.45987750211933132, -0.13501102001039084, 0.085441273882241486, 0.035226291882100656 };
        lp2 = { 0.33267055295095688, 0.80689150931333875, 0.45987750211933132, -0.13501102001039084, -0.085441273882241486, 0.035226291882100656 };
        hp2 = { 0.035226291882100656, 0.085441273882241486, -0.13501102001039084, -0.45987750211933132, 0.80689150931333875, -0.33267055295095688 };
        return 0;
    }
    if (name == "db4")
    {
        lp1 = {
            -0.010597401784997278, 0.032883011666982945, 0.030841381835986965, -0.18703481171888114, -0.027983769416983849, 0.63088076792959036,
            0.71484657055254153, 0.23037781330885523
        };

        hp1 = {
            -0.23037781330885523, 0.71484657055254153, -0.63088076792959036, -0.027983769416983849, 0.18703481171888114, 0.030841381835986965,
            -0.032883011666982945, -0.010597401784997278
        };

        lp2 = {
            0.23037781330885523, 0.71484657055254153, 0.63088076792959036, -0.027983769416983849, -0.18703481171888114, 0.030841381835986965,
            0.032883011666982945, -0.010597401784997278
        };

        hp2 = {
            -0.010597401784997278, -0.032883011666982945, 0.030841381835986965, 0.18703481171888114, -0.027983769416983849, -0.63088076792959036,
            0.71484657055254153, -0.23037781330885523
        };
        return 0;
    }
    if (name == "db5")
    {
        lp1 = {
            0.0033357252850015492, -0.012580751999015526, -0.0062414902130117052, 0.077571493840065148, -0.03224486958502952, -0.24229488706619015,
            0.13842814590110342, 0.72430852843857441, 0.60382926979747287, 0.16010239797412501
        };

        hp1 = {
            -0.16010239797412501, 0.60382926979747287, -0.72430852843857441, 0.13842814590110342, 0.24229488706619015, -0.03224486958502952,
            -0.077571493840065148, -0.0062414902130117052, 0.012580751999015526, 0.0033357252850015492
        };

        lp2 = {
            0.16010239797412501, 0.60382926979747287, 0.72430852843857441, 0.13842814590110342, -0.24229488706619015, -0.03224486958502952,
            0.077571493840065148, -0.0062414902130117052, -0.012580751999015526, 0.0033357252850015492
        };

        hp2 = {
            0.0033357252850015492, 0.012580751999015526, -0.0062414902130117052, -0.077571493840065148, -0.03224486958502952, 0.24229488706619015,
            0.13842814590110342, -0.72430852843857441, 0.60382926979747287, -0.16010239797412501
        };
        return 0;
    }
    if (name == "db6")
    {
        lp1 = {
            -0.0010773010849955799, 0.0047772575110106514, 0.0005538422009938016, -0.031582039318031156, 0.027522865530016288, 0.097501605587079362,
            -0.12976686756709563, -0.22626469396516913, 0.3152503517092432, 0.75113390802157753, 0.49462389039838539, 0.11154074335008017
        };

        hp1 = {
            -0.11154074335008017, 0.49462389039838539, -0.75113390802157753, 0.3152503517092432, 0.22626469396516913, -0.12976686756709563,
            -0.097501605587079362, 0.027522865530016288, 0.031582039318031156, 0.0005538422009938016, -0.0047772575110106514, -0.0010773010849955799
        };

        lp2 = {
            0.11154074335008017, 0.49462389039838539, 0.75113390802157753, 0.3152503517092432, -0.22626469396516913, -0.12976686756709563, 0.097501605587079362,
            0.027522865530016288, -0.031582039318031156, 0.0005538422009938016, 0.0047772575110106514, -0.0010773010849955799
        };

        hp2 = {
            -0.0010773010849955799, -0.0047772575110106514, 0.0005538422009938016, 0.031582039318031156, 0.027522865530016288, -0.097501605587079362,
            -0.12976686756709563, 0.22626469396516913, 0.3152503517092432, -0.75113390802157753, 0.49462389039838539, -0.11154074335008017
        };
        return 0;
    }
    if (name == "db7")
    {
        lp1 = {
            0.00035371380000103988, -0.0018016407039998328, 0.00042957797300470274, 0.012550998556013784, -0.01657454163101562, -0.038029936935034633,
            0.080612609151065898, 0.071309219267050042, -0.22403618499416572, -0.14390600392910627, 0.4697822874053586, 0.72913209084655506,
            0.39653931948230575, 0.077852054085062364
        };

        hp1 = {
            -0.077852054085062364, 0.39653931948230575, -0.72913209084655506, 0.4697822874053586, 0.14390600392910627, -0.22403618499416572,
            -0.071309219267050042, 0.080612609151065898, 0.038029936935034633, -0.01657454163101562, -0.012550998556013784, 0.0004295779730047027,
            0.0018016407039998328, 0.00035371380000103988
        };

        lp2 = {
            0.077852054085062364, 0.39653931948230575, 0.72913209084655506, 0.4697822874053586, -0.14390600392910627, -0.22403618499416572,
            0.071309219267050042, 0.080612609151065898, -0.038029936935034633, -0.01657454163101562, 0.012550998556013784, 0.00042957797300470274,
            -0.0018016407039998328, 0.00035371380000103988
        };

        hp2 = {
            0.00035371380000103988, 0.0018016407039998328, 0.00042957797300470274, -0.01255099855601378, -0.01657454163101562, 0.038029936935034633,
            0.080612609151065898, -0.071309219267050042, -0.22403618499416572, 0.14390600392910627, 0.4697822874053586, -0.72913209084655506,
            0.39653931948230575, -0.077852054085062364
        };
        return 0;
    }
    if (name == "db8")
    {
        lp1 = {
            -0.00011747678400228192, 0.00067544940599855677, -0.00039174037299597711, -0.0048703529930106603, 0.0087460940470156547, 0.013981027917015516,
            -0.044088253931064719, -0.017369301002022108, 0.12874742662018601, 0.00047248457399797254, -0.28401554296242809, -0.015829105256023893,
            0.58535468365486909, 0.67563073629801285, 0.31287159091446592, 0.054415842243081609
        };

        hp1 = {
            -0.054415842243081609, 0.31287159091446592, -0.67563073629801285, 0.58535468365486909, 0.015829105256023893, -0.28401554296242809,
            -0.00047248457399797254, 0.12874742662018601, 0.017369301002022108, -0.044088253931064719, -0.013981027917015516, 0.0087460940470156547,
            0.0048703529930106603, -0.00039174037299597711, -0.00067544940599855677, -0.00011747678400228192
        };

        lp2 = {
            0.054415842243081609, 0.31287159091446592, 0.67563073629801285, 0.58535468365486909, -0.015829105256023893, -0.28401554296242809,
            0.00047248457399797254, 0.12874742662018601, -0.017369301002022108, -0.044088253931064719, 0.013981027917015516, 0.0087460940470156547,
            -0.0048703529930106603, -0.00039174037299597711, 0.00067544940599855677, -0.00011747678400228192
        };

        hp2 = {
            -0.00011747678400228192, -0.00067544940599855677, -0.00039174037299597711, 0.0048703529930106603, 0.0087460940470156547, -0.013981027917015516,
            -0.044088253931064719, 0.017369301002022108, 0.12874742662018601, -0.00047248457399797254, -0.28401554296242809, 0.015829105256023893,
            0.58535468365486909, -0.67563073629801285, 0.31287159091446592, -0.054415842243081609
        };
        return 0;
    }
    if (name == "db9")
    {
        lp1 = {
            3.9347319995026124e-05, -0.00025196318899817888, 0.00023038576399541288, 0.0018476468829611268, -0.0042815036819047227, -0.004723204757894831,
            0.022361662123515244, 0.00025094711499193845, -0.067632829059523988, 0.030725681478322865, 0.14854074933476008, -0.096840783220879037,
            -0.29327378327258685, 0.13319738582208895, 0.65728807803663891, 0.6048231236767786, 0.24383467463766728, 0.038077947363167282
        };

        hp1 = {
            -0.038077947363167282, 0.24383467463766728, -0.6048231236767786, 0.65728807803663891, -0.13319738582208895, -0.29327378327258685,
            0.096840783220879037, 0.14854074933476008, -0.030725681478322865, -0.067632829059523988, -0.00025094711499193845, 0.022361662123515244,
            0.004723204757894831, -0.0042815036819047227, -0.0018476468829611268, 0.00023038576399541288, 0.00025196318899817888, 3.9347319995026124e-05
        };

        lp2 = {
            0.038077947363167282, 0.24383467463766728, 0.6048231236767786, 0.65728807803663891, 0.13319738582208895, -0.29327378327258685,
            -0.096840783220879037, 0.14854074933476008, 0.030725681478322865, -0.067632829059523988, 0.00025094711499193845, 0.022361662123515244,
            -0.004723204757894831, -0.0042815036819047227, 0.0018476468829611268, 0.00023038576399541288, -0.00025196318899817888, 3.9347319995026124e-05
        };

        hp2 = {
            3.9347319995026124e-05, 0.00025196318899817888, 0.00023038576399541288, -0.0018476468829611268, -0.0042815036819047227, 0.004723204757894831,
            0.022361662123515244, -0.00025094711499193845, -0.067632829059523988, -0.030725681478322865, 0.14854074933476008, 0.096840783220879037,
            -0.29327378327258685, -0.13319738582208895, 0.65728807803663891, -0.6048231236767786, 0.24383467463766728, -0.038077947363167282
        };
        return 0;
    }
    if (name == "db10")
    {
        lp1 = {
            -1.3264203002354869e-05, 9.3588670001089845e-05, -0.0001164668549943862, -0.00068585669500468248, 0.0019924052949908499, 0.0013953517469940798,
            -0.010733175482979604, 0.0036065535669883944, 0.033212674058933238, -0.029457536821945671, -0.071394147165860775, 0.093057364603806592,
            0.12736934033574265, -0.19594627437659665, -0.24984642432648865, 0.28117234366042648, 0.68845903945259213, 0.52720118893091983, 0.18817680007762133,
            0.026670057900950818
        };

        hp1 = {
            -0.026670057900950818, 0.18817680007762133, -0.52720118893091983, 0.68845903945259213, -0.28117234366042648, -0.24984642432648865,
            0.19594627437659665, 0.12736934033574265, -0.093057364603806592, -0.071394147165860775, 0.029457536821945671, 0.033212674058933238,
            -0.0036065535669883944, -0.010733175482979604, -0.0013953517469940798, 0.0019924052949908499, 0.00068585669500468248, -0.0001164668549943862,
            -9.3588670001089845e-05, -1.3264203002354869e-05
        };

        lp2 = {
            0.026670057900950818, 0.18817680007762133, 0.52720118893091983, 0.68845903945259213, 0.28117234366042648, -0.24984642432648865,
            -0.19594627437659665, 0.12736934033574265, 0.093057364603806592, -0.071394147165860775, -0.029457536821945671, 0.033212674058933238,
            0.0036065535669883944, -0.010733175482979604, 0.0013953517469940798, 0.0019924052949908499, -0.00068585669500468248, -0.0001164668549943862,
            9.3588670001089845e-05, -1.3264203002354869e-05
        };

        hp2 = {
            -1.3264203002354869e-05, -9.3588670001089845e-05, -0.0001164668549943862, 0.00068585669500468248, 0.0019924052949908499, -0.0013953517469940798,
            -0.010733175482979604, -0.0036065535669883944, 0.033212674058933238, 0.029457536821945671, -0.071394147165860775, -0.093057364603806592,
            0.12736934033574265, 0.19594627437659665, -0.24984642432648865, -0.28117234366042648, 0.68845903945259213, -0.52720118893091983,
            0.18817680007762133, -0.026670057900950818
        };
        return 0;
    }
    if (name == "db12")
    {
        lp1 = {
            -1.5290717580684923e-06, 1.2776952219379579e-05, -2.4241545757030318e-05, -8.8504109208203182e-05, 0.00038865306282092672, 6.5451282125215034e-06,
            -0.0021795036186277044, 0.0022486072409952287, 0.0067114990087955486, -0.012840825198299882, -0.01221864906974642, 0.041546277495087637,
            0.010849130255828966, -0.09643212009649671, 0.0053595696743599965, 0.18247860592758275, -0.023779257256064865, -0.31617845375277914,
            -0.044763885653777619, 0.51588647842780067, 0.65719872257929113, 0.37735513521420411, 0.10956627282118277, 0.013112257957229239
        };

        hp1 = {
            -0.013112257957229239, 0.10956627282118277, -0.37735513521420411, 0.65719872257929113, -0.51588647842780067, -0.044763885653777619,
            0.31617845375277914, -0.023779257256064865, -0.18247860592758275, 0.0053595696743599965, 0.09643212009649671, 0.010849130255828966,
            -0.041546277495087637, -0.01221864906974642, 0.012840825198299882, 0.0067114990087955486, -0.0022486072409952287, -0.0021795036186277044,
            -6.5451282125215034e-06, 0.00038865306282092672, 8.8504109208203182e-05, -2.4241545757030318e-05, -1.2776952219379579e-05, -1.5290717580684923e-06
        };

        lp2 = {
            0.013112257957229239, 0.10956627282118277, 0.37735513521420411, 0.65719872257929113, 0.51588647842780067, -0.044763885653777619,
            -0.31617845375277914, -0.023779257256064865, 0.18247860592758275, 0.0053595696743599965, -0.09643212009649671, 0.010849130255828966,
            0.041546277495087637, -0.01221864906974642, -0.012840825198299882, 0.0067114990087955486, 0.0022486072409952287, -0.0021795036186277044,
            6.5451282125215034e-06, 0.00038865306282092672, -8.8504109208203182e-05, -2.4241545757030318e-05, 1.2776952219379579e-05, -1.5290717580684923e-06
        };

        hp2 = {
            -1.5290717580684923e-06, -1.2776952219379579e-05, -2.4241545757030318e-05, 8.8504109208203182e-05, 0.00038865306282092672, -6.5451282125215034e-06,
            -0.0021795036186277044, -0.0022486072409952287, 0.0067114990087955486, 0.012840825198299882, -0.01221864906974642, -0.041546277495087637,
            0.010849130255828966, 0.09643212009649671, 0.0053595696743599965, -0.18247860592758275, -0.023779257256064865, 0.31617845375277914,
            -0.044763885653777619, -0.51588647842780067, 0.65719872257929113, -0.37735513521420411, 0.10956627282118277, -0.013112257957229239
        };
        return 0;
    }
    if (name == "db13")
    {
        lp1 = {
            5.2200350984547998e-07, -4.7004164793608082e-06, 1.0441930571407941e-05, 3.0678537579324358e-05, -0.00016512898855650571, 4.9251525126285676e-05,
            0.00093232613086724904, -0.0013156739118922766, -0.002761911234656831, 0.0072555894016171187, 0.0039239414487955773, -0.023831420710327809,
            0.0023799722540522269, 0.056139477100276156, -0.026488406475345658, -0.10580761818792761, 0.072948933656788742, 0.17947607942935084,
            -0.12457673075080665, -0.31497290771138414, 0.086985726179645007, 0.58888957043121193, 0.61105585115878114, 0.31199632216043488,
            0.082861243872901946, 0.0092021335389622788
        };

        hp1 = {
            -0.0092021335389622788, 0.082861243872901946, -0.31199632216043488, 0.61105585115878114, -0.58888957043121193, 0.086985726179645007,
            0.31497290771138414, -0.12457673075080665, -0.17947607942935084, 0.072948933656788742, 0.10580761818792761, -0.026488406475345658,
            -0.056139477100276156, 0.0023799722540522269, 0.023831420710327809, 0.0039239414487955773, -0.0072555894016171187, -0.002761911234656831,
            0.0013156739118922766, 0.00093232613086724904, -4.9251525126285676e-05, -0.00016512898855650571, -3.0678537579324358e-05, 1.0441930571407941e-05,
            4.7004164793608082e-06, 5.2200350984547998e-07
        };

        lp2 = {
            0.0092021335389622788, 0.082861243872901946, 0.31199632216043488, 0.61105585115878114, 0.58888957043121193, 0.086985726179645007,
            -0.31497290771138414, -0.12457673075080665, 0.17947607942935084, 0.072948933656788742, -0.10580761818792761, -0.026488406475345658,
            0.056139477100276156, 0.0023799722540522269, -0.023831420710327809, 0.0039239414487955773, 0.0072555894016171187, -0.002761911234656831,
            -0.0013156739118922766, 0.00093232613086724904, 4.9251525126285676e-05, -0.00016512898855650571, 3.0678537579324358e-05, 1.0441930571407941e-05,
            -4.7004164793608082e-06, 5.2200350984547998e-07
        };

        hp2 = {
            5.2200350984547998e-07, 4.7004164793608082e-06, 1.0441930571407941e-05, -3.0678537579324358e-05, -0.00016512898855650571, -4.9251525126285676e-05,
            0.00093232613086724904, 0.0013156739118922766, -0.002761911234656831, -0.0072555894016171187, 0.0039239414487955773, 0.023831420710327809,
            0.0023799722540522269, -0.056139477100276156, -0.026488406475345658, 0.10580761818792761, 0.072948933656788742, -0.17947607942935084,
            -0.12457673075080665, 0.31497290771138414, 0.086985726179645007, -0.58888957043121193, 0.61105585115878114, -0.31199632216043488,
            0.082861243872901946, -0.0092021335389622788
        };
        return 0;
    }
    if (name == "db11")
    {
        lp1 = {
            4.4942742772363519e-06, -3.4634984186983789e-05, 5.4439074699366381e-05, 0.00024915252355281426, -0.00089302325066623663, -0.00030859285881515924,
            0.0049284176560587777, -0.0033408588730145018, -0.015364820906201324, 0.020840904360180039, 0.031335090219045313, -0.066438785695020222,
            -0.04647995511667613, 0.14981201246638268, 0.066043588196690886, -0.27423084681792875, -0.16227524502747828, 0.41196436894789695,
            0.68568677491617847, 0.44989976435603013, 0.14406702115061959, 0.018694297761470441
        };

        hp1 = {
            -0.018694297761470441, 0.14406702115061959, -0.44989976435603013, 0.68568677491617847, -0.41196436894789695, -0.16227524502747828,
            0.27423084681792875, 0.066043588196690886, -0.14981201246638268, -0.04647995511667613, 0.066438785695020222, 0.031335090219045313,
            -0.020840904360180039, -0.015364820906201324, 0.0033408588730145018, 0.0049284176560587777, 0.00030859285881515924, -0.00089302325066623663,
            -0.00024915252355281426, 5.4439074699366381e-05, 3.4634984186983789e-05, 4.4942742772363519e-06
        };

        lp2 = {
            0.018694297761470441, 0.14406702115061959, 0.44989976435603013, 0.68568677491617847, 0.41196436894789695, -0.16227524502747828,
            -0.27423084681792875, 0.066043588196690886, 0.14981201246638268, -0.04647995511667613, -0.066438785695020222, 0.031335090219045313,
            0.020840904360180039, -0.015364820906201324, -0.0033408588730145018, 0.0049284176560587777, -0.00030859285881515924, -0.00089302325066623663,
            0.00024915252355281426, 5.4439074699366381e-05, -3.4634984186983789e-05, 4.4942742772363519e-06
        };

        hp2 = {
            4.4942742772363519e-06, 3.4634984186983789e-05, 5.4439074699366381e-05, -0.00024915252355281426, -0.00089302325066623663, 0.00030859285881515924,
            0.0049284176560587777, 0.0033408588730145018, -0.015364820906201324, -0.020840904360180039, 0.031335090219045313, 0.066438785695020222,
            -0.04647995511667613, -0.14981201246638268, 0.066043588196690886, 0.27423084681792875, -0.16227524502747828, -0.41196436894789695,
            0.68568677491617847, -0.44989976435603013, 0.14406702115061959, -0.018694297761470441
        };
        return 0;
    }
    if (name == "db14")
    {
        lp1 = {
            -1.7871399683109222e-07, 1.7249946753674012e-06, -4.3897049017804176e-06, -1.0337209184568496e-05, 6.875504252695734e-05, -4.1777245770370672e-05,
            -0.00038683194731287514, 0.00070802115423540481, 0.001061691085606874, -0.003849638868019787, -0.00074621898926387534, 0.012789493266340071,
            -0.0056150495303375755, -0.030185351540353976, 0.026981408307947971, 0.05523712625925082, -0.071548955503983505, -0.086748411568110598,
            0.13998901658445695, 0.13839521386479153, -0.21803352999321651, -0.27168855227867705, 0.21867068775886594, 0.63118784910471981, 0.55430561794077093,
            0.25485026779256437, 0.062364758849384874, 0.0064611534600864905
        };

        hp1 = {
            -0.0064611534600864905, 0.062364758849384874, -0.25485026779256437, 0.55430561794077093, -0.63118784910471981, 0.21867068775886594,
            0.27168855227867705, -0.21803352999321651, -0.13839521386479153, 0.13998901658445695, 0.086748411568110598, -0.071548955503983505,
            -0.05523712625925082, 0.026981408307947971, 0.030185351540353976, -0.0056150495303375755, -0.012789493266340071, -0.00074621898926387534,
            0.003849638868019787, 0.001061691085606874, -0.00070802115423540481, -0.00038683194731287514, 4.1777245770370672e-05, 6.875504252695734e-05,
            1.0337209184568496e-05, -4.3897049017804176e-06, -1.7249946753674012e-06, -1.7871399683109222e-07
        };

        lp2 = {
            0.0064611534600864905, 0.062364758849384874, 0.25485026779256437, 0.55430561794077093, 0.63118784910471981, 0.21867068775886594,
            -0.27168855227867705, -0.21803352999321651, 0.13839521386479153, 0.13998901658445695, -0.086748411568110598, -0.071548955503983505,
            0.05523712625925082, 0.026981408307947971, -0.030185351540353976, -0.0056150495303375755, 0.012789493266340071, -0.00074621898926387534,
            -0.003849638868019787, 0.001061691085606874, 0.00070802115423540481, -0.00038683194731287514, -4.1777245770370672e-05, 6.875504252695734e-05,
            -1.0337209184568496e-05, -4.3897049017804176e-06, 1.7249946753674012e-06, -1.7871399683109222e-07
        };

        hp2 = {
            -1.7871399683109222e-07, -1.7249946753674012e-06, -4.3897049017804176e-06, 1.0337209184568496e-05, 6.875504252695734e-05, 4.1777245770370672e-05,
            -0.00038683194731287514, -0.00070802115423540481, 0.001061691085606874, 0.003849638868019787, -0.00074621898926387534, -0.012789493266340071,
            -0.0056150495303375755, 0.030185351540353976, 0.026981408307947971, -0.05523712625925082, -0.071548955503983505, 0.086748411568110598,
            0.13998901658445695, -0.13839521386479153, -0.21803352999321651, 0.27168855227867705, 0.21867068775886594, -0.63118784910471981,
            0.55430561794077093, -0.25485026779256437, 0.062364758849384874, -0.0064611534600864905
        };
        return 0;
    }
    if (name == "db15")
    {
        lp1 = {
            6.1333599133037138e-08, -6.3168823258794506e-07, 1.8112704079399406e-06, 3.3629871817363823e-06, -2.8133296266037558e-05, 2.579269915531323e-05,
            0.00015589648992055726, -0.00035956524436229364, -0.00037348235413726472, 0.0019433239803823459, -0.00024175649075894543, -0.0064877345603061454,
            0.0051010003604228726, 0.015083918027862582, -0.020810050169636805, -0.025767007328366939, 0.054780550584559995, 0.033877143923563204,
            -0.11112093603713753, -0.039666176555733602, 0.19014671400708816, 0.065282952848765688, -0.28888259656686216, -0.19320413960907623,
            0.33900253545462167, 0.64581314035721027, 0.49263177170797529, 0.20602386398692688, 0.046743394892750617, 0.0045385373615773762
        };

        hp1 = {
            -0.0045385373615773762, 0.046743394892750617, -0.20602386398692688, 0.49263177170797529, -0.64581314035721027, 0.33900253545462167,
            0.19320413960907623, -0.28888259656686216, -0.065282952848765688, 0.19014671400708816, 0.039666176555733602, -0.11112093603713753,
            -0.033877143923563204, 0.054780550584559995, 0.025767007328366939, -0.020810050169636805, -0.015083918027862582, 0.0051010003604228726,
            0.0064877345603061454, -0.00024175649075894543, -0.0019433239803823459, -0.00037348235413726472, 0.00035956524436229364, 0.00015589648992055726,
            -2.579269915531323e-05, -2.8133296266037558e-05, -3.3629871817363823e-06, 1.8112704079399406e-06, 6.3168823258794506e-07, 6.1333599133037138e-08
        };

        lp2 = {
            0.0045385373615773762, 0.046743394892750617, 0.20602386398692688, 0.49263177170797529, 0.64581314035721027, 0.33900253545462167,
            -0.19320413960907623, -0.28888259656686216, 0.065282952848765688, 0.19014671400708816, -0.039666176555733602, -0.11112093603713753,
            0.033877143923563204, 0.054780550584559995, -0.025767007328366939, -0.020810050169636805, 0.015083918027862582, 0.0051010003604228726,
            -0.0064877345603061454, -0.00024175649075894543, 0.0019433239803823459, -0.00037348235413726472, -0.00035956524436229364, 0.00015589648992055726,
            2.579269915531323e-05, -2.8133296266037558e-05, 3.3629871817363823e-06, 1.8112704079399406e-06, -6.3168823258794506e-07, 6.1333599133037138e-08
        };

        hp2 = {
            6.1333599133037138e-08, 6.3168823258794506e-07, 1.8112704079399406e-06, -3.3629871817363823e-06, -2.8133296266037558e-05, -2.579269915531323e-05,
            0.00015589648992055726, 0.00035956524436229364, -0.00037348235413726472, -0.0019433239803823459, -0.00024175649075894543, 0.0064877345603061454,
            0.0051010003604228726, -0.015083918027862582, -0.020810050169636805, 0.025767007328366939, 0.054780550584559995, -0.033877143923563204,
            -0.11112093603713753, 0.039666176555733602, 0.19014671400708816, -0.065282952848765688, -0.28888259656686216, 0.19320413960907623,
            0.33900253545462167, -0.64581314035721027, 0.49263177170797529, -0.20602386398692688, 0.046743394892750617, -0.0045385373615773762
        };
        return 0;
    }
    if (name == "bior1.1")
    {
        lp1 = { 0.70710678118654757, 0.70710678118654757 };
        hp1 = { -0.70710678118654757, 0.70710678118654757 };
        lp2 = { 0.70710678118654757, 0.70710678118654757 };
        hp2 = { 0.70710678118654757, -0.70710678118654757 };
        return 0;
    }
    if (name == "bior1.3")
    {
        lp1 = { -0.088388347648318447, 0.088388347648318447, 0.70710678118654757, 0.70710678118654757, 0.088388347648318447, -0.088388347648318447, };
        hp1 = { 0.0, 0.0, -0.70710678118654757, 0.70710678118654757, 0.0, 0.0 };
        lp2 = { 0.0, 0.0, 0.70710678118654757, 0.70710678118654757, 0.0, 0.0 };
        hp2 = { -0.088388347648318447, -0.088388347648318447, 0.70710678118654757, -0.70710678118654757, 0.088388347648318447, 0.088388347648318447 };
        return 0;
    }
    if (name == "bior1.5")
    {
        lp1 = {
            0.01657281518405971, -0.01657281518405971, -0.12153397801643787, 0.12153397801643787, 0.70710678118654757, 0.70710678118654757, 0.12153397801643787,
            -0.12153397801643787, -0.01657281518405971, 0.01657281518405971
        };

        hp1 = { 0.0, 0.0, 0.0, 0.0, -0.70710678118654757, 0.70710678118654757, 0.0, 0.0, 0.0, 0.0 };

        lp2 = { 0.0, 0.0, 0.0, 0.0, 0.70710678118654757, 0.70710678118654757, 0.0, 0.0, 0.0, 0.0 };

        hp2 = {
            0.01657281518405971, 0.01657281518405971, -0.12153397801643787, -0.12153397801643787, 0.70710678118654757, -0.70710678118654757,
            0.12153397801643787, 0.12153397801643787, -0.01657281518405971, -0.01657281518405971
        };
        return 0;
    }
    if (name == "bior2.2")
    {
        lp1 = { 0.0, -0.17677669529663689, 0.35355339059327379, 1.0606601717798214, 0.35355339059327379, -0.17677669529663689 };

        hp1 = { 0.0, 0.35355339059327379, -0.70710678118654757, 0.35355339059327379, 0.0, 0.0 };

        lp2 = { 0.0, 0.35355339059327379, 0.70710678118654757, 0.35355339059327379, 0.0, 0.0 };

        hp2 = { 0.0, 0.17677669529663689, 0.35355339059327379, -1.0606601717798214, 0.35355339059327379, 0.17677669529663689 };
        return 0;
    }
    if (name == "bior2.4")
    {
        lp1 = {
            0.0, 0.033145630368119419, -0.066291260736238838, -0.17677669529663689, 0.4198446513295126, 0.99436891104358249, 0.4198446513295126,
            -0.17677669529663689, -0.066291260736238838, 0.033145630368119419
        };

        hp1 = { 0.0, 0.0, 0.0, 0.35355339059327379, -0.70710678118654757, 0.35355339059327379, 0.0, 0.0, 0.0, 0.0 };

        lp2 = { 0.0, 0.0, 0.0, 0.35355339059327379, 0.70710678118654757, 0.35355339059327379, 0.0, 0.0, 0.0, 0.0 };

        hp2 = {
            0.0, -0.033145630368119419, -0.066291260736238838, 0.17677669529663689, 0.4198446513295126, -0.99436891104358249, 0.4198446513295126,
            0.17677669529663689, -0.066291260736238838, -0.033145630368119419
        };
        return 0;
    }
    if (name == "bior2.6")
    {
        lp1 = {
            0.0, -0.0069053396600248784, 0.013810679320049757, 0.046956309688169176, -0.10772329869638811, -0.16987135563661201, 0.44746600996961211,
            0.96674755240348298, 0.44746600996961211, -0.16987135563661201, -0.10772329869638811, 0.046956309688169176, 0.013810679320049757,
            -0.0069053396600248784
        };

        hp1 = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.35355339059327379, -0.70710678118654757, 0.35355339059327379, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

        lp2 = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.35355339059327379, 0.70710678118654757, 0.35355339059327379, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

        hp2 = {
            0.0, 0.0069053396600248784, 0.013810679320049757, -0.046956309688169176, -0.10772329869638811, 0.16987135563661201, 0.44746600996961211,
            -0.96674755240348298, 0.44746600996961211, 0.16987135563661201, -0.10772329869638811, -0.046956309688169176, 0.013810679320049757,
            0.0069053396600248784
        };
        return 0;
    }
    if (name == "bior2.8")
    {
        lp1 = {
            0.0, 0.0015105430506304422, -0.0030210861012608843, -0.012947511862546647, 0.028916109826354178, 0.052998481890690945, -0.13491307360773608,
            -0.16382918343409025, 0.46257144047591658, 0.95164212189717856, 0.46257144047591658, -0.16382918343409025, -0.13491307360773608,
            0.052998481890690945, 0.028916109826354178, -0.012947511862546647, -0.0030210861012608843, 0.0015105430506304422
        };

        hp1 = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.35355339059327379, -0.70710678118654757, 0.35355339059327379, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

        lp2 = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.35355339059327379, 0.70710678118654757, 0.35355339059327379, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

        hp2 = {
            0.0, -0.0015105430506304422, -0.0030210861012608843, 0.012947511862546647, 0.028916109826354178, -0.052998481890690945, -0.13491307360773608,
            0.16382918343409025, 0.46257144047591658, -0.95164212189717856, 0.46257144047591658, 0.16382918343409025, -0.13491307360773608,
            -0.052998481890690945, 0.028916109826354178, 0.012947511862546647, -0.0030210861012608843, -0.0015105430506304422
        };
        return 0;
    }
    if (name == "bior3.1")
    {
        lp1 = { -0.35355339059327379, 1.0606601717798214, 1.0606601717798214, -0.35355339059327379 };

        hp1 = { -0.17677669529663689, 0.53033008588991071, -0.53033008588991071, 0.17677669529663689 };

        lp2 = { 0.17677669529663689, 0.53033008588991071, 0.53033008588991071, 0.17677669529663689 };

        hp2 = { -0.35355339059327379, -1.0606601717798214, 1.0606601717798214, 0.35355339059327379 };
        return 0;
    }
    if (name == "bior3.3")
    {
        lp1 = {
            0.066291260736238838, -0.19887378220871652, -0.15467960838455727, 0.99436891104358249, 0.99436891104358249, -0.15467960838455727,
            -0.19887378220871652, 0.066291260736238838
        };

        hp1 = { 0.0, 0.0, -0.17677669529663689, 0.53033008588991071, -0.53033008588991071, 0.17677669529663689, 0.0, 0.0 };

        lp2 = { 0.0, 0.0, 0.17677669529663689, 0.53033008588991071, 0.53033008588991071, 0.17677669529663689, 0.0, 0.0 };

        hp2 = {
            0.066291260736238838, 0.19887378220871652, -0.15467960838455727, -0.99436891104358249, 0.99436891104358249, 0.15467960838455727,
            -0.19887378220871652, -0.066291260736238838
        };
        return 0;
    }
    if (name == "bior3.5")
    {
        lp1 = {
            -0.013810679320049757, 0.041432037960149271, 0.052480581416189075, -0.26792717880896527, -0.071815532464258744, 0.96674755240348298,
            0.96674755240348298, -0.071815532464258744, -0.26792717880896527, 0.052480581416189075, 0.041432037960149271, -0.013810679320049757
        };

        hp1 = { 0.0, 0.0, 0.0, 0.0, -0.17677669529663689, 0.53033008588991071, -0.53033008588991071, 0.17677669529663689, 0.0, 0.0, 0.0, 0.0 };

        lp2 = { 0.0, 0.0, 0.0, 0.0, 0.17677669529663689, 0.53033008588991071, 0.53033008588991071, 0.17677669529663689, 0.0, 0.0, 0.0, 0.0 };

        hp2 = {
            -0.013810679320049757, -0.041432037960149271, 0.052480581416189075, 0.26792717880896527, -0.071815532464258744, -0.96674755240348298,
            0.96674755240348298, 0.071815532464258744, -0.26792717880896527, -0.052480581416189075, 0.041432037960149271, 0.013810679320049757
        };
        return 0;
    }
    if (name == "bior3.7")
    {
        lp1 = {
            0.0030210861012608843, -0.0090632583037826529, -0.016831765421310641, 0.074663985074019001, 0.031332978707362888, -0.301159125922835,
            -0.026499240945345472, 0.95164212189717856, 0.95164212189717856, -0.026499240945345472, -0.301159125922835, 0.031332978707362888,
            0.074663985074019001, -0.016831765421310641, -0.0090632583037826529, 0.0030210861012608843
        };

        hp1 = {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.17677669529663689, 0.53033008588991071, -0.53033008588991071, 0.17677669529663689, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        };

        lp2 = {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.17677669529663689, 0.53033008588991071, 0.53033008588991071, 0.17677669529663689, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        };

        hp2 = {
            0.0030210861012608843, 0.0090632583037826529, -0.016831765421310641, -0.074663985074019001, 0.031332978707362888, 0.301159125922835,
            -0.026499240945345472, -0.95164212189717856, 0.95164212189717856, 0.026499240945345472, -0.301159125922835, -0.031332978707362888,
            0.074663985074019001, 0.016831765421310641, -0.0090632583037826529, -0.0030210861012608843
        };
        return 0;
    }
    if (name == "bior3.9")
    {
        lp1 = {
            -0.00067974437278369901, 0.0020392331183510968, 0.0050603192196119811, -0.020618912641105536, -0.014112787930175846, 0.09913478249423216,
            0.012300136269419315, -0.32019196836077857, 0.0020500227115698858, 0.94212570067820678, 0.94212570067820678, 0.0020500227115698858,
            -0.32019196836077857, 0.012300136269419315, 0.09913478249423216, -0.014112787930175846, -0.020618912641105536, 0.0050603192196119811,
            0.0020392331183510968, -0.00067974437278369901
        };

        hp1 = {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.17677669529663689, 0.53033008588991071, -0.53033008588991071, 0.17677669529663689, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0
        };

        lp2 = {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.17677669529663689, 0.53033008588991071, 0.53033008588991071, 0.17677669529663689, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0
        };

        hp2 = {
            -0.00067974437278369901, -0.0020392331183510968, 0.0050603192196119811, 0.020618912641105536, -0.014112787930175846, -0.09913478249423216,
            0.012300136269419315, 0.32019196836077857, 0.0020500227115698858, -0.94212570067820678, 0.94212570067820678, -0.0020500227115698858,
            -0.32019196836077857, -0.012300136269419315, 0.09913478249423216, 0.014112787930175846, -0.020618912641105536, -0.0050603192196119811,
            0.0020392331183510968, 0.00067974437278369901
        };
        return 0;
    }
    if (name == "bior4.4")
    {
        lp1 = {
            0.0, 0.03782845550726404, -0.023849465019556843, -0.11062440441843718, 0.37740285561283066, 0.85269867900889385, 0.37740285561283066,
            -0.11062440441843718, -0.023849465019556843, 0.03782845550726404
        };

        hp1 = {
            0.0, -0.064538882628697058, 0.040689417609164058, 0.41809227322161724, -0.7884856164055829, 0.41809227322161724, 0.040689417609164058,
            -0.064538882628697058, 0.0, 0.0
        };

        lp2 = {
            0.0, -0.064538882628697058, -0.040689417609164058, 0.41809227322161724, 0.7884856164055829, 0.41809227322161724, -0.040689417609164058,
            -0.064538882628697058, 0.0, 0.0
        };

        hp2 = {
            0.0, -0.03782845550726404, -0.023849465019556843, 0.11062440441843718, 0.37740285561283066, -0.85269867900889385, 0.37740285561283066,
            0.11062440441843718, -0.023849465019556843, -0.03782845550726404
        };
        return 0;
    }
    if (name == "bior5.5")
    {
        lp1 = {
            0.0, 0.0, 0.03968708834740544, 0.0079481086372403219, -0.054463788468236907, 0.34560528195603346, 0.73666018142821055, 0.34560528195603346,
            -0.054463788468236907, 0.0079481086372403219, 0.03968708834740544, 0.0
        };

        hp1 = {
            -0.013456709459118716, -0.0026949668801115071, 0.13670658466432914, -0.093504697400938863, -0.47680326579848425, 0.89950610974864842,
            -0.47680326579848425, -0.093504697400938863, 0.13670658466432914, -0.0026949668801115071, -0.013456709459118716, 0.0
        };

        lp2 = {
            0.013456709459118716, -0.0026949668801115071, -0.13670658466432914, -0.093504697400938863, 0.47680326579848425, 0.89950610974864842,
            0.47680326579848425, -0.093504697400938863, -0.13670658466432914, -0.0026949668801115071, 0.013456709459118716, 0.0
        };

        hp2 = {
            0.0, 0.0, 0.03968708834740544, -0.0079481086372403219, -0.054463788468236907, -0.34560528195603346, 0.73666018142821055, -0.34560528195603346,
            -0.054463788468236907, -0.0079481086372403219, 0.03968708834740544, 0.0
        };
        return 0;
    }
    if (name == "bior6.8")
    {
        lp1 = {
            0.0, 0.0019088317364812906, -0.0019142861290887667, -0.016990639867602342, 0.01193456527972926, 0.04973290349094079, -0.077263173167204144,
            -0.09405920349573646, 0.42079628460982682, 0.82592299745840225, 0.42079628460982682, -0.09405920349573646, -0.077263173167204144,
            0.04973290349094079, 0.01193456527972926, -0.016990639867602342, -0.0019142861290887667, 0.0019088317364812906
        };

        hp1 = {
            0.0, 0.0, 0.0, 0.014426282505624435, -0.014467504896790148, -0.078722001062628819, 0.040367979030339923, 0.41784910915027457, -0.75890772945365415,
            0.41784910915027457, 0.040367979030339923, -0.078722001062628819, -0.014467504896790148, 0.014426282505624435, 0.0, 0.0, 0.0, 0.0
        };

        lp2 = {
            0.0, 0.0, 0.0, 0.014426282505624435, 0.014467504896790148, -0.078722001062628819, -0.040367979030339923, 0.41784910915027457, 0.75890772945365415,
            0.41784910915027457, -0.040367979030339923, -0.078722001062628819, 0.014467504896790148, 0.014426282505624435, 0.0, 0.0, 0.0, 0.0
        };

        hp2 = {
            0.0, -0.0019088317364812906, -0.0019142861290887667, 0.016990639867602342, 0.01193456527972926, -0.04973290349094079, -0.077263173167204144,
            0.09405920349573646, 0.42079628460982682, -0.82592299745840225, 0.42079628460982682, 0.09405920349573646, -0.077263173167204144,
            -0.04973290349094079, 0.01193456527972926, 0.016990639867602342, -0.0019142861290887667, -0.0019088317364812906
        };
        return 0;
    }
    if (name == "coif1")
    {
        lp1 = { -0.01565572813546454, -0.072732619512853897, 0.38486484686420286, 0.85257202021225542, 0.33789766245780922, -0.072732619512853897 };

        hp1 = { 0.072732619512853897, 0.33789766245780922, -0.85257202021225542, 0.38486484686420286, 0.072732619512853897, -0.01565572813546454 };

        lp2 = { -0.072732619512853897, 0.33789766245780922, 0.85257202021225542, 0.38486484686420286, -0.072732619512853897, -0.01565572813546454 };

        hp2 = { -0.01565572813546454, 0.072732619512853897, 0.38486484686420286, -0.85257202021225542, 0.33789766245780922, 0.072732619512853897 };
        return 0;
    }
    if (name == "coif2")
    {
        lp1 = {
            -0.00072054944536451221, -0.0018232088707029932, 0.0056114348193944995, 0.023680171946334084, -0.059434418646456898, -0.076488599078306393,
            0.41700518442169254, 0.81272363544554227, 0.38611006682116222, -0.067372554721963018, -0.041464936781759151, 0.016387336463522112
        };

        hp1 = {
            -0.016387336463522112, -0.041464936781759151, 0.067372554721963018, 0.38611006682116222, -0.81272363544554227, 0.41700518442169254,
            0.076488599078306393, -0.059434418646456898, -0.023680171946334084, 0.0056114348193944995, 0.0018232088707029932, -0.00072054944536451221
        };

        lp2 = {
            0.016387336463522112, -0.041464936781759151, -0.067372554721963018, 0.38611006682116222, 0.81272363544554227, 0.41700518442169254,
            -0.076488599078306393, -0.059434418646456898, 0.023680171946334084, 0.0056114348193944995, -0.0018232088707029932, -0.00072054944536451221
        };

        hp2 = {
            -0.00072054944536451221, 0.0018232088707029932, 0.0056114348193944995, -0.023680171946334084, -0.059434418646456898, 0.076488599078306393,
            0.41700518442169254, -0.81272363544554227, 0.38611006682116222, 0.067372554721963018, -0.041464936781759151, -0.016387336463522112
        };
        return 0;
    }
    if (name == "coif3")
    {
        lp1 = {
            -3.4599772836212559e-05, -7.0983303138141252e-05, 0.00046621696011288631, 0.0011175187708906016, -0.0025745176887502236, -0.0090079761366615805,
            0.015880544863615904, 0.034555027573061628, -0.082301927106885983, -0.071799821619312018, 0.42848347637761874, 0.79377722262562056,
            0.4051769024096169, -0.061123390002672869, -0.0657719112818555, 0.023452696141836267, 0.0077825964273254182, -0.0037935128644910141
        };

        hp1 = {
            0.0037935128644910141, 0.0077825964273254182, -0.023452696141836267, -0.0657719112818555, 0.061123390002672869, 0.4051769024096169,
            -0.79377722262562056, 0.42848347637761874, 0.071799821619312018, -0.082301927106885983, -0.034555027573061628, 0.015880544863615904,
            0.0090079761366615805, -0.0025745176887502236, -0.0011175187708906016, 0.00046621696011288631, 7.0983303138141252e-05, -3.4599772836212559e-05
        };

        lp2 = {
            -0.0037935128644910141, 0.0077825964273254182, 0.023452696141836267, -0.0657719112818555, -0.061123390002672869, 0.4051769024096169,
            0.79377722262562056, 0.42848347637761874, -0.071799821619312018, -0.082301927106885983, 0.034555027573061628, 0.015880544863615904,
            -0.0090079761366615805, -0.0025745176887502236, 0.0011175187708906016, 0.00046621696011288631, -7.0983303138141252e-05, -3.4599772836212559e-05
        };

        hp2 = {
            -3.4599772836212559e-05, 7.0983303138141252e-05, 0.00046621696011288631, -0.0011175187708906016, -0.0025745176887502236, 0.0090079761366615805,
            0.015880544863615904, -0.034555027573061628, -0.082301927106885983, 0.071799821619312018, 0.42848347637761874, -0.79377722262562056,
            0.4051769024096169, 0.061123390002672869, -0.0657719112818555, -0.023452696141836267, 0.0077825964273254182, 0.0037935128644910141
        };
        return 0;
    }
    if (name == "coif4")
    {
        lp1 = {
            -1.7849850030882614e-06, -3.2596802368833675e-06, 3.1229875865345646e-05, 6.2339034461007128e-05, -0.00025997455248771324, -0.00058902075624433831,
            0.0012665619292989445, 0.0037514361572784571, -0.0056582866866107199, -0.015211731527946259, 0.025082261844864097, 0.039334427123337491,
            -0.096220442033987982, -0.066627474263425038, 0.4343860564914685, 0.78223893092049901, 0.41530840703043026, -0.056077313316754807,
            -0.081266699680878754, 0.026682300156053072, 0.016068943964776348, -0.0073461663276420935, -0.0016294920126017326, 0.00089231366858231456
        };

        hp1 = {
            -0.00089231366858231456, -0.0016294920126017326, 0.0073461663276420935, 0.016068943964776348, -0.026682300156053072, -0.081266699680878754,
            0.056077313316754807, 0.41530840703043026, -0.78223893092049901, 0.4343860564914685, 0.066627474263425038, -0.096220442033987982,
            -0.039334427123337491, 0.025082261844864097, 0.015211731527946259, -0.0056582866866107199, -0.0037514361572784571, 0.0012665619292989445,
            0.00058902075624433831, -0.00025997455248771324, -6.2339034461007128e-05, 3.1229875865345646e-05, 3.2596802368833675e-06, -1.7849850030882614e-06
        };

        lp2 = {
            0.00089231366858231456, -0.0016294920126017326, -0.0073461663276420935, 0.016068943964776348, 0.026682300156053072, -0.081266699680878754,
            -0.056077313316754807, 0.41530840703043026, 0.78223893092049901, 0.4343860564914685, -0.066627474263425038, -0.096220442033987982,
            0.039334427123337491, 0.025082261844864097, -0.015211731527946259, -0.0056582866866107199, 0.0037514361572784571, 0.0012665619292989445,
            -0.00058902075624433831, -0.00025997455248771324, 6.2339034461007128e-05, 3.1229875865345646e-05, -3.2596802368833675e-06, -1.7849850030882614e-06
        };

        hp2 = {
            -1.7849850030882614e-06, 3.2596802368833675e-06, 3.1229875865345646e-05, -6.2339034461007128e-05, -0.00025997455248771324, 0.00058902075624433831,
            0.0012665619292989445, -0.0037514361572784571, -0.0056582866866107199, 0.015211731527946259, 0.025082261844864097, -0.039334427123337491,
            -0.096220442033987982, 0.066627474263425038, 0.4343860564914685, -0.78223893092049901, 0.41530840703043026, 0.056077313316754807,
            -0.081266699680878754, -0.026682300156053072, 0.016068943964776348, 0.0073461663276420935, -0.0016294920126017326, -0.00089231366858231456
        };
        return 0;
    }
    if (name == "coif5")
    {
        lp1 = {
            -9.517657273819165e-08, -1.6744288576823017e-07, 2.0637618513646814e-06, 3.7346551751414047e-06, -2.1315026809955787e-05, -4.1340432272512511e-05,
            0.00014054114970203437, 0.00030225958181306315, -0.00063813134304511142, -0.0016628637020130838, 0.0024333732126576722, 0.0067641854480530832,
            -0.0091642311624818458, -0.019761778942572639, 0.032683574267111833, 0.041289208750181702, -0.10557420870333893, -0.062035963962903569,
            0.43799162617183712, 0.77428960365295618, 0.42156620669085149, -0.052043163176243773, -0.091920010559696244, 0.02816802897093635,
            0.023408156785839195, -0.010131117519849788, -0.004159358781386048, 0.0021782363581090178, 0.00035858968789573785, -0.00021208083980379827
        };

        hp1 = {
            0.00021208083980379827, 0.00035858968789573785, -0.0021782363581090178, -0.004159358781386048, 0.010131117519849788, 0.023408156785839195,
            -0.02816802897093635, -0.091920010559696244, 0.052043163176243773, 0.42156620669085149, -0.77428960365295618, 0.43799162617183712,
            0.062035963962903569, -0.10557420870333893, -0.041289208750181702, 0.032683574267111833, 0.019761778942572639, -0.0091642311624818458,
            -0.0067641854480530832, 0.0024333732126576722, 0.0016628637020130838, -0.00063813134304511142, -0.00030225958181306315, 0.00014054114970203437,
            4.1340432272512511e-05, -2.1315026809955787e-05, -3.7346551751414047e-06, 2.0637618513646814e-06, 1.6744288576823017e-07, -9.517657273819165e-08
        };

        lp2 = {
            -0.00021208083980379827, 0.00035858968789573785, 0.0021782363581090178, -0.004159358781386048, -0.010131117519849788, 0.023408156785839195,
            0.02816802897093635, -0.091920010559696244, -0.052043163176243773, 0.42156620669085149, 0.77428960365295618, 0.43799162617183712,
            -0.062035963962903569, -0.10557420870333893, 0.041289208750181702, 0.032683574267111833, -0.019761778942572639, -0.0091642311624818458,
            0.0067641854480530832, 0.0024333732126576722, -0.0016628637020130838, -0.00063813134304511142, 0.00030225958181306315, 0.00014054114970203437,
            -4.1340432272512511e-05, -2.1315026809955787e-05, 3.7346551751414047e-06, 2.0637618513646814e-06, -1.6744288576823017e-07, -9.517657273819165e-08
        };

        hp2 = {
            -9.517657273819165e-08, 1.6744288576823017e-07, 2.0637618513646814e-06, -3.7346551751414047e-06, -2.1315026809955787e-05, 4.1340432272512511e-05,
            0.00014054114970203437, -0.00030225958181306315, -0.00063813134304511142, 0.0016628637020130838, 0.0024333732126576722, -0.0067641854480530832,
            -0.0091642311624818458, 0.019761778942572639, 0.032683574267111833, -0.041289208750181702, -0.10557420870333893, 0.062035963962903569,
            0.43799162617183712, -0.77428960365295618, 0.42156620669085149, 0.052043163176243773, -0.091920010559696244, -0.02816802897093635,
            0.023408156785839195, 0.010131117519849788, -0.004159358781386048, -0.0021782363581090178, 0.00035858968789573785, 0.00021208083980379827
        };
        return 0;
    }
    if (name == "sym2")
    {
        lp1 = { -0.12940952255092145, 0.22414386804185735, 0.83651630373746899, 0.48296291314469025 };

        hp1 = { -0.48296291314469025, 0.83651630373746899, -0.22414386804185735, -0.12940952255092145 };

        lp2 = { 0.48296291314469025, 0.83651630373746899, 0.22414386804185735, -0.12940952255092145 };

        hp2 = { -0.12940952255092145, -0.22414386804185735, 0.83651630373746899, -0.48296291314469025 };
        return 0;
    }
    if (name == "sym3")
    {
        lp1 = { 0.035226291882100656, -0.085441273882241486, -0.13501102001039084, 0.45987750211933132, 0.80689150931333875, 0.33267055295095688 };

        hp1 = { -0.33267055295095688, 0.80689150931333875, -0.45987750211933132, -0.13501102001039084, 0.085441273882241486, 0.035226291882100656 };

        lp2 = { 0.33267055295095688, 0.80689150931333875, 0.45987750211933132, -0.13501102001039084, -0.085441273882241486, 0.035226291882100656 };

        hp2 = { 0.035226291882100656, 0.085441273882241486, -0.13501102001039084, -0.45987750211933132, 0.80689150931333875, -0.33267055295095688 };
        return 0;
    }
    if (name == "sym4")
    {
        lp1 = {
            -0.075765714789273325, -0.02963552764599851, 0.49761866763201545, 0.80373875180591614, 0.29785779560527736, -0.099219543576847216,
            -0.012603967262037833, 0.032223100604042702
        };

        hp1 = {
            -0.032223100604042702, -0.012603967262037833, 0.099219543576847216, 0.29785779560527736, -0.80373875180591614, 0.49761866763201545,
            0.02963552764599851, -0.075765714789273325
        };

        lp2 = {
            0.032223100604042702, -0.012603967262037833, -0.099219543576847216, 0.29785779560527736, 0.80373875180591614, 0.49761866763201545,
            -0.02963552764599851, -0.075765714789273325
        };

        hp2 = {
            -0.075765714789273325, 0.02963552764599851, 0.49761866763201545, -0.80373875180591614, 0.29785779560527736, 0.099219543576847216,
            -0.012603967262037833, -0.032223100604042702
        };
        return 0;
    }
    if (name == "sym5")
    {
        lp1 = {
            0.027333068345077982, 0.029519490925774643, -0.039134249302383094, 0.1993975339773936, 0.72340769040242059, 0.63397896345821192,
            0.016602105764522319, -0.17532808990845047, -0.021101834024758855, 0.019538882735286728
        };

        hp1 = {
            -0.019538882735286728, -0.021101834024758855, 0.17532808990845047, 0.016602105764522319, -0.63397896345821192, 0.72340769040242059,
            -0.1993975339773936, -0.039134249302383094, -0.029519490925774643, 0.027333068345077982
        };

        lp2 = {
            0.019538882735286728, -0.021101834024758855, -0.17532808990845047, 0.016602105764522319, 0.63397896345821192, 0.72340769040242059,
            0.1993975339773936, -0.039134249302383094, 0.029519490925774643, 0.027333068345077982
        };

        hp2 = {
            0.027333068345077982, -0.029519490925774643, -0.039134249302383094, -0.1993975339773936, 0.72340769040242059, -0.63397896345821192,
            0.016602105764522319, 0.17532808990845047, -0.021101834024758855, -0.019538882735286728
        };
        return 0;
    }
    if (name == "sym6")
    {
        lp1 = {
            0.015404109327027373, 0.0034907120842174702, -0.11799011114819057, -0.048311742585632998, 0.49105594192674662, 0.787641141030194,
            0.3379294217276218, -0.072637522786462516, -0.021060292512300564, 0.044724901770665779, 0.0017677118642428036, -0.007800708325034148
        };

        hp1 = {
            0.007800708325034148, 0.0017677118642428036, -0.044724901770665779, -0.021060292512300564, 0.072637522786462516, 0.3379294217276218,
            -0.787641141030194, 0.49105594192674662, 0.048311742585632998, -0.11799011114819057, -0.0034907120842174702, 0.015404109327027373
        };

        lp2 = {
            -0.007800708325034148, 0.0017677118642428036, 0.044724901770665779, -0.021060292512300564, -0.072637522786462516, 0.3379294217276218,
            0.787641141030194, 0.49105594192674662, -0.048311742585632998, -0.11799011114819057, 0.0034907120842174702, 0.015404109327027373
        };

        hp2 = {
            0.015404109327027373, -0.0034907120842174702, -0.11799011114819057, 0.048311742585632998, 0.49105594192674662, -0.787641141030194,
            0.3379294217276218, 0.072637522786462516, -0.021060292512300564, -0.044724901770665779, 0.0017677118642428036, 0.007800708325034148
        };
        return 0;
    }
    if (name == "sym7")
    {
        lp1 = {
            0.0026818145682578781, -0.0010473848886829163, -0.01263630340325193, 0.03051551316596357, 0.067892693501372697, -0.049552834937127255,
            0.017441255086855827, 0.5361019170917628, 0.76776431700316405, 0.28862963175151463, -0.14004724044296152, -0.10780823770381774,
            0.0040102448715336634, 0.010268176708511255
        };

        hp1 = {
            -0.010268176708511255, 0.0040102448715336634, 0.10780823770381774, -0.14004724044296152, -0.28862963175151463, 0.76776431700316405,
            -0.5361019170917628, 0.017441255086855827, 0.049552834937127255, 0.067892693501372697, -0.03051551316596357, -0.01263630340325193,
            0.0010473848886829163, 0.0026818145682578781
        };

        lp2 = {
            0.010268176708511255, 0.0040102448715336634, -0.10780823770381774, -0.14004724044296152, 0.28862963175151463, 0.76776431700316405,
            0.5361019170917628, 0.017441255086855827, -0.049552834937127255, 0.067892693501372697, 0.03051551316596357, -0.01263630340325193,
            -0.0010473848886829163, 0.0026818145682578781
        };

        hp2 = {
            0.0026818145682578781, 0.0010473848886829163, -0.01263630340325193, -0.03051551316596357, 0.067892693501372697, 0.049552834937127255,
            0.017441255086855827, -0.5361019170917628, 0.76776431700316405, -0.28862963175151463, -0.14004724044296152, 0.10780823770381774,
            0.0040102448715336634, -0.010268176708511255
        };
        return 0;
    }
    if (name == "sym8")
    {
        lp1 = {
            -0.0033824159510061256, -0.00054213233179114812, 0.031695087811492981, 0.0076074873249176054, -0.14329423835080971, -0.061273359067658524,
            0.48135965125837221, 0.77718575170052351, 0.3644418948353314, -0.051945838107709037, -0.027219029917056003, 0.049137179673607506,
            0.0038087520138906151, -0.014952258337048231, -0.0003029205147213668, 0.0018899503327594609
        };

        hp1 = {
            -0.0018899503327594609, -0.0003029205147213668, 0.014952258337048231, 0.0038087520138906151, -0.049137179673607506, -0.027219029917056003,
            0.051945838107709037, 0.3644418948353314, -0.77718575170052351, 0.48135965125837221, 0.061273359067658524, -0.14329423835080971,
            -0.0076074873249176054, 0.031695087811492981, 0.00054213233179114812, -0.0033824159510061256
        };

        lp2 = {
            0.0018899503327594609, -0.0003029205147213668, -0.014952258337048231, 0.0038087520138906151, 0.049137179673607506, -0.027219029917056003,
            -0.051945838107709037, 0.3644418948353314, 0.77718575170052351, 0.48135965125837221, -0.061273359067658524, -0.14329423835080971,
            0.0076074873249176054, 0.031695087811492981, -0.00054213233179114812, -0.0033824159510061256
        };

        hp2 = {
            -0.0033824159510061256, 0.00054213233179114812, 0.031695087811492981, -0.0076074873249176054, -0.14329423835080971, 0.061273359067658524,
            0.48135965125837221, -0.77718575170052351, 0.3644418948353314, 0.051945838107709037, -0.027219029917056003, -0.049137179673607506,
            0.0038087520138906151, 0.014952258337048231, -0.0003029205147213668, -0.0018899503327594609
        };
        return 0;
    }
    if (name == "sym9")
    {
        lp1 = {
            0.0014009155259146807, 0.00061978088898558676, -0.013271967781817119, -0.01152821020767923, 0.03022487885827568, 0.00058346274612580684,
            -0.054568958430834071, 0.238760914607303, 0.717897082764412, 0.61733844914093583, 0.035272488035271894, -0.19155083129728512, -0.018233770779395985,
            0.06207778930288603, 0.0088592674934004842, -0.010264064027633142, -0.00047315449868008311, 0.0010694900329086053
        };

        hp1 = {
            -0.0010694900329086053, -0.00047315449868008311, 0.010264064027633142, 0.0088592674934004842, -0.06207778930288603, -0.018233770779395985,
            0.19155083129728512, 0.035272488035271894, -0.61733844914093583, 0.717897082764412, -0.238760914607303, -0.054568958430834071,
            -0.00058346274612580684, 0.03022487885827568, 0.01152821020767923, -0.013271967781817119, -0.00061978088898558676, 0.0014009155259146807
        };

        lp2 = {
            0.0010694900329086053, -0.00047315449868008311, -0.010264064027633142, 0.0088592674934004842, 0.06207778930288603, -0.018233770779395985,
            -0.19155083129728512, 0.035272488035271894, 0.61733844914093583, 0.717897082764412, 0.238760914607303, -0.054568958430834071,
            0.00058346274612580684, 0.03022487885827568, -0.01152821020767923, -0.013271967781817119, 0.00061978088898558676, 0.0014009155259146807
        };

        hp2 = {
            0.0014009155259146807, -0.00061978088898558676, -0.013271967781817119, 0.01152821020767923, 0.03022487885827568, -0.00058346274612580684,
            -0.054568958430834071, -0.238760914607303, 0.717897082764412, -0.61733844914093583, 0.035272488035271894, 0.19155083129728512,
            -0.018233770779395985, -0.06207778930288603, 0.0088592674934004842, 0.010264064027633142, -0.00047315449868008311, -0.0010694900329086053
        };
        return 0;
    }
    if (name == "sym10")
    {
        lp1 = {
            0.00077015980911449011, 9.5632670722894754e-05, -0.0086412992770224222, -0.0014653825813050513, 0.045927239231092203, 0.011609893903711381,
            -0.15949427888491757, -0.070880535783243853, 0.47169066693843925, 0.7695100370211071, 0.38382676106708546, -0.035536740473817552,
            -0.0319900568824278, 0.049994972077376687, 0.0057649120335819086, -0.02035493981231129, -0.00080435893201654491, 0.0045931735853118284,
            5.7036083618494284e-05, -0.00045932942100465878
        };

        hp1 = {
            0.00045932942100465878, 5.7036083618494284e-05, -0.0045931735853118284, -0.00080435893201654491, 0.02035493981231129, 0.0057649120335819086,
            -0.049994972077376687, -0.0319900568824278, 0.035536740473817552, 0.38382676106708546, -0.7695100370211071, 0.47169066693843925,
            0.070880535783243853, -0.15949427888491757, -0.011609893903711381, 0.045927239231092203, 0.0014653825813050513, -0.0086412992770224222,
            -9.5632670722894754e-05, 0.00077015980911449011
        };

        lp2 = {
            -0.00045932942100465878, 5.7036083618494284e-05, 0.0045931735853118284, -0.00080435893201654491, -0.02035493981231129, 0.0057649120335819086,
            0.049994972077376687, -0.0319900568824278, -0.035536740473817552, 0.38382676106708546, 0.7695100370211071, 0.47169066693843925,
            -0.070880535783243853, -0.15949427888491757, 0.011609893903711381, 0.045927239231092203, -0.0014653825813050513, -0.0086412992770224222,
            9.5632670722894754e-05, 0.00077015980911449011
        };

        hp2 = {
            0.00077015980911449011, -9.5632670722894754e-05, -0.0086412992770224222, 0.0014653825813050513, 0.045927239231092203, -0.011609893903711381,
            -0.15949427888491757, 0.070880535783243853, 0.47169066693843925, -0.7695100370211071, 0.38382676106708546, 0.035536740473817552,
            -0.0319900568824278, -0.049994972077376687, 0.0057649120335819086, 0.02035493981231129, -0.00080435893201654491, -0.0045931735853118284,
            5.7036083618494284e-05, 0.00045932942100465878
        };
        return 0;
    }
    std::cout << "Filter Not in Database" << std::endl;
    return -1;
}

#endif
