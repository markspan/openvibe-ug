#pragma once

#include "windows.h"
#include "ov_defines.h"
#include "ovCString.h"

//___________________________________________________________________//
//                                                                   //
// Get Cpu Time                                                      //
//___________________________________________________________________//
//                                                                   //
namespace OpenViBE
{
    OV_API bool GetEnvVar( CString& sVar, CString& sValue );
    OV_API bool PutEnvVar( CString& sVar, CString& sValue );

    OV_API inline double GetCPUTimeInMilliseconds()
    {
        static bool bGotCPUFreq = false;
        static double timerFrequency = 1;
        if ( !bGotCPUFreq )
        {
            unsigned __int64 cpufreq = 1;
            QueryPerformanceFrequency((LARGE_INTEGER*)&cpufreq);
            timerFrequency = (1000.0/cpufreq);
        }
        unsigned __int64 curTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER *)&curTime);
        return timerFrequency * curTime;
    };

}