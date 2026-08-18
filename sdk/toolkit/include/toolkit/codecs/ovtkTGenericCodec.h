///-------------------------------------------------------------------------------------------------
/// 
/// \file ovtkTGenericCodec.h
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

#include <map>

#include <openvibe/ov_all.h>

#include "../ovtk_base.h"
#include "decoders/ovtkTStreamedMatrixDecoder.h"
#include "decoders/ovtkTSignalDecoder.h"
#include "decoders/ovtkTSpectrumDecoder.h"
#include "decoders/ovtkTFeatureVectorDecoder.h"

#include "encoders/ovtkTStreamedMatrixEncoder.h"
#include "encoders/ovtkTSignalEncoder.h"
#include "encoders/ovtkTSpectrumEncoder.h"
#include "encoders/ovtkTFeatureVectorEncoder.h"

namespace OpenViBE {
namespace Toolkit {

// ______________________________________________________________________________________________________________________________________________________________________________
// ______________________________________________________________________________________________________________________________________________________________________________
//

#define decoder_return_impl(what) \
        if(m_signalDecoder) { return m_signalDecoder->what; } \
        if(m_spectrumDecoder) { return m_spectrumDecoder->what; } \
        if(m_featureVectorDecoder) { return m_featureVectorDecoder->what; } \
        return m_streamedMatrixDecoder->what;

template <class T>
class TGenericDecoder
{
protected:

    void reset()
    {
        m_streamedMatrixDecoder = nullptr;
        m_signalDecoder         = nullptr;
        m_spectrumDecoder       = nullptr;
        m_featureVectorDecoder  = nullptr;
    }

public:

    TGenericDecoder() : m_streamedMatrixDecoder(nullptr), m_signalDecoder(nullptr), m_spectrumDecoder(nullptr), m_featureVectorDecoder(nullptr)
    {
        this->reset();
    }

    ~TGenericDecoder() { this->uninitialize(); }

    TGenericDecoder<T>& operator=(TStreamedMatrixDecoder<T>* decoder)
    {
        this->reset();
        m_streamedMatrixDecoder = decoder;
        return *this;
    }

    TGenericDecoder<T>& operator=(TSignalDecoder<T>* decoder)
    {
        this->reset();
        m_signalDecoder = decoder;
        return *this;
    }

    TGenericDecoder<T>& operator=(TSpectrumDecoder<T>* decoder)
    {
        this->reset();
        m_spectrumDecoder = decoder;
        return *this;
    }

    TGenericDecoder<T>& operator=(TFeatureVectorDecoder<T>* decoder)
    {
        this->reset();
        m_featureVectorDecoder = decoder;
        return *this;
    }

    void uninitialize() { this->reset(); }

    Kernel::TParameterHandler<CMatrix*>& getOutputMatrix()
    {
        decoder_return_impl(getOutputMatrix());
    }

    Kernel::TParameterHandler<uint64_t>& getOutputSamplingRate()
    {
        if (m_signalDecoder) { return m_signalDecoder->getOutputSamplingRate(); }
        return m_spectrumDecoder->getOutputSamplingRate();
    }

    Kernel::TParameterHandler<CMatrix*>& getOutputFrequencyAbcissa() { return m_spectrumDecoder->getOutputFrequencyAbscissa(); }

    bool decode(int, int)       = delete;
    bool decode(size_t, size_t) = delete;

    bool decode(const size_t chunkIdx, bool bMarkInputAsDeprecated = true)
    {
        decoder_return_impl(decode(chunkIdx, bMarkInputAsDeprecated));
    }

    bool isHeaderReceived()
    {
        decoder_return_impl(isHeaderReceived());
    }

    bool isBufferReceived()
    {
        decoder_return_impl(isBufferReceived());
    }

    bool isEndReceived()
    {
        decoder_return_impl(isEndReceived());
    }

protected:

    TStreamedMatrixDecoder<T>* m_streamedMatrixDecoder = nullptr;
    TSignalDecoder<T>* m_signalDecoder                 = nullptr;
    TSpectrumDecoder<T>* m_spectrumDecoder             = nullptr;
    TFeatureVectorDecoder<T>* m_featureVectorDecoder   = nullptr;
};

// ______________________________________________________________________________________________________________________________________________________________________________
// ______________________________________________________________________________________________________________________________________________________________________________
//

#define encoder_return_impl(what) \
        if(m_signalEncoder) { return m_signalEncoder->what; } \
        if(m_spectrumEncoder) { return m_spectrumEncoder->what; } \
        if(m_featureVectorEncoder) { return m_featureVectorEncoder->what; } \
        return m_streamedMatrixDecoder->what;

template <class T>
class TGenericEncoder
{
protected:

