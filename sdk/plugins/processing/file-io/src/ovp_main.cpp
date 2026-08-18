#include "ovp_defines.h"

#include "algorithms/ovpCAlgorithmOVMatrixFileReader.h"
#include "algorithms/ovpCAlgorithmOVMatrixFileWriter.h"

#include "algorithms/xml-scenario/ovpCAlgorithmXMLScenarioExporter.h"
#include "algorithms/xml-scenario/ovpCAlgorithmXMLScenarioImporter.h"

#include "box-algorithms/csv/ovpCBoxAlgorithmCSVFileWriter.h"
#include "box-algorithms/csv/ovpCBoxAlgorithmCSVFileReader.h"

#include "box-algorithms/openvibe/ovpCBoxAlgorithmGenericStreamReader.h"
#include "box-algorithms/openvibe/ovpCBoxAlgorithmGenericStreamWriter.h"

#include "box-algorithms/ovpCBoxAlgorithmElectrodeLocalizationFileReader.h"

#include "box-algorithms/csv/CBoxAlgorithmOVCSVFileWriter.hpp"
#include "box-algorithms/csv/CBoxAlgorithmOVCSVFileReader.hpp"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

OVP_Declare_Begin()
    OVP_Declare_New(CAlgorithmOVMatrixFileReaderDesc)
    OVP_Declare_New(CAlgorithmOVMatrixFileWriterDesc)

    OVP_Declare_New(CAlgorithmXMLScenarioExporterDesc)
    OVP_Declare_New(CAlgorithmXMLScenarioImporterDesc)

    OVP_Declare_New(CBoxAlgorithmCSVFileWriterDesc)
    OVP_Declare_New(CBoxAlgorithmCSVFileReaderDesc)

    OVP_Declare_New(CBoxAlgorithmGenericStreamReaderDesc)
    OVP_Declare_New(CBoxAlgorithmGenericStreamWriterDesc)

    OVP_Declare_New(CBoxAlgorithmElectrodeLocalisationFileReaderDesc)

    OVP_Declare_New(CBoxAlgorithmOVCSVFileWriterDesc)
    OVP_Declare_New(CBoxAlgorithmOVCSVFileReaderDesc)

    context.getScenarioManager().registerScenarioImporter(OV_ScenarioImportContext_SchedulerMetaboxImport, ".mxb", OVP_ClassId_Algorithm_XMLScenarioImporter);
    context.getScenarioManager().registerScenarioImporter(OV_ScenarioImportContext_SchedulerMetaboxImport, ".xml", OVP_ClassId_Algorithm_XMLScenarioImporter);
    context.getConfigurationManager().createConfigurationToken("ScenarioFileNameExtension.xml", "OpenViBE XML Scenario");
    context.getConfigurationManager().createConfigurationToken("ScenarioFileNameExtension.mxs", "Mensia XML Scenario");
    context.getConfigurationManager().createConfigurationToken("ScenarioFileNameExtension.mxb", "Mensia XML Component");

OVP_Declare_End()

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
