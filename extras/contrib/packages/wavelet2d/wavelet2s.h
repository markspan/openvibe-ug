#pragma once

#if defined(TARGET_HAS_ThirdPartyFFTW3)

#include <vector>
#include <complex>

// 1D Functions
// void* dwt(std::vector<double> &, int ,std::string , std::vector<double> &,  std::vector<double> &);
void* dwt1(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD);
void* dyadic_zpad_1d(std::vector<double>& signal);
double convol(std::vector<double>& a1, std::vector<double>& b1, std::vector<double>& c);
int filtcoef(const std::string& name, std::vector<double>& lp1, std::vector<double>& hp1, std::vector<double>& lp2, std::vector<double>& hp2);
void downsamp(std::vector<double>& sig, int m, std::vector<double>& sigD);
void upsamp(std::vector<double>& sig, int m, std::vector<double>& sigU);
void circshift(std::vector<double>& sigCir, int l);
int sign(int x);
void* idwt1(const std::string& wname, std::vector<double>& X, std::vector<double>& cA, std::vector<double>& cD);
int vecsum(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c);

// 1D Symmetric Extension DWT Functions
void* dwt_sym(std::vector<double>& signal, int J, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag,
              std::vector<size_t>& length);
void* dwt1_sym(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD);
void* idwt_sym(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<double>& idwtOutput, std::vector<size_t>& length);
void* symm_ext(std::vector<double>& sig, int a);
void* idwt1_sym(const std::string& wname, std::vector<double>& x, std::vector<double>& app, std::vector<double>& detail); // Not Tested

// 1D Stationary Wavelet Transform
void* swt(std::vector<double>& signal1, int J, const std::string& nm, std::vector<double>& swtOutput, int& length);
void* iswt(std::vector<double>& swtop, int J, const std::string& nm, std::vector<double>& iswtOutput);
void* per_ext(std::vector<double>& sig, int a);

// 2D Functions
void* branch_lp_dn(const std::string& wname, std::vector<double>& signal, std::vector<double>& sigop);
void* branch_hp_dn(const std::string& wname, std::vector<double>& signal, std::vector<double>& sigop);
void* branch_lp_hp_up(const std::string& wname, std::vector<double>& cA, std::vector<double>& cD, std::vector<double>& x);
// void* dwt_2d(std::vector<std::vector<double> > &, int , std::string , std::vector<std::vector<double> > &, std::vector<double> &) ;
// void* idwt_2d(std::vector<std::vector<double> > &,std::vector<double> &, std::string ,std::vector<std::vector<double> > &);
void* dyadic_zpad_2d(std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& mod);
void* dwt_output_dim(std::vector<std::vector<double>>& signal, int& r, int& c);
void* zero_remove(std::vector<std::vector<double>>& input, std::vector<std::vector<double>>& output);
void* getcoeff2d(std::vector<std::vector<double>>& dwtoutput, std::vector<std::vector<double>>& cH, std::vector<std::vector<double>>& cV,
                 std::vector<std::vector<double>>& cD, std::vector<double>& flag, int& n);
void* idwt2(const std::string& name, std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& cLL, std::vector<std::vector<double>>& cLH,
            std::vector<std::vector<double>>& cHL, std::vector<std::vector<double>>& cHH);
void* dwt2(const std::string& name, std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& cLL, std::vector<std::vector<double>>& cLH,
           std::vector<std::vector<double>>& cHL, std::vector<std::vector<double>>& cHH);
void* downsamp2(std::vector<std::vector<double>>& vec1, std::vector<std::vector<double>>& vec2, int rowsDn, int colsDn);
void* upsamp2(std::vector<std::vector<double>>& vec1, std::vector<std::vector<double>>& vec2, int rowsUp, int colsUp);

// 2D DWT (Symmetric Extension) Functions
void* dwt_2d_sym(std::vector<std::vector<double>>& origsig, int J, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag,
                 std::vector<size_t>& length);
void* dwt2_sym(const std::string& name, std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& cLL, std::vector<std::vector<double>>& cLH,
               std::vector<std::vector<double>>& cHL, std::vector<std::vector<double>>& cHH);
void* idwt_2d_sym(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<std::vector<double>>& idwtOutput,
                  std::vector<size_t>& length);
void* circshift2d(std::vector<std::vector<double>>& signal, int x, int y);
void symm_ext2d(std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& temp2, int a);
void* dispDWT(std::vector<double>& output, std::vector<std::vector<double>>& dwtdisp, std::vector<size_t>& length, std::vector<size_t>& length2, int J);

//2D Stationary Wavelet Transform
void* swt_2d(std::vector<std::vector<double>>& sig, int J, const std::string& nm, std::vector<double>& swtOutput);
void* per_ext2d(std::vector<std::vector<double>>& signal, std::vector<std::vector<double>>& temp2, int a);

// FFT functions
double convfft(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c);
double convfftm(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c);
void* fft(std::vector<std::complex<double>>& data, int sign, size_t n);
void* bitreverse(std::vector<std::complex<double>>& sig);
void* freq(std::vector<double>& sig, std::vector<double>& freqResp);

//New
void* dwt1_sym_m(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD);//FFTW3 for 2D
void* idwt1_sym_m(const std::string& wname, std::vector<double>& x, std::vector<double>& app, std::vector<double>& detail);
void* dwt(std::vector<double>& sig, int j, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag, std::vector<size_t>& length);
void* idwt(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<double>& idwtOutput, std::vector<size_t>& length);
void* dwt_2d(std::vector<std::vector<double>>& origsig, int J, const std::string& nm, std::vector<double>& dwtOutput, std::vector<double>& flag,
             std::vector<size_t>& length);
void* dwt1_m(const std::string& wname, std::vector<double>& signal, std::vector<double>& cA, std::vector<double>& cD);
void* idwt_2d(std::vector<double>& dwtop, std::vector<double>& flag, const std::string& nm, std::vector<std::vector<double>>& idwtOutput,
              std::vector<size_t>& length);
void* idwt1_m(const std::string& wname, std::vector<double>& x, std::vector<double>& cA, std::vector<double>& cD);
void* dwt_output_dim2(std::vector<size_t>& length, std::vector<size_t>& length2, int j);

#endif
