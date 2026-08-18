///-------------------------------------------------------------------------------------------------
/// 
/// \file CStimulationSet.hpp
/// \brief Basic standalone OpenViBE Stimulation Set implementation.
/// 
/// This interface offers functionalities to handle a collection of OpenViBE stimulations.
/// This collection basicaly consists in a list of stimulation information.
/// Each stimulation has three information : an identifier, a dateand a duration.
/// \author  Yann Renard (INRIA/IRISA) & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/11/2021.
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

#include "ov_defines.h"
#include <vector>
#include <string>
#include <sstream>

namespace OpenViBE {
/// <summary> OpenViBE StimulationSet Class. </summary>
/// <seealso cref="IObject" />
class OV_API CStimulationSet final
{
public:
    //--------------------------------------------------
    //------------ Constructor / Destructor ------------
    //--------------------------------------------------

    CStimulationSet() : m_set(new std::vector<SStimulation>) { }
    ~CStimulationSet() { delete m_set; }

    //--------------------------------------------------
    //----------------- Getter/Setter ------------------
    //--------------------------------------------------
    
    /// <summary> Clears this stimulation set removing every existing stimulation. </summary>
    void clear() const { m_set->clear(); }

    /// <summary> Clears this stimulation set removing every existing stimulation. </summary>
    /// <return> Clears this stimulation set removing every existing stimulation. </return>
    size_t size() const { return m_set->size(); }

    /// <summary> Changes the stimulation count in this stimulation set. </summary>
    /// <param name="n">the new number of stimulations. </param>
    void resize(const size_t n) const { m_set->resize(n); }

    /// <summary> Gets the identifier of a specific stimulation in this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation which identifier has to be returned. </param>
    /// <return> The identifier for the specified stimulation. </return>
    uint64_t getId(const size_t index) const { return m_set->at(index).id; }

    /// <summary> Gets the date of a specific stimulation in this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation which date has to be returned. </param>
    /// <return> the date for the specified stimulation. </return>
    /// <remarks> The returned date is relative to the beginning of this stimulation set. </remarks>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    uint64_t getDate(const size_t index) const { return m_set->at(index).date; }

    /// <summary> Gets the duration of a specific stimulation in this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation which duration has to be returned. </param>
    /// <return> the duration for the specified stimulation. </return>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    uint64_t getDuration(const size_t index) const { return m_set->at(index).duration; }

    /// <summary> Changes the identifier of a specific stimulation in this stimulation set. </summary>
    /// <param name="index">the index of the stimulation which id should be changed. </param>
    /// <param name="id">the new id for the specified stimulation. </param>
    void setId(const size_t index, const uint64_t id) const { m_set->at(index).id = id; }

    /// <summary> Changes the date of a specific stimulation in this stimulation set. </summary>
    /// <param name="index">the index of the stimulation which date should be changed. </param>
    /// <param name="date">the new date for the specified stimulation. </param>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    void setDate(const size_t index, const uint64_t date) const { m_set->at(index).date = date; }

    /// <summary> Changes the duration of a specific stimulation in this stimulation set. </summary>
    /// <param name="index">the index of the stimulation which duration should be changed. </param>
    /// <param name="duration">the new duration for the specified stimulation. </param>
    /// <remarks> The returned date is relative to the beginning of this stimulation set. </remarks>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    void setDuration(const size_t index, const uint64_t duration) const { m_set->at(index).duration = duration; }

    //--------------------------------------------------
    //---------------------- Misc ----------------------
    //--------------------------------------------------
    /// <summary> Appends a stimulation to this stimulation set. </summary>
    /// <param name="id">the identifier of the stimulation to append. </param>
    /// <param name="date">the date of the stimulation. </param>
    /// <param name="duration">the duration of the stimulation. </param>
    void push_back(const uint64_t id, const uint64_t date, const uint64_t duration) const { m_set->push_back(SStimulation(id, date, duration)); }

    /// <summary> Inserts a stimulation to this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation to insert. </param>
    /// <param name="id"> The identifier of the stimulation. </param>
    /// <param name="date"> The date of the stimulation. </param>
    /// <param name="duration"> The duration of the stimulation. </param>
    /// <remarks> Stimulation Set indexing changes after calling this function : stimulation(s) following the given index have their index incremented by 1. </remarks>
    void insert(const size_t index, const uint64_t id, const uint64_t date, const uint64_t duration) const
    {
        m_set->insert(m_set->begin() + index, SStimulation(id, date, duration));
    }

