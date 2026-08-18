#pragma once

#if defined TARGET_HAS_ThirdPartyPython2

#include <Python.h>

#if defined(PY_MAJOR_VERSION) && (PY_MAJOR_VERSION == 2)

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <string>
#include <vector>
//#include <map>

namespace OpenViBE
{
    namespace Plugins
    {
        namespace Python
        {
            class CBoxAlgorithmPython2 final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
            {
            public:
                void release() override { delete this; }

                uint64_t getClockFrequency() override;
                bool initialize() override;
                bool uninitialize() override;
                bool processClock(Kernel::CMessageClock& msg) override;
                bool processInput(const size_t index) override;
                bool process() override;

                _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Python2)

            protected:

                uint64_t m_clockFrequency = 0;
                CString m_scriptFilename;

                std::vector<Toolkit::TDecoder<CBoxAlgorithmPython2>*> m_decoders;
                std::vector<Toolkit::TEncoder<CBoxAlgorithmPython2>*> m_encoders;

                //std::map<char,PyObject *> m_PyObjectMap;
                PyObject *m_box            = nullptr, *m_boxInput      = nullptr, *m_boxOutput  = nullptr, *m_boxSetting      = nullptr,
                         *m_boxCurrentTime = nullptr, *m_boxInitialize = nullptr, *m_boxProcess = nullptr, *m_boxUninitialize = nullptr;
                bool m_initializeSucceeded = false;


                bool logSysStd(const bool out);
                bool logSysStdout() { return logSysStd(true); }
                bool logSysStderr() { return logSysStd(false); }
                void buildPythonSettings();

                bool initializePythonSafely();
                //bool clearPyObjectMap();
                bool transferStreamedMatrixInputChunksToPython(const size_t index);
                bool transferStreamedMatrixOutputChunksFromPython(const size_t index);
                bool transferSignalInputChunksToPython(const size_t index);
                bool transferSignalOutputChunksFromPython(const size_t index);
                bool transferStimulationInputChunksToPython(const size_t index);
                bool transferStimulationOutputChunksFromPython(const size_t index);

                static bool m_isPythonInitialized;

                // These are all borrowed references in python v2.7. Do not free them.
                static PyObject *m_mainModule, *m_mainDictionnary;
                static PyObject *m_matrixHeader, *m_matrixBuffer, *m_matrixEnd;
                static PyObject *m_signalHeader, *m_signalBuffer, *m_signalEnd;
                static PyObject *m_stimulationHeader, *m_stimulation, *m_stimulationSet, *m_stimulationEnd;
                static PyObject* m_buffer;
                static PyObject* m_execFileFunction;
                static PyObject *m_stdout, *m_stderr;
            };

            class CBoxAlgorithmPython2Listener final : public Toolkit::TBoxListener<IBoxListener>
            {
            public:
                bool onInputAdded(Kernel::IBox& box, const size_t index) override
                {
                    box.setInputType(index, OV_TypeId_StreamedMatrix);
                    return true;
                }

                bool onOutputAdded(Kernel::IBox& box, const size_t index) override
                {
                    box.setOutputType(index, OV_TypeId_StreamedMatrix);
                    return true;
                }

                _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
            };

            class CBoxAlgorithmPython2Desc final : virtual public IBoxAlgorithmDesc
            {
            public:

                void release() override { }

                CString getName() const override { return "Python 2 scripting"; }
                CString getAuthorName() const override { return "Aurelien Van Langhenhove and Laurent George"; }
                CString getAuthorCompanyName() const override { return "CICIT Garches, Inria"; }
                CString getShortDescription() const override { return "This box executes a python script."; }
                CString getDetailedDescription() const override { return ""; }
                CString getCategory() const override { return "Scripting"; }
                CString getVersion() const override { return "0.1"; }
                CString getStockItemName() const override { return "gtk-convert"; }
            
    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Python2; }
                IPluginObject* create() override { return new CBoxAlgorithmPython2; }
                IBoxListener* createBoxListener() const override { return new CBoxAlgorithmPython2Listener; }
                void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

                bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
                {
                    prototype.addSetting("Clock frequency (Hz)", OV_TypeId_Integer, "64");
                    prototype.addSetting("Script", OV_TypeId_Script, "");

                    prototype.addFlag(Kernel::BoxFlag_CanAddInput);
                    prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
                    prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
                    prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
                    prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
                    prototype.addFlag(Kernel::BoxFlag_CanModifySetting);

                    prototype.addInputSupport(OV_TypeId_Signal);
                    prototype.addInputSupport(OV_TypeId_Stimulations);
                    prototype.addInputSupport(OV_TypeId_StreamedMatrix);

                    prototype.addOutputSupport(OV_TypeId_Signal);
                    prototype.addOutputSupport(OV_TypeId_Stimulations);
                    prototype.addOutputSupport(OV_TypeId_StreamedMatrix);

                    return true;
                }

                _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_Python2Desc)
            };
        } // namespace Python
    }  // namespace Plugins
}  // namespace OpenViBE

#endif // #if defined(PY_MAJOR_VERSION) && (PY_MAJOR_VERSION == 2)

#endif // TARGET_HAS_ThirdPartyPython2
