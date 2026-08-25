/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include <gtest/gtest.h>

#include <memory>

#include <nlohmann/json.hpp>
#include <openvibe/parameters/parameters.h>

namespace {

/// Rebuilds a parameter the way the settings do, through the factory rather than by calling
/// deserialize() by hand.
std::unique_ptr<OpenViBE::Parameters::ParameterTraits> restore(const nlohmann::json& j)
{
	return std::unique_ptr<OpenViBE::Parameters::ParameterTraits>(OpenViBE::Parameters::ParameterFactory::create(j));
}

}	// namespace

//---------------------------------------------------------------------------------------------------
// A stored value outside the stored bounds must come back clamped. It used to be written straight
// into the parameter, so the parameter ended up out of its own range.
//---------------------------------------------------------------------------------------------------
TEST(ParameterTest, IntClampsValueFromJson)
{
	OpenViBE::Parameters::ParameterInt param("Channels", "Channel count", 5, 0, 10);

	nlohmann::json j = param.serialize();
	j["value"]       = 50;

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());

	const auto* asInt = dynamic_cast<OpenViBE::Parameters::ParameterInt*>(restored.get());
	ASSERT_NE(nullptr, asInt);
	EXPECT_EQ(10, asInt->value()) << "the stored value should have been clamped to the stored maximum";
}

TEST(ParameterTest, DoubleClampsValueFromJson)
{
	OpenViBE::Parameters::ParameterDouble param("Gain", "Gain", 1.0, 0.0, 2.0);

	nlohmann::json j = param.serialize();
	j["value"]       = 9.0;

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());

	const auto* asDouble = dynamic_cast<OpenViBE::Parameters::ParameterDouble*>(restored.get());
	ASSERT_NE(nullptr, asDouble);
	EXPECT_DOUBLE_EQ(2.0, asDouble->value()) << "the stored value should have been clamped to the stored maximum";
}

/**
 * The available values have to be read before the value, otherwise the value is checked against a
 * list that is still empty and dropped.
 */
TEST(ParameterTest, ListIntKeepsStoredValueInList)
{
	OpenViBE::Parameters::ParameterListInt param("Chunk", "Samples per chunk", { 4, 8, 16 }, 4);
	param.setValue(8);

	const auto restored = restore(param.serialize());
	ASSERT_NE(nullptr, restored.get());

	const auto* asList = dynamic_cast<OpenViBE::Parameters::ParameterListInt*>(restored.get());
	ASSERT_NE(nullptr, asList);
	EXPECT_EQ(8, asList->value()) << "a stored value that is in the stored list must survive the round trip";
}

TEST(ParameterTest, ListIntRejectsValueNotInList)
{
	OpenViBE::Parameters::ParameterListInt param("Chunk", "Samples per chunk", { 4, 8, 16 }, 4);

	nlohmann::json j = param.serialize();
	j["value"]       = 7;

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());

	const auto* asList = dynamic_cast<OpenViBE::Parameters::ParameterListInt*>(restored.get());
	ASSERT_NE(nullptr, asList);
	EXPECT_NE(7, asList->value()) << "a stored value that is not in the list must not be accepted";
}

TEST(ParameterTest, ListStringRejectsValueNotInList)
{
	OpenViBE::Parameters::ParameterListString param("Policy", "Drift policy", { "Disabled", "Forced" }, "Disabled");

	nlohmann::json j = param.serialize();
	j["value"]       = "SomethingElse";

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());

	const auto* asList = dynamic_cast<OpenViBE::Parameters::ParameterListString*>(restored.get());
	ASSERT_NE(nullptr, asList);
	EXPECT_NE("SomethingElse", asList->value()) << "a stored value that is not in the list must not be accepted";
}

TEST(ParameterTest, MultiSelectDropsSelectionNotInAvailableValues)
{
	OpenViBE::Parameters::ParameterMultiSelect param("Flags", "Flags", { "a", "b", "c" }, { "a", "c" });

	nlohmann::json j    = param.serialize();
	j["value"]          = std::vector<std::string>{ "a", "zzz" };
	j["selectedValues"] = std::vector<std::string>{ "a", "zzz" };

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());

	const auto* asMulti = dynamic_cast<OpenViBE::Parameters::ParameterMultiSelect*>(restored.get());
	ASSERT_NE(nullptr, asMulti);
	const std::vector<std::string> expected{ "a" };
	EXPECT_EQ(expected, asMulti->value()) << "only the selected values that are available must be kept";
}

/**
 * Deserializing sets the value through setValue(), so whoever listens to the parameter is told about
 * it. The value used to be written straight into the parameter and nothing was notified.
 */
TEST(ParameterTest, DeserializeNotifiesListener)
{
	OpenViBE::Parameters::ParameterInt param("Rate", "Sampling rate", 1, 0, 100);

	nlohmann::json j = param.serialize();
	j["value"]       = 42;

	int notifications = 0;
	param.connect([&notifications](std::any) { ++notifications; });

	param.deserialize(j);

	EXPECT_EQ(1, notifications) << "deserializing must notify, like setting the value does";
	EXPECT_EQ(42, param.value());
}

/**
 * Every registered type must survive serialize() followed by the factory, keeping its type, its name,
 * its description and its value.
 */
