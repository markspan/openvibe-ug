///-------------------------------------------------------------------------------------------------
/// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.54.
///
/// \file ErrorApologyTest.hpp
/// \brief Tests of the apology that begins every error message.
///-------------------------------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include <openvibe/ErrorApology.hpp>

#include <set>
#include <string>

namespace {
std::string apologyText(const OpenViBE::EErrorContext context, const size_t occurrence, const size_t pick)
{
	return OpenViBE::errorApologyText(context, occurrence, pick).toASCIIString();
}
}  // namespace

TEST(ErrorApology_Tests, every_phrasing_ends_with_a_colon_and_names_the_occurrence)
{
	using OpenViBE::EErrorContext;
	for (const auto context : { EErrorContext::First, EErrorContext::FollowsAnother, EErrorContext::SeenBefore })
	{
		for (size_t pick = 0; pick < 12; ++pick)
		{
			const std::string text = apologyText(context, 3, pick);
			ASSERT_FALSE(text.empty());
			EXPECT_EQ(text.back(), ':') << text;
			EXPECT_EQ(text.find('#'), std::string::npos) << text;
		}
	}
	EXPECT_EQ(apologyText(EErrorContext::SeenBefore, 3, 1), "I am afraid this is the 3rd time:");
	EXPECT_EQ(apologyText(EErrorContext::SeenBefore, 11, 1), "I am afraid this is the 11th time:");
	EXPECT_EQ(apologyText(EErrorContext::SeenBefore, 22, 1), "I am afraid this is the 22nd time:");
	EXPECT_EQ(apologyText(EErrorContext::First, 1, 0), "I am so very sorry:");
}

TEST(ErrorApology_Tests, the_phrasings_differ_per_context)
{
	using OpenViBE::EErrorContext;
	std::set<std::string> first, follows;
	for (size_t pick = 0; pick < 12; ++pick)
	{
		first.insert(apologyText(EErrorContext::First, 1, pick));
		follows.insert(apologyText(EErrorContext::FollowsAnother, 1, pick));
	}
	EXPECT_GT(first.size(), 3u);
	EXPECT_GT(follows.size(), 2u);
	for (const auto& text : follows) { EXPECT_EQ(first.count(text), 0u) << text; }
}

TEST(ErrorApology_Tests, an_error_right_after_another_follows_it_and_two_in_a_row_read_differently)
{
	// These calls come within a second of each other: all but possibly the first follow another error.
	const size_t type = 987654;  // a type no other test uses
	std::string previous = OpenViBE::errorApology(type).toASCIIString();
	std::set<std::string> follows;
	for (size_t pick = 0; pick < 12; ++pick) { follows.insert(apologyText(OpenViBE::EErrorContext::FollowsAnother, 1, pick)); }
	for (int i = 0; i < 20; ++i)
	{
		const std::string text = OpenViBE::errorApology(type).toASCIIString();
		EXPECT_EQ(follows.count(text), 1u) << text;
		EXPECT_NE(text, previous);
		previous = text;
	}
}