    /// <summary> Removes a stimulation from this stimulation set. </summary>
    /// <param name="index">the index of the stimulation to remove. </param>
    /// <remarks> Stimulation Set indexing changes after calling this function : stimulation(s) following the given index have their index decremented by 1. </remarks>
    void erase(const size_t index) const { m_set->erase(m_set->begin() + index); }

    /// <summary> Shifts the dates of all stimulations in set. </summary>
    /// <param name="shift"> The time shift. </param>
    void shift(const uint64_t shift) const { for (auto& stim : *m_set) { stim.date += shift; } }

    /// <summary> Copies the specified stimulation set. </summary>
    /// <param name="set"> The stimulation set to copy. </param>
    /// <param name="shift"> The time shift. </param>
    void copy(const CStimulationSet& set, const uint64_t shift = 0) const
    {
        if (this == &set) { return; }
        clear();
        append(set, shift);
    }

    /// <summary> Appends the specified stimulation set. </summary>
    /// <param name="set"> The stimulation set to append. </param>
    /// <param name="shift"> The time shift. </param>
    void append(const CStimulationSet& set, const uint64_t shift = 0) const
    {
        const size_t n = set.size();
        for (size_t i = 0; i < n; ++i) { push_back(set.getId(i), set.getDate(i) + shift, set.getDuration(i)); }
    }

    /// <summary> Appends the stimulations that are in the specified time range. </summary>
    /// <param name="set"> The stimulation set to append. </param>
    /// <param name="startTime"> The start time. </param>
    /// <param name="endTime"> The end time. </param>
    /// <param name="shift"> The time shift. </param>
    /// <remarks> The range includes start time, but excludes end time. The shift is applied after the selection. </remarks>
    void appendRange(const CStimulationSet& set, const uint64_t startTime, const uint64_t endTime, const uint64_t shift = 0) const
    {
        const size_t n = set.size();
        for (size_t i = 0; i < n; ++i) {
            if (startTime <= set.getDate(i) && set.getDate(i) < endTime) { push_back(set.getId(i), set.getDate(i) + shift, set.getDuration(i)); }
        }
    }

    /// <summary> Removes the stimulations that are in the specified time range. </summary>
    /// <param name="startTime"> The start time. </param>
    /// <param name="endTime"> The end time. </param>
    /// <remarks> The range includes start time, but excludes end time. </remarks>
    void removeRange(const uint64_t startTime, const uint64_t endTime) const
    {
        for (size_t i = 0; i < size(); ++i) { if (startTime <= getDate(i) && getDate(i) < endTime) { erase(i--); } }
    }


    /// <summary> Provides readable format of the stimulation set. </summary>
    /// <returns> the Stimulation Set. </returns>
    std::string str() const
    {
        std::stringstream ss;
        ss.precision(10);
        for (auto& stim : *m_set) { ss << "[" << stim.id << ", " << stim.date << ", " << stim.duration << "]" << std::endl; }
        return ss.str();
    }

    /// <summary> Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns> Return the modified ostream. </returns>
    friend std::ostream& operator<<(std::ostream& os, const CStimulationSet& obj)
    {
        os << obj.str();
        return os;
    }

    //--------------------------------------------------------
    //---------------------- Deprecated ----------------------
    //--------------------------------------------------------

    /// <summary> Clears this stimulation set removing every existing stimulation. </summary>
    /// <return> Clears this stimulation set removing every existing stimulation. </return>
    /// \deprecated Use size() method instead (more standard naming style).
    OV_Deprecated("Use size() method instead (more standard naming style).")
    size_t getStimulationCount() const { return m_set->size(); }

    /// <summary> Changes the stimulation count in this stimulation set. </summary>
    /// <param name="n">the new number of stimulations. </param>
    /// <remarks> Prefer use resize (more stl naming style). </remarks>
    /// \deprecated Use resize() method instead (more standard naming style).
    OV_Deprecated("Use resize() method instead (more standard naming style).")
    void setStimulationCount(const size_t n) const { m_set->resize(n); }

    /// <summary> Gets the identifier of a specific stimulation in this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation which identifier has to be returned. </param>
    /// <return> The identifier for the specified stimulation. </return>
    /// \deprecated Use getId() method instead.
    OV_Deprecated("Use getId() method instead.")
    uint64_t getStimulationIdentifier(const size_t index) const { return m_set->at(index).id; }

