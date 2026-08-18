///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxSharedMemoryWriter.cpp
/// \brief Class of the box SharedMemoryWriter.
/// \author Dieter Devlaminck (Inria).
/// \version 1.0.
/// \date 17/01/2013
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxSharedMemoryWriter.hpp"

#include <iostream>
#include <sstream>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace OpenViBE {
namespace Plugins {
namespace FileReadingAndWriting {

#define time2ms(x, y) ((x) * 1000 + (y) / 1000.0) + 0.5

//--------------------------------------------------------------------------------
bool CBoxSharedMemoryWriter::initialize()
{
    //remove and create shared memory
    m_sharedMemoryName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0); // can be local variable
    boost::interprocess::shared_memory_object::remove(m_sharedMemoryName.toASCIIString());
    m_sharedMemoryArray = boost::interprocess::managed_shared_memory(boost::interprocess::create_only, m_sharedMemoryName.toASCIIString(), 655360);

    //remove and create mutex
    m_mutexName = m_sharedMemoryName + "_Mutex";
    boost::interprocess::named_mutex::remove(m_mutexName.toASCIIString());
    m_mutex = new boost::interprocess::named_mutex(boost::interprocess::open_or_create, m_mutexName.toASCIIString());

    //create shared vector for meta info (type and name)
    const ShmemAllocatorMetaInfo metaInfo(m_sharedMemoryArray.get_segment_manager());
    MyVectorMetaInfo* metaInfoVector = m_sharedMemoryArray.construct<MyVectorMetaInfo>("MetaInfo")(std::less<ShmString>(), metaInfo);
    const StringAllocator instString(m_sharedMemoryArray.get_segment_manager());

    //fill meta info vector and create shared vector variable for the appropriate types
    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    for (size_t i = 0; i < boxContext.getInputCount(); ++i) {
        CIdentifier typeID;
        std::ostringstream convert;   // stream used for the conversion
        convert << i;

        boxContext.getInputType(i, typeID);
        if (typeID == OVTK_TypeId_StreamedMatrix) {
            m_decoders.push_back(new Toolkit::TStreamedMatrixDecoder<CBoxSharedMemoryWriter>());
            ShmString name("Matrix", instString);
            name += ShmString(convert.str().c_str(), instString);
            metaInfoVector->insert(std::make_pair(name, typeID));

            const ShmemAllocatorMatrix inst(m_sharedMemoryArray.get_segment_manager());
            m_streamedMatrices.push_back(m_sharedMemoryArray.construct<MyVectorStreamedMatrix>(name.c_str())(inst));

            this->getLogManager() << Kernel::LogLevel_Info << "Constructed variable in shared memory of type matrix with name " << name.c_str() << "\n";
        }
        else if (typeID == OVTK_TypeId_Stimulations) {
            m_decoders.push_back(new Toolkit::TStimulationDecoder<CBoxSharedMemoryWriter>());
            ShmString name("Stimuli", instString);
            name += ShmString(convert.str().c_str(), instString);
            metaInfoVector->insert(std::make_pair(name, typeID));

            const ShmemAllocatorStimulation inst(m_sharedMemoryArray.get_segment_manager());
            m_stimSets.push_back(m_sharedMemoryArray.construct<MyVectorStimulation>(name.c_str())(inst));

            this->getLogManager() << Kernel::LogLevel_Info << "Constructed variable in shared memory of type stimulation with name " << name.c_str() << "\n";
        }
        else { this->getLogManager() << Kernel::LogLevel_Warning << "Input type " << typeID << " is not supported\n"; }
        m_decoders.back()->initialize(*this, i);
    }

