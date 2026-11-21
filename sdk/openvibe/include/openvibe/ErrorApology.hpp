///-------------------------------------------------------------------------------------------------
/// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.54.
///
/// \file ErrorApology.hpp
/// \brief The apology that begins every error message (OV_ERROR_LOG in ovAssert.h): picked at random from a few,
///        and aware of the errors before it in this program: one that follows another within a second
///        apologises again, one of a type seen before says so.
///-------------------------------------------------------------------------------------------------

#pragma once

#include "ov_defines.h"
#include "ovCString.h"

#include <cstddef>

namespace OpenViBE {

/// What the apology for an error knows about the errors before it.
enum class EErrorContext
{
	First,		///< No error just before it, and none of its type yet.
	FollowsAnother,	///< Within a second of the previous error, which is often its cause.
	SeenBefore		///< Not right after another, but its type has occurred before.
};

/// The apology for the next error of the given type (the value of a Kernel::ErrorType), with a colon, e.g.
/// "I am so very sorry:". Counts the error, so call it once per error. Thread-safe.
OV_API CString errorApology(size_t errorType);

/// The apology itself, without counting: \p pick chooses among the phrasings for \p context, \p occurrence says
/// which occurrence of its type this error is (1 for the first). For tests, and for errorApology().
OV_API CString errorApologyText(EErrorContext context, size_t occurrence, size_t pick);

}  // namespace OpenViBE