TEST(ParameterTest, JsonRoundTripsEveryRegisteredType)
{
	std::vector<std::unique_ptr<OpenViBE::Parameters::ParameterTraits>> params;
	params.emplace_back(new OpenViBE::Parameters::ParameterBool("B", "a bool", true));
	params.emplace_back(new OpenViBE::Parameters::ParameterInt("I", "an int", 7, 0, 10));
	params.emplace_back(new OpenViBE::Parameters::ParameterDouble("D", "a double", 1.5, 0.0, 2.0));
	params.emplace_back(new OpenViBE::Parameters::ParameterString("S", "a string", "hello"));
	params.emplace_back(new OpenViBE::Parameters::ParameterColor("C", "a color", "#102030"));
	params.emplace_back(new OpenViBE::Parameters::ParameterColorGradient("G", "a gradient", "0:2,36,58; 100:83,17,20"));
	params.emplace_back(new OpenViBE::Parameters::ParameterListInt("LI", "an int list", { 4, 8, 16 }, 8));
	params.emplace_back(new OpenViBE::Parameters::ParameterListString("LS", "a string list", { "x", "y" }, "y"));
	params.emplace_back(new OpenViBE::Parameters::ParameterMultiSelect("MS", "a multiselect", { "a", "b", "c" }, { "a", "c" }));
	params.emplace_back(new OpenViBE::Parameters::ParameterPath("P", "a path", "/tmp/file.txt"));
	params.emplace_back(new OpenViBE::Parameters::ParameterStringVector("SV", "a string vector", { "Fp1", "Fp2" }));

	for (const auto& param : params) {
		const auto restored = restore(param->serialize());
		ASSERT_NE(nullptr, restored.get()) << "could not rebuild a parameter of type " << param->type();

		EXPECT_EQ(param->type(), restored->type());
		EXPECT_EQ(param->name(), restored->name()) << "for type " << param->type();
		EXPECT_EQ(param->description(), restored->description()) << "for type " << param->type();
		EXPECT_EQ(param->toString(), restored->toString()) << "for type " << param->type();
	}
}

/**
 * A gradient checks the shape of what it is given, so a malformed stored value is refused instead of
 * being taken as is and leaving the parameter disagreeing with the colors it parsed.
 */
TEST(ParameterTest, ColorGradientRejectsMalformedValue)
{
	OpenViBE::Parameters::ParameterColorGradient param("G", "a gradient", "0:2,36,58; 100:83,17,20");

	nlohmann::json j = param.serialize();
	j["value"]       = "not-a-gradient";

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());
	EXPECT_NE("not-a-gradient", restored->toString()) << "a malformed gradient must not be accepted";
}

/**
 * A string vector is stored in json as a real array, so an element may hold the ';' that the single
 * string form of toString() has to escape.
 */
TEST(ParameterTest, StringVectorStoresAnArrayInJson)
{
	OpenViBE::Parameters::ParameterStringVector param("Names", "Channel names", { "Fp1", "a;b", "C3" });

	const nlohmann::json j = param.serialize();
	ASSERT_TRUE(j.at("value").is_array()) << "the value should be a json array, not a joined string";

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());

	const auto* asVector = dynamic_cast<OpenViBE::Parameters::ParameterStringVector*>(restored.get());
	ASSERT_NE(nullptr, asVector);
	const std::vector<std::string> expected{ "Fp1", "a;b", "C3" };
	EXPECT_EQ(expected, asVector->value()) << "an element holding a ';' must survive the round trip";
}

/**
 * The factory refuses what it can not build, rather than returning something half filled.
 */
TEST(ParameterTest, FactoryThrowsOnUnknownType)
{
	nlohmann::json j;
	j["type"]        = "not-a-registered-type";
	j["name"]        = "N";
	j["description"] = "";
	j["value"]       = 0;

	EXPECT_THROW(restore(j), std::runtime_error);
}

/**
 * create() takes the json by const reference. It used to take it by non const reference, so a const
 * json did not bind and the call quietly resolved to create(const std::string&) instead, which threw
 * at run time. This one is as much a compile time check as a run time one.
 */
TEST(ParameterTest, FactoryAcceptsAConstJson)
{
	const OpenViBE::Parameters::ParameterInt param("I", "an int", 3, 0, 10);
	const nlohmann::json j = param.serialize();

	const auto restored = restore(j);
	ASSERT_NE(nullptr, restored.get());
	EXPECT_EQ("3", restored->toString());
}

/**
 * A string literal must still pick the type overload rather than being an ambiguous match for both
 * the std::string and the nlohmann::json one.
 */
TEST(ParameterTest, FactoryAcceptsAStringLiteralType)
{
	const std::unique_ptr<OpenViBE::Parameters::ParameterTraits> param(OpenViBE::Parameters::ParameterFactory::create("int"));
	ASSERT_NE(nullptr, param.get());
	EXPECT_EQ("int", param->type());
}

TEST(ParameterTest, FactoryThrowsWhenTypeIsMissing)
{
	nlohmann::json j;
	j["name"]  = "N";
	j["value"] = 0;

	EXPECT_THROW(restore(j), std::runtime_error);
}
