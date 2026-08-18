//
// OpenViBE Tracker
//


#pragma once

#include "CodecsAll.h"

#include "Stream.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamFactory 
 * \brief A factory method returning an empty Stream object of the requested type
 * \author J. T. Lindgren
 *
 */
class StreamFactory
{
public:
    // Factory
    static StreamPtr getStream(const Kernel::IKernelContext& ctx, const CIdentifier& typeID)
    {
        std::shared_ptr<StreamBase> stream;
        // @todo others
        if (typeID == OVTK_TypeId_StreamedMatrix) { stream = std::make_shared<Stream<TypeMatrix>>(ctx); }
        else if (typeID == OVTK_TypeId_Signal) { stream = std::make_shared<Stream<TypeSignal>>(ctx); }
        else if (typeID == OVTK_TypeId_Stimulations) { stream = std::make_shared<Stream<TypeStimulation>>(ctx); }
        else if (typeID == OVTK_TypeId_ExperimentInfo) { stream = std::make_shared<Stream<TypeExperimentInfo>>(ctx); }
        else if (typeID == OV_TypeId_ChannelLocalisation) {
            stream = std::make_shared<Stream<TypeChannelLocalization>>(ctx);
        } // bug in toolkit atm with OVTK_ define
        else if (typeID == OV_TypeId_ChannelUnits) { stream = std::make_shared<Stream<TypeChannelUnits>>(ctx); } // bug in toolkit atm with OVTK_ define
        else if (typeID == OVTK_TypeId_Spectrum) { stream = std::make_shared<Stream<TypeSpectrum>>(ctx); }
        else if (typeID == OVTK_TypeId_FeatureVector) { stream = std::make_shared<Stream<TypeFeatureVector>>(ctx); }
        else {
            ctx.getLogManager() << Kernel::LogLevel_Info << "Warning: Unknown stream type "
                    << ctx.getTypeManager().getTypeName(typeID) << " " << typeID.str() << "\n";
            stream = std::make_shared<Stream<TypeError>>(ctx);
        }

        return stream;
    }

    // Prevent constructing
    StreamFactory()                     = delete;
    StreamFactory(const StreamFactory&) = delete;
    StreamFactory(StreamFactory&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