    void reset()
    {
        m_streamedMatrixDecoder = nullptr;
        m_signalEncoder         = nullptr;
        m_spectrumEncoder       = nullptr;
        m_featureVectorEncoder  = nullptr;
    }

public:

    TGenericEncoder() : m_streamedMatrixDecoder(nullptr), m_signalEncoder(nullptr), m_spectrumEncoder(nullptr), m_featureVectorEncoder(nullptr)
    {
        this->reset();
    }

    ~TGenericEncoder() { this->uninitialize(); }

    TGenericEncoder<T>& operator=(TStreamedMatrixEncoder<T>* pEncoder)
    {
        this->reset();
        m_streamedMatrixDecoder = pEncoder;
        return *this;
    }

    TGenericEncoder<T>& operator=(TSignalEncoder<T>* pEncoder)
    {
        this->reset();
        m_signalEncoder = pEncoder;
        return *this;
    }

    TGenericEncoder<T>& operator=(TSpectrumEncoder<T>* pEncoder)
    {
        this->reset();
        m_spectrumEncoder = pEncoder;
        return *this;
    }

    TGenericEncoder<T>& operator=(TFeatureVectorEncoder<T>* pEncoder)
    {
        this->reset();
        m_featureVectorEncoder = pEncoder;
        return *this;
    }

    void uninitialize() { this->reset(); }

    Kernel::TParameterHandler<CMatrix*>& getInputMatrix()
    {
        encoder_return_impl(getInputMatrix());
    }

    Kernel::TParameterHandler<uint64_t>& getInputSamplingRate()
    {
        if (m_signalEncoder) { return m_signalEncoder->getInputSamplingRate(); }
        return m_spectrumEncoder->getInputSamplingRate();
    }

    Kernel::TParameterHandler<CMatrix*>& getInputFrequencyAbcissa() { return m_spectrumEncoder->getInputFrequencyAbscissa(); }

    bool encodeHeader()
    {
        encoder_return_impl(encodeHeader());
    }

    bool encodeBuffer()
    {
        encoder_return_impl(encodeBuffer());
    }

    bool encodeEnd()
    {
        encoder_return_impl(encodeEnd());
    }

protected:

    TStreamedMatrixEncoder<T>* m_streamedMatrixDecoder = nullptr;
    TSignalEncoder<T>* m_signalEncoder                 = nullptr;
    TSpectrumEncoder<T>* m_spectrumEncoder             = nullptr;
    TFeatureVectorEncoder<T>* m_featureVectorEncoder   = nullptr;
};

// ______________________________________________________________________________________________________________________________________________________________________________
// ______________________________________________________________________________________________________________________________________________________________________________
//

template <class T>
class TGenericListener final : public T
{
public:

    typedef enum
    {
        Type_None = 0x00000000,
        Type_StreamedMatrix = 0x00000001,
        Type_Signal = 0x00000002,
        Type_Spectrum = 0x00000004,
        Type_Covariance = 0x00000008,
        Type_All = 0xffffffff
    } EType;

    explicit TGenericListener(const size_t typeFlag = Type_All)
    {
        if (typeFlag & Type_StreamedMatrix) m_allowedTypeIDs[OV_TypeId_StreamedMatrix] = true;
        if (typeFlag & Type_Signal) m_allowedTypeIDs[OV_TypeId_Signal] = true;
        if (typeFlag & Type_Spectrum) m_allowedTypeIDs[OV_TypeId_Spectrum] = true;
        if (typeFlag & Type_Covariance) m_allowedTypeIDs[OV_TypeId_CovarianceMatrix] = true;
    }

    bool isValidInputType(const CIdentifier& typeID, size_t /*index*/)
    {
        return m_allowedTypeIDs[typeID];
        //return (typeID==OV_TypeId_Signal || typeID==OV_TypeId_Spectrum);
    }

    virtual bool onInputTypeChanged(Kernel::IBox& box, const size_t index)
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        if (this->isValidInputType(typeID, index)) { box.setOutputType(index, typeID); }
        else
        {
            box.getOutputType(index, typeID);
            box.setInputType(index, typeID);
        }
        return true;
    }

    bool isValidOutputType(const CIdentifier& typeID, size_t /*index*/)
    {
        return m_allowedTypeIDs[typeID];
        //return (typeID==OV_TypeId_Signal || typeID==OV_TypeId_Spectrum);
    }

    virtual bool onOutputTypeChanged(Kernel::IBox& box, const size_t index)
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        if (this->isValidOutputType(typeID, index)) { box.setInputType(index, typeID); }
        else
        {
            box.getInputType(index, typeID);
            box.setOutputType(index, typeID);
        }
        return true;
    }

    _IsDerivedFromClass_Final_(T, CIdentifier::undefined())

private:

    std::map<CIdentifier, bool> m_allowedTypeIDs;
};
}  // namespace Toolkit
}  // namespace OpenViBE
