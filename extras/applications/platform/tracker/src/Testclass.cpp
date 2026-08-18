//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include "Testclass.h"
// #include "Source.h"
#include "Stream.h"
#include "Encoder.h"

namespace OpenViBE {
namespace Tracker {

TestClass::TestClass(Kernel::IKernelContext& ctx) : m_Ctx(ctx)
{
    std::cout << "Testing\n";

    //  Encoder2<TypeMatrix::Header, TypeMatrix::Buffer, TypeMatrix::End> testCoder(ctx);
    //  Encoder<TypeMatrix> testCoder(ctx);

#if 0
    StreamSignal testStream(ctx);
    StreamMatrix testStream2(ctx);

    std::vector<Stream*> list;

    list.push_back(&testStream);
    list.push_back(&testStream2);

    Stream* tmp = list[0];
    
    std::cout << "First is " << ctx.getTypeManager().getTypeName(tmp->getTypeIdentifier()) << "\n";
#endif

    //const CString eegFile = Directories::getDataDir() + CString("/scenarios/signals/bci-motor-imagery.ov");
    //Source src;
    //src.initialize(eegFile.toASCIIString());

#if 0
    // Test code illustrating how to alter stimulation stream
    for (auto it = m_Streams.begin();it != m_Streams.end(); ++it)
    {
        if(it->second->getTypeIdentifier() == OV_TypeId_Stimulations)
        {
            TypeError::Buffer *ptr = nullptr;
            it->second->peek(CTime(5.0).time(), &ptr);
            TypeStimulation::Buffer *ptr2 = reinterpret_cast<TypeStimulation::Buffer*>(ptr);
            // std::cout << "cnt: " << ptr2->m_buffer.size() << "\n";

            // Request early stop
            ptr2->m_buffer.clear();
            ptr2->m_buffer.push_back(OVTK_StimulationId_ExperimentStop, CTime(5.0).time(),0);
        }
    }
#endif

#if 0
    StreamHeaderSignal signalHeader;
    signalHeader.m_samplingRate = 512;

    Stream<StreamHeaderSignal, StreamDataSignal> testStream(m_ctx);
    testStream.initialize(signalHeader);

    StreamDataSignal* data = new(StreamDataSignal);
    data->data.setDimensionSize(10,2);
    testStream.push(data);

    // Encode a stream
    Encoder<StreamHeaderSignal, StreamDataSignal> encoder(ctx);

    std::vector<CMemoryBuffer> encoded;

    encoded.push_back( encoder.encodeHeader(testStream.getHeader()) );

    for (size_t i=0;i<testStream.m_chunks.size(); ++i) { encoded.push_back( encoder.encodeBuffer(*testStream.m_chunks[i]) ); }

    Stream* ptr = testStream;
#endif
}

}  // namespace Tracker
}  // namespace OpenViBE
