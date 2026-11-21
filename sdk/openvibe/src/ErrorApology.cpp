///-------------------------------------------------------------------------------------------------
/// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.54.
///
/// \file ErrorApology.cpp
///-------------------------------------------------------------------------------------------------

#include "ErrorApology.hpp"

#include <array>
#include <chrono>
#include <map>
#include <mutex>
#include <random>
#include <string>

namespace OpenViBE {
namespace {

// An error within this time of the previous one is taken to follow from it (a box fails, then the player).
constexpr double FOLLOWS_WITHIN_S = 1.0;

std::string ordinal(const size_t n)
{
	const size_t lastTwo = n % 100, last = n % 10;
	const char* suffix   = (lastTwo >= 11 && lastTwo <= 13) ? "th" : last == 1 ? "st" : last == 2 ? "nd" : last == 3 ? "rd" : "th";
	return std::to_string(n) + suffix;
}

const std::array<const char*, 6> FIRST = {
	"I am so very sorry:",
	"My sincere apologies:",
	"I am terribly sorry:",
	"Please forgive me:",
	"I regret to report:",
	"Oh dear, I am sorry:"
};

const std::array<const char*, 4> FOLLOWS = {
	"And, sorry again:",
	"I am afraid it does not end there:",
	"Following from that, I apologise:",
	"One more thing, I am sorry to say:"
};

// "#" is replaced by the ordinal of the occurrence ("3rd").
const std::array<const char*, 4> SEEN_BEFORE = {
	"Sorry again, this happened before:",
	"I am afraid this is the # time:",
	"This again, I do apologise:",
	"Once more, and I am sorry for it (the # time):"
};

struct SState
{
	std::mutex mutex;
	std::mt19937 random{std::random_device{}()};
	std::map<size_t, size_t> countByType;
	bool hasPrevious = false;
	std::chrono::steady_clock::time_point previous;
	std::string lastText;
};

SState& state()
{
	static SState s;  // one per process: this library is loaded once, and every plugin calls into it
	return s;
}

}  // namespace

CString errorApologyText(const EErrorContext context, const size_t occurrence, const size_t pick)
{
	std::string text;
	switch (context)
	{
		case EErrorContext::FollowsAnother: text = FOLLOWS[pick % FOLLOWS.size()]; break;
		case EErrorContext::SeenBefore: text = SEEN_BEFORE[pick % SEEN_BEFORE.size()]; break;
		case EErrorContext::First:
		default: text = FIRST[pick % FIRST.size()]; break;
	}
	const size_t hash = text.find('#');
	if (hash != std::string::npos) { text.replace(hash, 1, ordinal(occurrence)); }
	return CString(text.c_str());
}

CString errorApology(const size_t errorType)
{
	SState& s = state();
	std::lock_guard<std::mutex> lock(s.mutex);

	const auto now          = std::chrono::steady_clock::now();
	const bool follows      = s.hasPrevious && std::chrono::duration<double>(now - s.previous).count() < FOLLOWS_WITHIN_S;
	const size_t occurrence = ++s.countByType[errorType];
	s.hasPrevious           = true;
	s.previous              = now;

	const EErrorContext context = follows ? EErrorContext::FollowsAnother : occurrence > 1 ? EErrorContext::SeenBefore : EErrorContext::First;

	// A different phrasing from the previous apology, so that two errors in a row do not read the same.
	CString text = errorApologyText(context, occurrence, s.random());
	for (int i = 0; i < 8 && std::string(text.toASCIIString()) == s.lastText; ++i) { text = errorApologyText(context, occurrence, s.random()); }
	s.lastText = text.toASCIIString();
	return text;
}

}  // namespace OpenViBE
