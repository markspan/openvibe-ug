///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMouseTracking.hpp
/// \author Alison Cellard (Inria)
/// \version 1.0.
/// \date 10/03/2014.
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

#if defined(TARGET_HAS_ThirdPartyGTK)

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <iostream>

#include <gtk/gtk.h>

namespace OpenViBE {
namespace Plugins {
namespace Tools {
/// <summary> The class CBoxAlgorithmMouseTracking describes the box Mouse tracking. </summary>
class CBoxAlgorithmMouseTracking final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    // CBoxAlgorithmMouseTracking();
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    uint64_t getClockFrequency() override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_MouseTracking)

protected:
    Toolkit::TSignalEncoder<CBoxAlgorithmMouseTracking> m_algo0SignalEncoder;   ///< Feature vector stream encoder
    Toolkit::TSignalEncoder<CBoxAlgorithmMouseTracking> m_algo1SignalEncoder;   ///< Feature vector stream encoder

    bool m_headerSent = false;  ///< To check if the header was sent or not

    size_t m_sampling         = 0;      ///< Requested Sampling frequency
    uint64_t m_clockFrequency = 0;      ///< Process clock frequency

    size_t m_nGeneratedEpochSample      = 0;        ///< Length of output chunks
    CMatrix* m_absoluteCoordinateBuffer = nullptr;  ///< Absolute coordinates of the mouse pointer, that is, relative to the window in fullscreen
    CMatrix* m_relativeCoordinateBuffer = nullptr;  ///< Relative coordinates of the mouse pointer, the coordinates is relative to the previous point

    size_t m_nSentSample = 0;

    GtkWidget* m_window = nullptr;  ///< Gtk window to track mouse position

    double m_prevX = 0; ///< X coordinate from the previous position (in pixel, reference is upper left corner of window)
    double m_prevY = 0; ///< Y coordinate from the previous position (in pixel, reference is upper left corner of window)


public:
    double m_MouseX = 0;    ///< X coordinate of mouse current position
    double m_MouseY = 0;    ///< Y coordinate of mouse current position
};


class CBoxAlgorithmMouseTrackingListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Mouse tracking. </summary>
class CBoxAlgorithmMouseTrackingDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Mouse Tracking"; }
    CString getAuthorName() const override { return "Alison Cellard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Track mouse position within the screen"; }
    CString getDetailedDescription() const override { return "Return absolute and relative to the previous one mouse position"; }
    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "1"; }
    CString getStockItemName() const override { return "gtk-index"; }

    CIdentifier getCreatedClass() const override { return Box_MouseTracking; }
    IPluginObject* create() override { return new CBoxAlgorithmMouseTracking; }


    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmMouseTrackingListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Absolute coordinate",OV_TypeId_Signal);
        prototype.addOutput("Previous relative coordinate",OV_TypeId_Signal);

        prototype.addSetting("Sampling Frequency",OV_TypeId_Integer, "16");
        prototype.addSetting("Generated epoch sample count",OV_TypeId_Integer, "1");

        prototype.addFlag(Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_MouseTrackingDesc)
};
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE


#endif
