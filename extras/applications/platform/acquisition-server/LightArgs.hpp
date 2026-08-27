
#pragma once

// SPDX-License-Identifier: MIT

/**
 * @file LightArgs.h
 * @brief A lightweight, header-only, zero-copy command line argument parser.
 *
 * This library leverages std::string_view to parse argc/argv directly without
 * unnecessary allocations.
 *
 * Supports short (-f) and long (--file) flags automatically.
 *
 * @author Florent LERAY <florent.leray@gmail.com>
 * @version 0.1.0
 * @date 2025-12-27
 * @copyright Copyright (c) 2025 Florent LERAY
 * @license MIT License. See LICENSE file in the project root for full license information.
 */


#include <iostream>
#include <vector>
#include <string_view>
#include <string>
#include <map>
#include <optional>
#include <charconv>
#include <type_traits>
#include <iomanip>
#include <cstdlib> // Pour std::wcstombs

namespace LightArgs
{
    /**
     * @brief Information structure for a registered argument.
     */
    struct ArgInfo
    {
        char shortTag;                ///< The short char (e.g., 'p' for -p). 0 if unused.
        std::string_view longTag;     ///< The long string (e.g., "port" for --port). Empty if unused.
        std::string_view description; ///< The help description.
        bool requiresValue;           ///< True if it expects a value.
    };

    /**
     * @brief A lightweight, modern command line parser.
     */
    class Parser
    {
    public:
        /**
         * @brief Standard constructor.
         * @details It's for char-based arguments (Linux/Mac/Standard Windows).
         * @note Zero-copy: Views point directly to argv memory.
         */
        Parser(int argc, char* argv[])
        {
            PrepareArgs(argc, argv);
        }

#if defined(_WIN32) || defined(_WIN64)
        /**
         * @brief Windows-specific constructor for wide-char arguments (wmain).
         * @note Performs conversion from UNICODE UTF-16/Wide to UTF-8 (allocates memory).
         */
        Parser(int argc, wchar_t* argv[])
        {
            PrepareArgsWindows(argc, argv);
        }
#endif

        /**
         * @brief Registers a boolean flag (e.g., -v, --verbose).
         * @param shortTag Single character for short version ('v'). Use '\0' if none.
         * @param longTag String for long version ("verbose"). Use "" if none.
         * @param desc Description for help.
         */
        void RegisterFlag(char shortTag, std::string_view longTag, std::string_view desc)
        {
            ArgInfo info = { shortTag, longTag, desc, false };
            m_registeredArgs.push_back(info);
        }

        void RegisterFlag(std::string_view longTag, std::string_view desc) { RegisterFlag('\0', longTag, desc); }
        void RegisterFlag(char shortTag, std::string_view desc) { RegisterFlag(shortTag, "", desc); }

        /**
         * @brief Registers an option requiring a value (e.g., -p 8080, --port 8080).
         */
        void RegisterOption(char shortTag, std::string_view longTag, std::string_view desc)
        {
            ArgInfo info = { shortTag, longTag, desc, true };
            m_registeredArgs.push_back(info);
        }
        
        void RegisterOption(std::string_view longTag, std::string_view desc) { RegisterOption('\0', longTag, desc); }
        void RegisterOption(char shortTag, std::string_view desc) { RegisterOption(shortTag, "", desc); }