    //m_inputCounter = 0;

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxSharedMemoryWriter::uninitialize()
{
    m_sharedMemoryArray.destroy<MyVectorMetaInfo>("MetaInfo");

    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    for (int i = int(boxContext.getInputCount() - 1); i >= 0; i--) {
        CIdentifier typeID;
        boxContext.getInputType(i, typeID);
        const std::string idxStr = std::to_string(i);

        if (typeID == OVTK_TypeId_StreamedMatrix) {
            this->getLogManager() << Kernel::LogLevel_Debug << "Uninitialize shared memory variable associated with input " << i << "\n";
            for (size_t j = 0; j < m_streamedMatrices.back()->size(); ++j) {
                m_sharedMemoryArray.deallocate(m_streamedMatrices.back()->at(j)->data.get());
                m_sharedMemoryArray.deallocate(m_streamedMatrices.back()->at(j).get());
            }
            this->getLogManager() << Kernel::LogLevel_Debug << "Deallocated shared memory for variable with name Matrix " << idxStr << "\n";
            m_streamedMatrices.back()->clear();
            m_sharedMemoryArray.destroy<MyVectorStreamedMatrix>(("Matrix " + idxStr).c_str());

            //TODO: pop_back()?
        }
        else if (typeID == OVTK_TypeId_Stimulations) {
            m_stimSets.back()->clear();
            m_sharedMemoryArray.destroy<MyVectorStimulation>(("Stimuli" + idxStr).c_str());
        }
    }
    this->getLogManager() << Kernel::LogLevel_Debug << "Destroyed all shared variables associated with input" << "\n";
    boost::interprocess::shared_memory_object::remove(m_sharedMemoryName.toASCIIString());
    boost::interprocess::named_mutex::remove(m_mutexName.toASCIIString());
    delete m_mutex;
    for (size_t i = 0; i < boxContext.getInputCount(); ++i) {
        m_decoders[i]->uninitialize();
        delete m_decoders[i];
    }


    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxSharedMemoryWriter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxSharedMemoryWriter::process()
{
    Kernel::IBoxIO& boxContext           = this->getDynamicBoxContext();
    const Kernel::IBox& staticBoxContext = this->getStaticBoxContext();

    size_t iStimulusCounter = 0;
    size_t iMatrixCounter   = 0;

    for (size_t i = 0; i < staticBoxContext.getInputCount(); ++i) {
        CIdentifier typeID;
        staticBoxContext.getInputType(i, typeID);
        if (typeID == OVTK_TypeId_Stimulations) {
            for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
                //m_oAlgo0_StimulationDecoder.decode(j,i, false);
                m_decoders[i]->decode(j, false);
                //CStimulationSet stimSet;
                CStimulationSet* stimSet = dynamic_cast<Toolkit::TStimulationDecoder<CBoxSharedMemoryWriter>*>(m_decoders[i])->
                        getOutputStimulationSet();
                //Toolkit::StimulationSet::copy(l_oStimSet, *m_oAlgo0_StimulationDecoder.getOutputStimulationSet());
                if (m_decoders[i]->isHeaderReceived()) { boxContext.markInputAsDeprecated(i, j); }
                if (m_decoders[i]->isBufferReceived()) {
                    if (stimSet->size() > 0) {
                        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_mutex, boost::interprocess::try_to_lock);
                        if (lock) {
                            for (size_t k = 0; k < stimSet->size(); ++k) {
                                // @fixme m_stimSets defined as 32bit will only work correctly with stimuli that fit in 32bits (OV stimulations can be 64bit)
                                m_stimSets[iStimulusCounter]->push_back(size_t(stimSet->getId(k)));
                                this->getLogManager() << Kernel::LogLevel_Info << "Added stimulus with id " << m_stimSets[iStimulusCounter]->back() <<
                                        " to shared memory variable\n";
                            }
                            boxContext.markInputAsDeprecated(i, j);
                        }
                        else { this->getLogManager() << Kernel::LogLevel_Warning << "Shared memory writer could not lock mutex\n"; }
                    }
                    else { boxContext.markInputAsDeprecated(i, j); }
                }
                if (m_decoders[i]->isEndReceived()) { boxContext.markInputAsDeprecated(i, j); }
            }
            iStimulusCounter++;
        }
        else if (typeID == OVTK_TypeId_StreamedMatrix) {
            for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
                m_decoders[i]->decode(j, false);
                CMatrix* matrix = dynamic_cast<Toolkit::TStreamedMatrixDecoder<CBoxSharedMemoryWriter>*>(m_decoders[i])->getOutputMatrix();
                if (m_decoders[i]->isHeaderReceived()) { boxContext.markInputAsDeprecated(i, j); }
                if (m_decoders[i]->isBufferReceived()) {
                    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*m_mutex, boost::interprocess::try_to_lock);
                    if (lock) {
                        boost::interprocess::offset_ptr<SMatrix> shmMatrix = static_cast<SMatrix*>(m_sharedMemoryArray.allocate(sizeof(SMatrix)));

                        //if we receive a vector (second dimension to 0) we force to one otherwise no memory will be allocated
                        const size_t row = (matrix->getDimensionSize(1) == 0) ? 1 : matrix->getDimensionSize(1);

                        shmMatrix->rowDim = row;
                        shmMatrix->colDim = matrix->getDimensionSize(0);

                        this->getLogManager() << Kernel::LogLevel_Trace << "dimensions " << matrix->getDimensionCount() << "row " << matrix->getDimensionSize(1)
                                << " by " << " columns " << matrix->getDimensionSize(0) << "\n";

                        shmMatrix->data = static_cast<double*>(m_sharedMemoryArray.allocate(matrix->getDimensionSize(0) * row * sizeof(double)));
                        for (size_t k = 0; k < matrix->getBufferElementCount(); ++k) { *(shmMatrix->data + k) = *(matrix->getBuffer() + k); }

                        m_streamedMatrices[iMatrixCounter]->push_back(shmMatrix);

                        boxContext.markInputAsDeprecated(i, j);
                    }
                }
                if (m_decoders[i]->isEndReceived()) { boxContext.markInputAsDeprecated(i, j); }
            }
            iMatrixCounter++;
        }
    }

    return true;
}
//--------------------------------------------------------------------------------

}  // namespace FileReadingAndWriting
}  // namespace Plugins
}  // namespace OpenViBE
