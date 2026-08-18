///-------------------------------------------------------------------------------------------------
/// 
/// \file TProgramOptions.hpp
/// \author Charles Garraud.
/// \version 1.0.
/// \date 25/01/2016.
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

#include <boost/variant.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

#if defined TARGET_OS_Windows
#include <codecvt>
#include <locale>
#include <Windows.h>
#endif

namespace OpenViBE {
/**
* \brief Helper class for ProgramOptions type checking
* \ingroup ScenarioPlayer
*
* This class provides aliases to types currently handled by ProgramOptions class
* as well as type checking meta-programming features.
*
*/
struct SProgramOptionsTraits
{
    // add a new type here (type must be default constructible)
    // and add the corresponding callback in ProgramOptions

    // types handled by ProgramOptions
    using TokenPair = std::pair<std::string, std::string>;
    using String = std::string;
    using Integer = int;
    using Float = double;

    using StringList = std::vector<std::string>;
    using IntegerList = std::vector<int>;
    using FloatList = std::vector<double>;
    using TokenPairList = std::vector<TokenPair>;
};

namespace ProgramOptionsUtils {
// template meta-programming features

/* Base type that represents a true value at compile-time */
struct STrueType
{
    static const bool VALUE = true;
};

/* Base type that represents a false value at compile-time */
struct SFalseType
{
    static const bool VALUE = false;
};

/* SIsCompliant is used to statically check if a type is compliant with the handled types */
template <typename T>
struct SIsCompliant : SFalseType { };

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::TokenPair> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::TokenPairList> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::String> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::StringList> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::Integer> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::IntegerList> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::Float> : STrueType {};

/* Specialization of the template for compliant types */
template <>
struct SIsCompliant<SProgramOptionsTraits::FloatList> : STrueType {};

/* SIsSignatureCompliant is used to statically checked a list of type is compliant with handled type */
template <typename... TList>
struct SIsSignatureCompliant;

/* Specialization for empty list */
template <>
struct SIsSignatureCompliant<> : STrueType {};

/* Specialization for non-empty list */
template <typename THead, typename... TTail>
struct SIsSignatureCompliant<THead, TTail...> : std::conditional<SIsCompliant<THead>::VALUE, SIsSignatureCompliant<TTail...>, SFalseType>::type { };

/* SIsIn is used to statically check if a type T is in a list of types TList*/
template <typename T, typename... TList>
struct SIsIn; // interface

/* Specialization for empty list */
template <typename T>
struct SIsIn<T> : SFalseType {};

/* Specialization for list where 1st element is a match */
template <typename T, typename... TTail>
struct SIsIn<T, T, TTail...> : STrueType {};

/* Specialization for list of many elements */
template <typename T, typename THead, typename... TTail>
struct SIsIn<T, THead, TTail...> : SIsIn<T, TTail...> {};

/* SHasDuplicate is used to statically check if a list of types has duplicates*/
template <typename... TList>
struct SHasDuplicate;

/* Specialization for empty list */
template <>
struct SHasDuplicate<> : SFalseType {};

/* Specialization for 1-element list */
template <typename T>
struct SHasDuplicate<T> : SFalseType {};

/* Specialization for list of many elements */
template <typename THead, typename... TTail>
struct SHasDuplicate<THead, TTail...> : std::conditional<SIsIn<THead, TTail...>::VALUE, STrueType, SHasDuplicate<TTail...>>::type { };
}  // namespace ProgramOptionsUtils

/**
* \class ProgramOptions
* \author Charles Garraud (INRIA)
* \date 2016-01-25
* \brief Command-line options parser
* \ingroup ScenarioPlayer
*
* This class is a basic class used to parse program options.
* Input format requirements:
* - Option prefix must be '-' or '--' e.g. cmd -help or --help
* - Option value assignment must be set with '=': e.g. cmd -config=myfile.txt
* - Option value that consists of pair are set with (): e.g. cmd -newToken=(key,value)
* .
*
* Template parameters are the list of option types that can be handled by the class.
* This list of options must comply to types defined in ProgramOptionsTrait.\n
*
* Parsing options occurs in 2 steps:
* - Populating the list of possible options with simple options (e.g. --help) and value options (e.g. --option=value)
* - Parsing options from command line
* .
* 
* \todo The parser has only be tested for the player use. It needs more in-depth testing
* to be used in another context. Moreover, it should be extended to accept any type.
*
* \note The implementation is trivial. Prefer the use of robust and fully featured
* boost program_options compiled library if possible.
*
*/
template <typename TFirst, typename... TTypes>
class ProgramOptions final
{
public:
    // static assert are used to raise understandable errors at compile time
    static_assert(!ProgramOptionsUtils::SHasDuplicate<TFirst, TTypes...>::VALUE, "Duplicates in the type list");
    static_assert(ProgramOptionsUtils::SIsSignatureCompliant<TFirst, TTypes...>::VALUE, "TTypes not handled by ProgramOptions");

