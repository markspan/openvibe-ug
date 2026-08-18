///-------------------------------------------------------------------------------------------------
/// 
/// \file CSpectrumDatabase.hpp
/// \brief Definition for the class CSpectrumDatabase.
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

#include "CStreamedMatrixDatabase.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
/**
* This class is used to store information about the incoming spectrum stream. It can request a IStreamDisplayDrawable
* object to redraw itself upon changes in its data.
*/
class CSpectrumDatabase final : public CStreamedMatrixDatabase
{
public:
    explicit CSpectrumDatabase(Toolkit::TBoxAlgorithm<IBoxAlgorithm>& parent) : CStreamedMatrixDatabase(parent) { }
    ~CSpectrumDatabase() override = default;

    bool Initialize() override;

    /// <summary> Get number of frequency bands. </summary>
    /// <returns> Number of frequency bands. </returns>
    size_t GetFrequencyAbscissaCount() const { return m_frequencyAbscissa.size(); }

protected:
    bool decodeHeader() override;

    /// <summary> Set displayed frequency range. </summary>
    /// <param name="minimumDisplayedFrequency"> Minimum frequency to display. </param>
    /// <param name="maximumDisplayedFrequency"> Maximum frequency to display. </param>
    //TODO (if min/max computation should be restricted to this range)
    /*
    void setDisplayedFrequencyRange(double minimumDisplayedFrequency, double maximumDisplayedFrequency);*/

    /** \name Frequency bands management */
    //@{


    /// <summary> Get width of a frequency band (in Hz). </summary>
    /// <returns> Frequency band width. </returns>
    // double getFrequencyBandWidth();

    /// <summary> Get frequency band start frequency. </summary>
    /// <param name="index"> Index of frequency band. </param>
    /// <returns> Frequency band start if it could be retrieved, 0 otherwise. </returns>
    // double getFrequencyBandStart(const size_t index);

    /// <summary> Get frequency band stop frequency. </summary>
    /// <param name="index"> Index of frequency band. </param>
    /// <returns> Frequency band stop if it could be retrieved, 0 otherwise. </returns>
    // double getFrequencyBandStop(const size_t index);

    //@}

private:
    std::vector<double> m_frequencyAbscissa;
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
