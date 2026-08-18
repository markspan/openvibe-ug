#pragma once

// Boxes
//---------------------------------------------------------------------------------------------------
#define OVP_ClassId_Algorithm_OVMatrixFileReader                        OpenViBE::CIdentifier(0x10661A33, 0x0B0F44A7)
#define OVP_ClassId_Algorithm_OVMatrixFileReaderDesc                    OpenViBE::CIdentifier(0x0E873B5E, 0x0A287FCB)
#define OVP_ClassId_Algorithm_OVMatrixFileWriter                        OpenViBE::CIdentifier(0x739158FC, 0x1E8240CC)
#define OVP_ClassId_Algorithm_OVMatrixFileWriterDesc                    OpenViBE::CIdentifier(0x44CF6DD0, 0x329D47F9)
#define OVP_ClassId_Algorithm_XMLScenarioExporter                       OpenViBE::CIdentifier(0x53693531, 0xB136CF3F)
#define OVP_ClassId_Algorithm_XMLScenarioExporterDesc                   OpenViBE::CIdentifier(0x9709C9FA, 0xF126F74E)
#define OVP_ClassId_Algorithm_XMLScenarioImporter                       OpenViBE::CIdentifier(0xE80C3EA2, 0x149C4A05)
#define OVP_ClassId_Algorithm_XMLScenarioImporterDesc                   OpenViBE::CIdentifier(0xFF25D456, 0x721FCC57)
#define OVP_ClassId_BoxAlgorithm_CSVFileReaderDesc                      OpenViBE::CIdentifier(0x193F22E9, 0x26A67233)
#define OVP_ClassId_BoxAlgorithm_CSVFileReader                          OpenViBE::CIdentifier(0x641D0717, 0x02884107)
#define OVP_ClassId_BoxAlgorithm_CSVFileWriter                          OpenViBE::CIdentifier(0x2C9312F1, 0x2D6613E5)
#define OVP_ClassId_BoxAlgorithm_CSVFileWriterDesc                      OpenViBE::CIdentifier(0x65075FF7, 0x2B555E97)
#define OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReader        OpenViBE::CIdentifier(0x40704155, 0x19C50E8F)
#define OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReaderDesc    OpenViBE::CIdentifier(0x4796613F, 0x653A48D5)
#define OVP_ClassId_BoxAlgorithm_GenericStreamReader                    OpenViBE::CIdentifier(0x6468099F, 0x0370095A)
#define OVP_ClassId_BoxAlgorithm_GenericStreamReaderDesc                OpenViBE::CIdentifier(0x1F1E3A53, 0x6CA07237)
#define OVP_ClassId_BoxAlgorithm_GenericStreamWriter                    OpenViBE::CIdentifier(0x09C92218, 0x7C1216F8)
#define OVP_ClassId_BoxAlgorithm_GenericStreamWriterDesc                OpenViBE::CIdentifier(0x50AB506A, 0x54804437)
#define OVP_ClassId_BoxAlgorithm_OVCSVFileReader                        OpenViBE::CIdentifier(0x336A3D9A, 0x753F1BA4)
#define OVP_ClassId_BoxAlgorithm_OVCSVFileReaderDesc                    OpenViBE::CIdentifier(0x584E1948, 0x65E91650)
#define OVP_ClassId_BoxAlgorithm_OVCSVFileWriter                        OpenViBE::CIdentifier(0x428375E8, 0x325F2DB9)
#define OVP_ClassId_BoxAlgorithm_OVCSVFileWriterDesc                    OpenViBE::CIdentifier(0x4B5C1D8F, 0x570E45FD)

// Type definitions
//---------------------------------------------------------------------------------------------------
#define OVP_NodeId_OpenViBEStream_Header                                EBML::CIdentifier(0xF59505AB, 0x3684C8D8)
#define OVP_NodeId_OpenViBEStream_Header_Compression                    EBML::CIdentifier(0x40358769, 0x166380D1)
#define OVP_NodeId_OpenViBEStream_Header_StreamType                     EBML::CIdentifier(0x732EC1D1, 0xFE904087)
#define OVP_NodeId_OpenViBEStream_Header_ChannelType                    OVP_NodeId_OpenViBEStream_Header_StreamType // deprecated old name
#define OVP_NodeId_OpenViBEStream_Buffer                                EBML::CIdentifier(0x2E60AD18, 0x87A29BDF)
#define OVP_NodeId_OpenViBEStream_Buffer_StreamIndex                    EBML::CIdentifier(0x30A56D8A, 0xB9C12238)
#define OVP_NodeId_OpenViBEStream_Buffer_ChannelIndex                   OVP_NodeId_OpenViBEStream_Buffer_StreamIndex // deprecated old name
#define OVP_NodeId_OpenViBEStream_Buffer_StartTime                      EBML::CIdentifier(0x093E6A0A, 0xC5A9467B)
#define OVP_NodeId_OpenViBEStream_Buffer_EndTime                        EBML::CIdentifier(0x8B5CCCD9, 0xC5024F29)
#define OVP_NodeId_OpenViBEStream_Buffer_Content                        EBML::CIdentifier(0x8D4B0BE8, 0x7051265C)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename      OpenViBE::CIdentifier(0x28F87B29, 0x0B09737E)
#define OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix       OpenViBE::CIdentifier(0x2F9521E0, 0x027D789F)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Open            OpenViBE::CIdentifier(0x2F996376, 0x2A942485)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Load            OpenViBE::CIdentifier(0x22841807, 0x102D681C)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Close           OpenViBE::CIdentifier(0x7FDE77DA, 0x384A0B3D)
#define OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_Error          OpenViBE::CIdentifier(0x6D4F2F4B, 0x05EC6CB9)
#define OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_DataProduced   OpenViBE::CIdentifier(0x76F46051, 0x003B6FE8)
#define OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Filename      OpenViBE::CIdentifier(0x330D2D0B, 0x175271E6)
#define OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Matrix        OpenViBE::CIdentifier(0x6F6402EE, 0x493044F3)