    /**
    * Struct used to store used-defined option parameters.
    */
    struct SOptionDesc // using a struct allows more extensibility than method parameters
    {
        /** Option shortname (e.g. h for help) */
        std::string shortName;
        /** Option description used for printing option list */
        std::string desc;
    };

    /**
    * \brief Add global description to the list of options
    * \param[in] desc the global description in printable format
    *
    * The global description is used as additional printable documentation
    * when printOptionsDesc() is called.
    *
    */
    void SetGlobalDesc(const std::string& desc);

    /**
    * \brief Add a simple option to the internal dictionary
    * \param[in] name the option name
    * \param[in] optionDesc the option description
    *
    * Simple options are option withou value (e.g. --help or --version)
    *
    */
    void AddSimpleOption(const std::string& name, const SOptionDesc& optionDesc);


    /**
    * \brief Add a value option to the internal dictionary
    * \param[in] name the option name
    * \param[in] optionDesc the option description
    * 
    * Template paramter T: The type of the option to be added
    */
    template <typename T>
    void AddValueOption(const std::string& name, const SOptionDesc& optionDesc);

    /**
    * \brief Parse command line options
    * \pre addSimpleOption and addValueOption must be called to populate options dictionary
    * \param[in] argc number of arguments
    * \param[in] argv pointer to the list of arguments
    * \return: false if an error occurred during parsing, true otherwise
    */
    bool Parse(int argc, char** argv);

    /**
    * \brief Check if an option was parsed or not
    * \pre Must be called after parse()
    * \param[in] name the option name
    * \return: true if the option was parsed, false otherwise
    */
    bool HasOption(const std::string& name) const;

    /**
    * \brief Get option value
    * \pre hasOption() should be called to ensure the option is available
    * \param[in] name the option name
    * \return the option value (will be the default value if the option was not parsed)
    *
    * Template paramter T: the type of the option to retrieve (must match the type used to set
    * the option with AddValueOption())
    */
    template <typename T>
    T GetOptionValue(const std::string& name) const;


    /**
    * \brief print all option descriptions
    */
    void PrintOptionsDesc() const;

private:
    // The visitor allows us to apply the correct parsing
    // for any type (see boost::variant documentation for more details).
    // Presently, it is pretty ugly. This should be refactored for 
    // as there is a lot of behavorial redundancy.
    class OptionVisitor : public boost::static_visitor<>
    {
    public:
        explicit OptionVisitor(std::string& value) : m_value(value) { }

        void operator()(SProgramOptionsTraits::Integer& operand) const { operand = std::stoi(m_value); }
        void operator()(SProgramOptionsTraits::Float& operand) const { operand = std::stod(m_value); }
        void operator()(SProgramOptionsTraits::String& operand) const { operand = m_value; }
        void operator()(SProgramOptionsTraits::TokenPair& operand) const { operand = this->parsePair(m_value); }
        void operator()(SProgramOptionsTraits::IntegerList& operand) const { operand.push_back(std::stoi(m_value)); }
        void operator()(SProgramOptionsTraits::FloatList& operand) const { operand.push_back(std::stod(m_value)); }
        void operator()(SProgramOptionsTraits::StringList& operand) const { operand.push_back(m_value); }
        void operator()(SProgramOptionsTraits::TokenPairList& operand) const { operand.push_back(this->parsePair(m_value)); }

    private:
        static SProgramOptionsTraits::TokenPair parsePair(const std::string& str);

        std::string& m_value;
    };

    using OptionValue = boost::variant<TFirst, TTypes...>;

    // the pair contais a boolean to quickly know if an option
    // is a simple option or a value option
    using FullOptionDesc = std::pair<bool, SOptionDesc>;

