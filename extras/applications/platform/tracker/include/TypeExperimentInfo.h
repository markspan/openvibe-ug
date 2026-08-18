#pragma once

#include <openvibe/ov_all.h>

#include "Chunk.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeExperimentInfo
 * \brief Abstact class defining chunk types for Experiment Information streams
 * \author J. T. Lindgren
 *
 */
class TypeExperimentInfo
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_ExperimentInfo; }

    class Header : public Chunk
    {
    public:
        // Experiment
        uint64_t m_ExperimentID = 0;
        std::string m_ExperimentDate;

        // Subject
        uint64_t m_SubjectID = 0;
        std::string m_SubjectName;
        uint64_t m_SubjectAge    = 0;
        uint64_t m_SubjectGender = 0;

        // Context
        uint64_t m_LaboratoryID = 0;
        std::string m_LaboratoryName;
        uint64_t m_TechnicianID = 0;
        std::string m_TechnicianName;
    };

    class Buffer : public Chunk { };    // Payload

    class End : public Chunk { };

    // Prevent constructing
    TypeExperimentInfo()                          = delete;
    TypeExperimentInfo(const TypeExperimentInfo&) = delete;
    TypeExperimentInfo(TypeExperimentInfo&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
