///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxSharedMemoryWriter.hpp
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

#pragma once

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

//#include <pair>
#include <vector>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/containers/string.hpp>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.

namespace OpenViBE {
namespace Plugins {
namespace FileReadingAndWriting {

struct SMatrix
{
    size_t rowDim, colDim;
    boost::interprocess::offset_ptr<double> data;
};

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> ShmString;
typedef boost::interprocess::allocator<ShmString, boost::interprocess::managed_shared_memory::segment_manager> StringAllocator;
//typedef vector<ShmString, StringAllocator> MyShmStringVector;     
typedef boost::interprocess::allocator<std::pair<const ShmString, CIdentifier>, boost::interprocess::managed_shared_memory::segment_manager>
ShmemAllocatorMetaInfo;
typedef boost::interprocess::map<ShmString, CIdentifier, std::less<ShmString>, ShmemAllocatorMetaInfo> MyVectorMetaInfo;

typedef boost::interprocess::allocator<size_t, boost::interprocess::managed_shared_memory::segment_manager> ShmemAllocatorStimulation;
typedef boost::interprocess::allocator<boost::interprocess::offset_ptr<SMatrix>, boost::interprocess::managed_shared_memory::segment_manager>
ShmemAllocatorMatrix;
typedef boost::interprocess::vector<size_t, ShmemAllocatorStimulation> MyVectorStimulation;
typedef boost::interprocess::vector<boost::interprocess::offset_ptr<SMatrix>, ShmemAllocatorMatrix> MyVectorStreamedMatrix;

//--------------------------------------------------------------------------------
/// <summary>  The class CBoxSharedMemoryWriter describes the box SharedMemoryWriter. </summary>
class CBoxSharedMemoryWriter final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SharedMemoryWriter)

protected:
    std::vector<Toolkit::TDecoder<CBoxSharedMemoryWriter>*> m_decoders;
    CString m_sharedMemoryName;
    boost::interprocess::managed_shared_memory m_sharedMemoryArray;

private:
    CIdentifier m_typeID = CIdentifier::undefined();
    CString m_mutexName;
    boost::interprocess::named_mutex* m_mutex = nullptr;
    std::vector<MyVectorStimulation*> m_stimSets;
    std::vector<MyVectorStreamedMatrix*> m_streamedMatrices;
};


//--------------------------------------------------------------------------------
/// <summary>  Listener of the box SharedMemoryWriter. </summary>
class CBoxSharedMemoryWriterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }
    bool onInputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }
    bool onInputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

//--------------------------------------------------------------------------------
/// <summary>  Descriptor of the box SharedMemoryWriter. </summary>
class CBoxSharedMemoryWriterDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "SharedMemoryWriter"; }
    CString getAuthorName() const override { return "Dieter Devlaminck"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Stream input to shared memory"; }

    CString getDetailedDescription() const override
    {
        return
                "The box writes input to shared memory so that it can be read by another process. Stimuli and streamed matrices are supported, and transformed into a format that can be written into shared memory. Based on the input types, a metainfo variable will be created in shared memory that will specify which variables have which type. This way the client can know what it will be reading.";
    }

    CString getCategory() const override { return "File reading and writing"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return Box_SharedMemoryWriter; }
    IPluginObject* create() override { return new CBoxSharedMemoryWriter; }


    IBoxListener* createBoxListener() const override { return new CBoxSharedMemoryWriterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("prediction1",OV_TypeId_StreamedMatrix);

        prototype.addSetting("SharedMemoryName", OV_TypeId_String, "SharedMemory_P300Stimulator");

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SharedMemoryWriterDesc)
};
}  // namespace FileReadingAndWriting
}  // namespace Plugins
}  // namespace OpenViBE