        /**
         * @brief Parses the arguments.
         * Handles both -s and --long formats interchangeably.
         */
        [[nodiscard]] bool Parse()
        {
            bool success = true;
            size_t i = 1;
            size_t count = m_args.size();

            while (i < count && success)
            {
                std::string_view current = m_args[i];
                const ArgInfo* foundInfo = nullptr;

                // 1. Identify the flag type and search in registered args
                if (current.starts_with("--") && current.size() > 2)
                {
                    // Long flag case
                    std::string_view key = current.substr(2); // Remove --
                    foundInfo = FindInfoByLongTag(key);
                }
                else if (current.starts_with("-") && current.size() > 1 && !current.starts_with("--"))
                {
                    // Short flag case
                    char key = current[1]; // Get char after -
                    foundInfo = FindInfoByShortTag(key);
                }

                // 2. Process found argument
                if (foundInfo != nullptr)
                {
                    // We use the longTag as the primary key for storage if it exists,
                    // otherwise we convert the shortTag to a string key.
                    // This normalizes storage: -p stores value under "port" if possible.
                    std::string_view storageKey = (!foundInfo->longTag.empty())
                        ? foundInfo->longTag
                        : std::string_view(&foundInfo->shortTag, 0); // Hacky for char key, dealt with logic below

                    // Special handling to create a stable key for map if only short tag exists is tricky with views.
                    // For simplicity in this view-based lib, users SHOULD provide long tags for ease of query,
                    // or we rely on the search method `Get` to resolve keys.
                    // HERE: We simply store "detected" presence. The Get() method will do the lookup logic.

                    if (foundInfo->requiresValue)
                    {
                        if ((i + 1) < count)
                        {
                            std::string_view val = m_args[i + 1];
                            if (!val.starts_with("-"))
                            {
                                StoreValue(foundInfo, val);
                                i = i + 1;
                            }
                            else
                            {
                                std::cerr << "Error: Option " << current << " expects a value." << std::endl;
                                success = false;
                            }
                        }
                        else
                        {
                            std::cerr << "Error: Option " << current << " expects a value at the end." << std::endl;
                            success = false;
                        }
                    }
                    else
                    {
                        StoreFlag(foundInfo);
                    }
                }
                else // 3. Handle Positional or Unknown
                {
                    if (!current.starts_with("-"))
                    {
                        m_positionalArgs.push_back(current);
                    }
                    else
                    {
                        std::cerr << "Unknown argument: " << current << std::endl;
                        success = false;
                    }
                }

                i = i + 1;
            }

            return success;
        }

        /**
         * @brief Check if flag is present (by long name or short char).
         */
        [[nodiscard]] bool HasFlag(std::string_view name) const
        {
            return m_activeFlags.contains(name) || (name.size() == 1 && m_activeShortFlags.contains(name[0]));
        }

        /**
         * @brief Get value.
         * @detail It's by long name or short char.
         */
        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[nodiscard]] std::optional<T> Get(std::string_view name) const
        {
            std::optional<T> result = std::nullopt;
            std::string_view sv = "";
            bool found = false;

            // Try finding by string key
            if (m_optionValues.contains(name))
            {
                sv = m_optionValues.at(name);
                found = true;
            }
            // Try finding by short char if name is 1 char
            else if (name.size() == 1 && m_shortOptionValues.contains(name[0]))
            {
                sv = m_shortOptionValues.at(name[0]);
                found = true;
            }

            if (found)
            {
                T tempValue = {};
                auto res = std::from_chars(sv.data(), sv.data() + sv.size(), tempValue);
                if (res.ec == std::errc())
                {
                    result = tempValue;
                }
            }
            return result;
        }

        /**
         * @brief Get string value.
         */
        [[nodiscard]] std::optional<std::string_view> GetString(std::string_view name) const
        {
            if (m_optionValues.contains(name)) return m_optionValues.at(name);
            if (name.size() == 1 && m_shortOptionValues.contains(name[0])) return m_shortOptionValues.at(name[0]);
            return std::nullopt;
        }

        /**
         * @brief Generates and prints the help message automatically.
         */
        void PrintHelp() const
        {
            std::string_view appName = (!m_args.empty()) ? m_args[0] : "app";
            std::cout << "Usage: " << appName << " [options] [arguments]\n\nOptions:\n";

            for (const auto& arg : m_registeredArgs)
            {
                std::cout << "  ";
                // Print short tag
                if (arg.shortTag != '\0') std::cout << "-" << arg.shortTag;
                else                      std::cout << "  ";

                // Print separator
                if (arg.shortTag != '\0' && !arg.longTag.empty()) std::cout << ", ";
                else                                              std::cout << "  ";

                // Print long tag
                if (!arg.longTag.empty()) std::cout << "--" << std::left << std::setw(20) << arg.longTag;
                else                      std::cout << std::left << std::setw(22) << "";

                std::cout << arg.description << "\n";
            }
            std::cout << std::endl;
        }



