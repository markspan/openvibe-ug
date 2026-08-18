#include <openvibe/ov_all.h>

#include <fs/Files.h>

#include "BoxAdapter.h"
#include "CodecFactory.h"

namespace OpenViBE {
namespace Tracker {


bool BoxAdapter::initialize()
{
    m_boxAlgorithm = m_kernelCtx.getPluginManager().createBoxAlgorithm(m_algorithmID, nullptr);
    if (!m_boxAlgorithm) {
        log() << Kernel::LogLevel_Error << "Error: failed to create algorithm for id " << m_algorithmID.str() << "\n";
        return false;
    }
    m_boxAlgorithmCtx.getTrackerBoxIO()->initialize(m_boxAlgorithmCtx.getStaticBoxContext());

    return true;
}

bool BoxAdapter::uninitialize()
{
    if (m_boxAlgorithm) {
        m_boxAlgorithm->uninitialize(m_boxAlgorithmCtx);
        m_kernelCtx.getPluginManager().releasePluginObject(m_boxAlgorithm);
        m_boxAlgorithm = nullptr;
    }

    return true;
}

BoxAdapterBundle::~BoxAdapterBundle()
{
    for (auto decoder : m_decoders) { delete decoder; }
    m_decoders.clear();
    for (auto encoder : m_encoders) { delete encoder; }
    m_encoders.clear();
}

bool BoxAdapterBundle::initialize()
{
    if (!BoxAdapter::initialize()) { return false; }

    Kernel::IBox* staticBoxContext = const_cast<Kernel::IBox*>(m_boxAlgorithmCtx.getStaticBoxContext());

    if (m_src) {
        for (size_t i = 0; i < m_src->getNumStreams(); ++i) {
            // @fixme test boxes input support here
            std::stringstream ss;
            ss << "Stream" << i; // not really used since there's no GUI for the box
            staticBoxContext->addInput(ss.str().c_str(), m_src->getStream(i)->getTypeIdentifier());
            m_encoders.push_back(CodecFactory::getEncoder(m_kernelCtx, *m_src->getStream(i)));
            if (m_dst) {
                m_dst->createStream(i, m_src->getStream(i)->getTypeIdentifier());
                m_decoders.push_back(CodecFactory::getDecoder(m_kernelCtx, *m_dst->getStream(i)));
                staticBoxContext->addOutput(ss.str().c_str(), m_dst->getStream(i)->getTypeIdentifier());
            }
        }
    }

    m_boxAlgorithm->initialize(m_boxAlgorithmCtx);

    return true;
}

bool BoxAdapterBundle::spool(const bool /* verbose */)
{
    Kernel::IBoxIO* boxCtx = const_cast<Kernel::IBoxIO*>(m_boxAlgorithmCtx.getDynamicBoxContext());
    TrackerBoxIO* ioCtx    = static_cast<TrackerBoxIO*>(boxCtx);

    if (!m_src) {
        log() << Kernel::LogLevel_Error << "Error: box wrapper doesn't yet support processing without source\n";
        return false;
    }

    m_src->rewind();

    const uint32_t nOutput = m_boxAlgorithmCtx.getStaticBoxContext()->getOutputCount();

    while (true) {
        size_t index;
        StreamPtr stream = m_src->getNextStream(index);
        if (!stream) { break; }
        EncodedChunk chk;
        EChunkType outputType;
        m_encoders[index]->encode(chk, outputType);

        ioCtx->addInputChunk(index, chk);

        m_boxAlgorithm->process(m_boxAlgorithmCtx);

        ioCtx->clearInputChunks();

        if (m_dst) {
            for (size_t j = 0; j < nOutput; ++j) {
                if (ioCtx->isReadyToSend(j)) {
                    ioCtx->getOutputChunk(j, chk);
                    m_decoders[j]->decode(chk);
                    ioCtx->deprecateOutput(j);
                }
            }
        }
        stream->step();
    }

    m_src->rewind();

    return true;
}

bool BoxAdapterStream::initialize()
{
    if (!BoxAdapter::initialize()) { return false; }

    Kernel::IBox* boxCtx = const_cast<Kernel::IBox*>(m_boxAlgorithmCtx.getStaticBoxContext());

    CIdentifier typeID;
    boxCtx->getInputType(0, typeID);
    if (typeID != m_src->getTypeIdentifier()) {
        log() << Kernel::LogLevel_Error << "Error: Box alg first input stream is wrong type\n";
        return false;
    }
    boxCtx->getOutputType(0, typeID);
    if (typeID != m_dst->getTypeIdentifier()) {
        log() << Kernel::LogLevel_Error << "Error: Box alg first output stream is wrong type\n";
        return false;
    }

    m_boxAlgorithm->initialize(m_boxAlgorithmCtx);

    return true;
}

bool BoxAdapterStream::spool(const bool verbose)
{
    Kernel::IBoxIO* boxCtx = const_cast<Kernel::IBoxIO*>(m_boxAlgorithmCtx.getDynamicBoxContext());
    TrackerBoxIO* ioCtx    = static_cast<TrackerBoxIO*>(boxCtx);

    if (!m_src) {
        log() << Kernel::LogLevel_Error << "Error: box wrapper doesn't yet support processing without source\n";
        return false;
    }

    m_src->reset();

    EncoderBase* encoder = CodecFactory::getEncoder(m_kernelCtx, *m_src);
    DecoderBase* decoder = CodecFactory::getDecoder(m_kernelCtx, *m_dst);

    if (verbose) { log() << Kernel::LogLevel_Info; }

    bool finished = false;
    uint64_t cnt  = 0;
    while (!finished) {
        if (m_src) {
            EncodedChunk chk;
            EChunkType outputType;
            if (encoder->encode(chk, outputType)) {
                // @fixme here we assume the box takes data in at the first slot
                ioCtx->clearInputChunks();
                ioCtx->addInputChunk(0, chk);

                m_src->step();
            }
            else { finished = true; }
        }

        m_boxAlgorithm->process(m_boxAlgorithmCtx);

        if (m_dst) {
            if (ioCtx->isReadyToSend(0)) {
                EncodedChunk chk;
                ioCtx->getOutputChunk(0, chk);
                decoder->decode(chk);
                ioCtx->deprecateOutput(0);
            }
        }
        if (verbose && cnt++ % 100 == 0) { log() << "."; }
    }
    if (verbose) { log() << "\n"; }

    m_src->reset();

    ioCtx->clearInputChunks();

    delete encoder;
    delete decoder;

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