    std::string m_globalDesc;
    std::map<std::string, FullOptionDesc> m_descs;
    std::map<std::string, OptionValue> m_values;
    std::vector<std::string> m_options;
};

///////////////////////////////////////////
/////// Definition ProgramOptions /////////
///////////////////////////////////////////

template <typename TFirst, typename... TTypes>
void ProgramOptions<TFirst, TTypes...>::SetGlobalDesc(const std::string& desc) { m_globalDesc = desc; }

template <typename TFirst, typename... TTypes>
void ProgramOptions<TFirst, TTypes...>::AddSimpleOption(const std::string& name, const SOptionDesc& optionDesc)
{
    m_descs[name] = std::make_pair(true, optionDesc);
}

template <typename TFirst, typename... TTypes>
template <typename T>
void ProgramOptions<TFirst, TTypes...>::AddValueOption(const std::string& name, const SOptionDesc& optionDesc)
{
    m_descs[name] = std::make_pair(false, optionDesc);

    T defaultValue{ }; // with this implementation, only default constructible type can be added 
    m_values[name] = defaultValue;
}

template <typename TFirst, typename... TTypes>
bool ProgramOptions<TFirst, TTypes...>::HasOption(const std::string& name) const
{
    return std::find(m_options.begin(), m_options.end(), name) != m_options.end();
}

template <typename TFirst, typename... TTypes>
template <typename T>
T ProgramOptions<TFirst, TTypes...>::GetOptionValue(const std::string& name) const
{
    T value{ };

    try { value = boost::get<T>(m_values.at(name)); }
    catch (const std::exception& e) { std::cerr << "ERROR: Caught exception during option value retrieval: " << e.what() << std::endl; }

    return value;
}

template <typename TFirst, typename... TTypes>
bool ProgramOptions<TFirst, TTypes...>::Parse(const int argc, char** argv)
{
    std::vector<std::string> args;
#if defined TARGET_OS_Windows
    int nArg;
    const LPWSTR* argListUtf16 = CommandLineToArgvW(GetCommandLineW(), &nArg);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    for (int i = 0; i < nArg; ++i) { args.push_back(converter.to_bytes(argListUtf16[i])); }
#else
        args = std::vector<std::string>(argv, argv + argc);
#endif

    for (int i = 1; i < argc; ++i) {
        std::string arg     = args[i];
        const auto argSplit = arg.find_first_of('=');       // = is the separator for value option
        std::string key;

        if (argSplit == std::string::npos) { key = arg; }   // simple option
        else { key = arg.substr(0, argSplit); }             // value option

        // first check if the key exists
        auto keyMatch = std::find_if(m_descs.begin(), m_descs.end(), [&](const std::pair<std::string, FullOptionDesc>& p)
        {
            const auto& desc = p.second.second;
            return (("-" + p.first) == key) || (("--" + p.first) == key) || (("-" + desc.shortName) == key) || (("--" + desc.shortName) == key);
        });

        if (keyMatch == m_descs.end()) {
            std::cout << "WARNING: Found unknown option: " << key << std::endl;
            std::cout << "Skipping..." << std::endl;
            continue;
        }

        if (!keyMatch->second.first) { // value option
            if (key == arg) {
                std::cerr << "ERROR: No value set for argument: " << key << std::endl;
                return false;
            }

            std::string val = arg.substr(argSplit + 1, arg.size() - argSplit - 1); // take value part of the arg

            try { boost::apply_visitor(OptionVisitor(val), m_values[keyMatch->first]); }
            catch (const std::exception& e) {
                std::cerr << "ERROR: Caught exception during option parsing: " << e.what() << std::endl;
                std::cerr << "Could not parse option with key = " << key << " and value = " << val << std::endl;
                return false;
            }
        }

        m_options.push_back(keyMatch->first);
    }


    return true;
}

template <typename TFirst, typename... TTypes>
void ProgramOptions<TFirst, TTypes...>::PrintOptionsDesc() const
{
    if (!m_globalDesc.empty()) { std::cout << m_globalDesc << std::endl; }

    std::cout << "TList of available options:\n" << std::endl;

    for (auto& option : m_descs) {
        std::cout << "Option: --" << option.first << std::endl;
        const auto& desc = option.second.second;
        if (!desc.shortName.empty()) { std::cout << "Shortname: --" << desc.shortName << std::endl; }
        std::cout << "Description: " << std::endl;
        std::cout << desc.desc << std::endl << std::endl;
    }
}

///////////////////////////////////////////
/////// Definition Internal Visitor ///////
///////////////////////////////////////////

template <typename TFirst, typename... TTypes>
SProgramOptionsTraits::TokenPair ProgramOptions<TFirst, TTypes...>::OptionVisitor::parsePair(const std::string& str)
{
    const auto split = str.find_first_of(':');
    const auto size  = str.size();

    // (a:b) pattern expected
    // minimal regex std::regex("\\(.+:.+\\)")
    if (!(size >= 5 && str[0] == '(' && str[size - 1] == ')') || split == std::string::npos) {
        throw std::runtime_error("Failed to parse token pair from value: " + str);
    }

    // magic 2 numbers is because substr takes a length as second parameter
    // 2 = remove the last ) + account for the first one
    return std::make_pair(str.substr(1, split - 1), str.substr(split + 1, size - split - 2));
}
}  // namespace OpenViBE