    private:
        std::vector<std::string_view> m_args; // Unified storage for views (points to argv OR m_convertedStorage)
        std::vector<std::string> m_convertedStorage; // Helper storage for converted strings (only used if wchar_t constructor is called)

        std::vector<ArgInfo> m_registeredArgs;

        std::map<std::string_view, std::string_view> m_optionValues;
        std::map<char, std::string_view> m_shortOptionValues;

        std::map<std::string_view, bool> m_activeFlags;
        std::map<char, bool> m_activeShortFlags;

        std::vector<std::string_view> m_positionalArgs;



    private:
        const ArgInfo* FindInfoByLongTag(std::string_view tag) const
        {
            const ArgInfo* found = nullptr;
            size_t j = 0;
            while (j < m_registeredArgs.size() && found == nullptr)
            {
                if (m_registeredArgs[j].longTag == tag) found = &m_registeredArgs[j];
                j = j + 1;
            }
            return found;
        }

        const ArgInfo* FindInfoByShortTag(char tag) const
        {
            const ArgInfo* found = nullptr;
            size_t j = 0;
            while (j < m_registeredArgs.size() && found == nullptr)
            {
                if (m_registeredArgs[j].shortTag == tag) found = &m_registeredArgs[j];
                j = j + 1;
            }
            return found;
        }

        void StoreValue(const ArgInfo* info, std::string_view val)
        {
            // Store under both keys if they exist so Get("p") and Get("port") both work
            if (!info->longTag.empty()) m_optionValues[info->longTag] = val;
            if (info->shortTag != '\0') m_shortOptionValues[info->shortTag] = val;
        }

        void StoreFlag(const ArgInfo* info)
        {
            if (!info->longTag.empty()) m_activeFlags[info->longTag] = true;
            if (info->shortTag != '\0') m_activeShortFlags[info->shortTag] = true;
        }

        /**
         * @brief Prepares views from standard char* array.
         */
        void PrepareArgs(int argc, char* argv[])
        {
            m_args.reserve(argc);
            for (int i = 0; i < argc; ++i)
            {
                m_args.push_back(argv[i]);
            }
        }

#if defined(_WIN32) || defined(_WIN64)
        /**
         * @brief Converts and stores wchar_t* args as UTF-8 std::string.
         */
        void PrepareArgsWindows(int argc, wchar_t* argv[])
        {
            m_convertedStorage.reserve(argc);
            m_args.reserve(argc);

            for (int i = 0; i < argc; ++i)
            {
                std::wstring_view wideArg = argv[i];

                // Prepare buffer for conversion
                // Note: MB_CUR_MAX is standard C, usually sufficient. 
                // For a robust strict UTF-8 lib, one might use WideCharToMultiByte,
                // but std::wcstombs is the standard portable "light" way.
                std::string s(wideArg.length() * MB_CUR_MAX + 1, '\0');

                size_t reqSize = 0;
                wcstombs_s(&reqSize, nullptr, 0, argv[i], 0);
                if (reqSize > 0)
                {
                    s.resize(reqSize);
                    wcstombs_s(&reqSize, s.data(), s.size(), argv[i], _TRUNCATE);
                    if (reqSize > 0) s.resize(reqSize - 1); // remove null
                    m_convertedStorage.push_back(std::move(s));
                    m_args.push_back(m_convertedStorage.back());
                }
                else
                {
                    // Fallback for conversion error
                    m_args.push_back("");
                }
            }
        }
#endif
    };
}