    /// <summary> Gets the date of a specific stimulation in this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation which date has to be returned. </param>
    /// <return> the date for the specified stimulation. </return>
    /// <remarks> The returned date is relative to the beginning of this stimulation set. </remarks>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    /// \deprecated Use getDate() method instead.
    OV_Deprecated("Use getDate() method instead.")
    uint64_t getStimulationDate(const size_t index) const { return m_set->at(index).date; }

    /// <summary> Gets the duration of a specific stimulation in this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation which duration has to be returned. </param>
    /// <return> the duration for the specified stimulation. </return>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    /// \deprecated Use getDuration() method instead.
    OV_Deprecated("Use getDuration() method instead.")
    uint64_t getStimulationDuration(const size_t index) const { return m_set->at(index).duration; }

    /// <summary> Changes the identifier of a specific stimulation in this stimulation set. </summary>
    /// <param name="index">the index of the stimulation which id should be changed. </param>
    /// <param name="id">the new id for the specified stimulation. </param>
    /// \deprecated Use setId() method instead.
    OV_Deprecated("Use setId() method instead.")
    void setStimulationIdentifier(const size_t index, const uint64_t id) const { m_set->at(index).id = id; }

    /// <summary> Changes the date of a specific stimulation in this stimulation set. </summary>
    /// <param name="index">the index of the stimulation which date should be changed. </param>
    /// <param name="date">the new date for the specified stimulation. </param>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    /// \deprecated Use setDate() method instead.
    OV_Deprecated("Use setDate() method instead.")
    void setStimulationDate(const size_t index, const uint64_t date) const { m_set->at(index).date = date; }

    /// <summary> Changes the duration of a specific stimulation in this stimulation set. </summary>
    /// <param name="index">the index of the stimulation which duration should be changed. </param>
    /// <param name="duration">the new duration for the specified stimulation. </param>
    /// <remarks> The returned date is relative to the beginning of this stimulation set. </remarks>
    /// <remarks> Dates and durations are returned in seconds fixed point 32:32. Refer to \Doc_TimeManagement for more details. </remarks>
    /// \deprecated Use setDuration() method instead.
    OV_Deprecated("Use setDuration() method instead.")
    void setStimulationDuration(const size_t index, const uint64_t duration) const { m_set->at(index).duration = duration; }

    /// <summary> Appends a stimulation to this stimulation set. </summary>
    /// <param name="id">the identifier of the stimulation to append. </param>
    /// <param name="date">the date of the stimulation. </param>
    /// <param name="duration">the duration of the stimulation. </param>
    /// \deprecated Use push_back() method instead (more standard naming style).
    OV_Deprecated("Use push_back() method instead (more standard naming style).")
    void appendStimulation(const uint64_t id, const uint64_t date, const uint64_t duration) const { m_set->push_back(SStimulation(id, date, duration)); }

    /// <summary> Inserts a stimulation to this stimulation set. </summary>
    /// <param name="index"> The index of the stimulation to insert. </param>
    /// <param name="id"> The identifier of the stimulation. </param>
    /// <param name="date"> The date of the stimulation. </param>
    /// <param name="duration"> The duration of the stimulation. </param>
    /// <remarks> Stimulation indexing change after call to this function : following stimulation(s) get one more indexed. </remarks>
    /// \deprecated Use insert() method instead (more standard naming style).
    OV_Deprecated("Use insert() method instead (more standard naming style).")
    void insertStimulation(const size_t index, const uint64_t id, const uint64_t date, const uint64_t duration) const
    {
        m_set->insert(m_set->begin() + index, SStimulation(id, date, duration));
    }

    /// <summary> Removes a stimulation from this stimulation set. </summary>
    /// <param name="index">the index of the stimulation to remove. </param>
    /// <remarks> Stimulation indexing change after call to this function : following stimulation(s) get one less indexed. </remarks>
    /// \deprecated Use erase() method instead (more standard naming style)
    OV_Deprecated("Use erase() method instead (more standard naming style)")
    void removeStimulation(const size_t index) const { m_set->erase(m_set->begin() + index); }

private:
    struct SStimulation
    {
        explicit SStimulation(const uint64_t id = 0, const uint64_t date = 0, const uint64_t duration = 0)
            : id(id), date(date), duration(duration) { }

        uint64_t id = 0, date = 0, duration = 0;
    };

    std::vector<SStimulation>* m_set;
};

/// \deprecated Use CStimulationSet instead
OV_Deprecated("Use CStimulationSet instead")
typedef CStimulationSet IStimulationSet;    ///< Keep previous compatibility. Avoid to used it, intended to be removed. 

}  // namespace OpenViBE
