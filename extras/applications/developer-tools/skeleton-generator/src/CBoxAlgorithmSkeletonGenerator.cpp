#include "CBoxAlgorithmSkeletonGenerator.hpp"

#include <iostream>
#include <sstream>

#include <glib/gstdio.h>
#include <cstdio>

#include <boost/regex.hpp>

#include <ctime>
#include <cmath>
#include <fstream>

#define TO_GTK_UTF8(s) g_locale_to_utf8((s), -1, nullptr, nullptr, nullptr)

namespace OpenViBE {
namespace SkeletonGenerator {

//Modified version of https://stackoverflow.com/questions/36435204/converting-a-cstring-to-camelcase
static std::string camelCase(const std::string& in)
{
    std::string out(in);
    bool active = true;

    for (size_t i = 0; i < out.length();) {
        if (std::isalpha(out[i])) {
            if (active) {
                out[i] = std::toupper(out[i]);
                active = false;
            }
            else { out[i] = std::tolower(out[i]); }
            i++;
        }
        else if (out[i] == ' ') {
            active = true;
            out.erase(i, 1);
        } else if ( out[i] == '/') {
            // '/' may appear in categories. We discard everything after it.
            out.erase(i, out.length() - i);
            break;
        }
    }
    return out;
}

static std::vector<std::string> ExtractKeys(const std::map<std::string, Kernel::EParameterType>& in)
{
    std::vector<std::string> res;
    for (const auto& element : in) { res.push_back(element.first); }
    return res;
}

bool CDummyAlgoProto::addInputParameter(const CIdentifier& /*id*/, const CString& name, const Kernel::EParameterType typeID, const CIdentifier& /*subTypeID*/)
{
    m_Inputs[std::string(name.toASCIIString())] = typeID;
    return true;
}

bool CDummyAlgoProto::addOutputParameter(const CIdentifier& /*id*/, const CString& name, const Kernel::EParameterType typeID, const CIdentifier& /*subTypeID*/)
{
    m_Outputs[std::string(name.toASCIIString())] = typeID;
    return true;
}

bool CDummyAlgoProto::addInputTrigger(const CIdentifier& /*id*/, const CString& name)
{
    m_InputTriggers.push_back(std::string(name.toASCIIString()));
    return true;
}

bool CDummyAlgoProto::addOutputTrigger(const CIdentifier& /*id*/, const CString& name)
{
    m_OutputTriggers.push_back(std::string(name.toASCIIString()));
    return true;
}

//-----------------------------------------------------------------------
static void CheckCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonCheckCB(); }
static void TooltipCB(GtkButton* button, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonTooltipCB(button); }
static void OkCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonOkCB(); }
static void AddInputCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonAddInputCB(); }
static void RemInputCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonRemoveGeneric("sg-box-inputs-treeview"); }
static void AddOutputCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonAddOutputCB(); }
static void RemOutputCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonRemoveGeneric("sg-box-outputs-treeview"); }
static void AddSettingCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonAddSettingCB(); }
static void RemSettingCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonRemoveGeneric("sg-box-settings-treeview"); }
static void AddAlgorithmCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonAddAlgorithmCB(); }
static void RemAlgorithmCB(GtkButton* /*button*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonRemoveGeneric("sg-box-algorithms-treeview"); }
static void AlgorithmSelectedCB(GtkComboBox* comboBox, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonAlgorithmSelectedCB(gtk_combo_box_get_active(comboBox)); }

static void ExitCB(GtkButton* /*button*/, void* data)
{
    static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ButtonExitCB();
    gtk_exit(0);
}
//-----------------------------------------------------------------------

extern "C" G_MODULE_EXPORT void EntryModifiedCB(GtkWidget* /*widget*/, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->SetSensitivity("sg-box-ok-button", false); }

extern "C" G_MODULE_EXPORT void ListenerCheckbuttonToggledCB(GtkWidget* widget, void* data) { static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->ToggleListenerCheckbuttonsStateCB((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) > 0)); }

extern "C" G_MODULE_EXPORT void ProcessingMethodClockToggled(GtkWidget* widget, void* data)
{
    // toggle clock frequency state
    static_cast<CBoxAlgorithmSkeletonGenerator*>(data)->SetSensitivity("sg-box-process-frequency-spinbutton",
                                                                       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) > 0);
}
//-----------------------------------------------------------------------

void CBoxAlgorithmSkeletonGenerator::ButtonExitCB()
{
    getCommonParameters();
    getCurrentParameters();
    if (!cleanConfigurationFile(m_configFile)) { return; }
    saveCommonParameters(m_configFile);
    save(m_configFile);

    getLogManager() << Kernel::LogLevel_Info << "All entries saved in [" << m_configFile << "]. Exiting.\n";
}

void CBoxAlgorithmSkeletonGenerator::ToggleListenerCheckbuttonsStateCB(const bool state) const
{
    SetSensitivity("sg-box-listener-input-added-checkbutton", state);
    SetSensitivity("sg-box-listener-input-removed-checkbutton", state);
    SetSensitivity("sg-box-listener-input-type-checkbutton", state);
    SetSensitivity("sg-box-listener-input-name-checkbutton", state);
    SetSensitivity("sg-box-listener-input-connected-checkbutton", state);
    SetSensitivity("sg-box-listener-input-disconnected-checkbutton", state);

    SetSensitivity("sg-box-listener-output-added-checkbutton", state);
    SetSensitivity("sg-box-listener-output-removed-checkbutton", state);
    SetSensitivity("sg-box-listener-output-type-checkbutton", state);
    SetSensitivity("sg-box-listener-output-name-checkbutton", state);
    SetSensitivity("sg-box-listener-output-connected-checkbutton", state);
    SetSensitivity("sg-box-listener-output-disconnected-checkbutton", state);

    SetSensitivity("sg-box-listener-setting-added-checkbutton", state);
    SetSensitivity("sg-box-listener-setting-removed-checkbutton", state);
    SetSensitivity("sg-box-listener-setting-type-checkbutton", state);
    SetSensitivity("sg-box-listener-setting-name-checkbutton", state);
    SetSensitivity("sg-box-listener-setting-connected-checkbutton", state);
    SetSensitivity("sg-box-listener-setting-disconnected-checkbutton", state);
}

void CBoxAlgorithmSkeletonGenerator::ButtonCheckCB()
{
    getLogManager() << Kernel::LogLevel_Info << "Extracting values... \n";
    //Author and Company
    getCommonParameters();
    //Box generator entries
    getCurrentParameters();

    getLogManager() << Kernel::LogLevel_Info << "Checking values... \n";

    bool success = true;

    std::stringstream ss;
    ss << "----- STATUS -----\n";

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // Box Description
    //-------------------------------------------------------------------------------------------------------------------------------------------//

    if (!isStringValid(m_Name)) {
        OV_WARNING_K("-- box name: INVALID (" << m_Name << ")");
        ss << "[FAILED] No name found. Please provide a name for the box (all characters allowed).\n";
        success = false;
    }
    else {
        //m_Name = ensureSedCompliancy(m_Name);
        getLogManager() << Kernel::LogLevel_Info << "-- box name: VALID (" << m_Name << ")\n";
        ss << "[   OK   ] Valid box name.\n";
    }

    const auto checkRegex = [&success, &ss, this](const std::string& str, const boost::regex& regex, const std::string& successText, const std::string& errorText)
    {
        if (regex_match(str, regex)) {
            getLogManager() << Kernel::LogLevel_Info << "-- " << successText;
            ss << "[   OK   ] " << successText;
        }
        else {
            OV_WARNING_K("-- " << errorText);
            ss << "[FAILED] " << errorText;
            success = false;
        }
    };

    checkRegex(m_ClassName, boost::regex("([a-z]|[A-Z])+([a-z]|[A-Z]|[0-9]|[_])*", boost::regex::perl), "Valid class name (" + m_ClassName + ").\n",
               "Invalid class name (" + m_ClassName + "). Please provide a class name using lower/upper case letters, numbers or underscores.\n");

    checkRegex(m_Category, boost::regex("([a-z]|[A-Z])+([a-z]|[A-Z]|[ ]|[/])*", boost::regex::perl), "Valid category (" + m_Category + ").\n",
               "Invalid category (" + m_Category + "). Please provide a category using only letters and spaces (for sub-category, use '/' separator).\n");

    checkRegex(m_Version, boost::regex("([0-9])+([a-z]|[A-Z]|[0-9]|[\\.])*", boost::regex::perl), "Valid box version (" + m_Version + ").\n",
               "Invalid box version (" + m_Version + "). Please use a number followed by either numbers, letters or '.'.\n");

    //m_ShortDesc = ensureSedCompliancy(m_ShortDesc);
    getLogManager() << Kernel::LogLevel_Info << "-- short description: VALID (" << m_ShortDesc << ")\n";
    ss << "[   OK   ] Valid short description.\n";

    if (m_DetailedDesc.length() < 500) {
        //m_DetailedDesc = ensureSedCompliancy(m_DetailedDesc);
        getLogManager() << Kernel::LogLevel_Info << "-- detailed description: VALID (" << m_DetailedDesc << ")\n";
        ss << "[   OK   ] Valid detailed description.\n";
    }

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // Box INPUTS OUTPUTS and SETTINGS
    //-------------------------------------------------------------------------------------------------------------------------------------------//

    auto setLogHeader = [&ss, this](const size_t collectionSize, const std::string& typeText)
    {
        if (collectionSize != 0) {
            ss << "Checking " << typeText << "... \n";
            getLogManager() << Kernel::LogLevel_Info << "-- checking " << typeText << "s...\n";
        }
        else {
            ss << "[----//----] No " << typeText << " specified.\n";
            getLogManager() << Kernel::LogLevel_Info << "No " << typeText << " specified.\n";
        }
    };

    const auto checkCollection = [&success, &setLogHeader, &ss, this](std::vector<SIOS>& collection, const std::string& type, const std::string& solution)
    {
        std::string capitalized(type);
        capitalized[0] = std::toupper(capitalized[0]);
        setLogHeader(collection.size(), type);
        for (size_t i = 0; i < collection.size(); ++i) {
            if (isStringValid(collection[i].name) && isStringValid(collection[i].type)) {
                collection[i].name = ensureSedCompliancy(collection[i].name);
                getLogManager() << Kernel::LogLevel_Info << "  -- " << capitalized << " " << i << ": [" << collection[i].name << "],["
                        << collection[i].type << "] VALID.\n";
                ss << ">>[   OK   ] Valid " << type << " " << i << " [" << collection[i].name << "]\n";
            }
            else {
                OV_WARNING_K("  -- " << capitalized << " " << i << ": [" << collection[i].name<<"],["<< collection[i].type<< "] INVALID.");
                ss << ">>[FAILED] Invalid " << type << " " << i << ". " << solution << "\n";
                success = false;
            }
        }
    };

    checkCollection(m_Inputs, "input", "Please provide a name and a type for each input.");
    checkCollection(m_Outputs, "output", "Please provide a name and a type for each output.");
    checkCollection(m_Settings, "setting", "Please provide a name, a type and a default value for each setting.");

    //checking the algorithms...
    setLogHeader(m_Algorithms.size(), "algorithm");
    for (size_t i = 0; i < m_Algorithms.size(); ++i) {
        getLogManager() << Kernel::LogLevel_Info << "  -- Algorithm " << i << ": [" << m_Algorithms[i] << "] VALID.\n";
        ss << ">>[   OK   ] Valid algorithm " << i << " [" << m_Algorithms[i] << "]\n";
    }

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    GtkWidget* textview   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-tooltips-textview"));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    if (success) {
        ss << "----- SUCCESS -----\nPress 'Generate!' to generate the files. If you want to modify your choice(s), please press the \"Check\" button again.";
        SetSensitivity("sg-box-ok-button", true);
    }
    else {
        ss << "----- PROCESS FAILED -----\nModify your choices and press the \"Check\" button again.";
        SetSensitivity("sg-box-ok-button", false);
    }

    gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(ss.str().c_str()), -1);
}

void CBoxAlgorithmSkeletonGenerator::ButtonOkCB()
{
    getLogManager() << Kernel::LogLevel_Info << "Generating files, please wait ... \n";
    std::string log       = "Generating files...\n";
    GtkWidget* textview   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-tooltips-textview"));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    bool success = true;

    // we ask for a target directory
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Select the destination folder", nullptr, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, nullptr);

    std::string directory;
    // if the user specified a target directory, it has full priority
    directory = m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_TargetDirectory}");
#ifdef TARGET_OS_Linux
    bool needFilePrefix = false;
#endif
    if (!directory.empty()) {
        getLogManager() << Kernel::LogLevel_Debug << "Target dir user  [" << directory << "]\n";
#ifdef TARGET_OS_Linux
        needFilePrefix = true;
#endif
    }
    else {
        //previous entry
        directory = m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Box_TargetDirectory}");
        if (!directory.empty()) {
            getLogManager() << Kernel::LogLevel_Debug << "Target previous  [" << directory << "]\n";
#ifdef TARGET_OS_Linux
            needFilePrefix = true;
#endif
        }
        else {
            //default path = dist
            getLogManager() << Kernel::LogLevel_Debug << "Target default  [dist]\n";
#ifdef TARGET_OS_Linux
            char* current_dir = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
            if (current_dir) {
              directory = std::string(current_dir) + "/..";
            }
#elif defined TARGET_OS_Windows
            directory = "..";
#endif
        }
    }
#ifdef TARGET_OS_Linux
    if(needFilePrefix) directory = "file://" + directory;
    gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), directory.c_str());
#elif defined TARGET_OS_Windows
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), directory.c_str());
#endif


    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* tmp   = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        m_directory = tmp;
    }
    else {
        getLogManager() << Kernel::LogLevel_Info << "User cancel. Aborting generation.\n";
        log += "User cancel. Aborting generation.\n";
        gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(log.c_str()), -1);
        gtk_widget_destroy(dialog);
        return;
    }
    gtk_widget_destroy(dialog);

    // replace tags in the algorithm description
    if (USE_CODEC_TOOLKIT) {
        for (size_t i = 0; i < m_Algorithms.size(); ++i) {
            std::string algo = m_AlgoDeclaration[m_Algorithms[i]];
            size_t it        = algo.find("@@ClassName@@");
            if (it != std::string::npos) {
                std::string className = "CBoxAlgorithm" + m_ClassName;
                algo.replace(it, 13, className);
                m_AlgoDeclaration[m_Algorithms[i]] = algo;
            }
        }
    }

    std::string date = getDate();
    // we construct the map of substitutions
    std::map<std::string, std::string> substitutions;
    substitutions["@@Author@@"]                                    = m_author;
    substitutions["@@Date@@"]                                      = date;
    substitutions["@@Company@@"]                                   = m_company;
    substitutions["@@Date@@"]                                      = date;
    substitutions["@@BoxName@@"]                                   = m_Name;
    substitutions["@@ClassName@@"]                                 = m_ClassName;
    substitutions["@@RandomIdentifierClass@@"]                     = getRandomIdentifierString();
    substitutions["@@RandomIdentifierDescriptor@@"]                = getRandomIdentifierString();
    substitutions["@@ShortDescription@@"]                          = m_ShortDesc;
    substitutions["@@DetailedDescription@@"]                       = m_DetailedDesc;
    substitutions["@@Category@@"]                                  = m_Category;
    substitutions["@@Namespace@@"]                                 = camelCase(m_Category);
    substitutions["@@Version@@"]                                   = m_Version;
    substitutions["@@StockItemName@@"]                             = m_GtkStockItemName;
    substitutions["@@InputFlagCanAdd@@"]                           = (m_CanAddInputs ? "prototype.addFlag(Kernel::BoxFlag_CanAddInput);" : "//prototype.addFlag(Kernel::BoxFlag_CanAddInput);");
    substitutions["@@InputFlagCanModify@@"]                        = (m_CanModifyInputs ? "prototype.addFlag(Kernel::BoxFlag_CanModifyInput);" : "//prototype.addFlag(Kernel::BoxFlag_CanModifyInput);");
    substitutions["@@OutputFlagCanAdd@@"]                          = (m_CanAddOutputs ? "prototype.addFlag(Kernel::BoxFlag_CanAddOutput);" : "//prototype.addFlag(Kernel::BoxFlag_CanAddOutput);");
    substitutions["@@OutputFlagCanModify@@"]                       = (m_CanModifyOutputs ? "prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);" : "//prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);");
    substitutions["@@SettingFlagCanAdd@@"]                         = (m_CanAddSettings ? "prototype.addFlag(Kernel::BoxFlag_CanAddSetting);" : "//prototype.addFlag(Kernel::BoxFlag_CanAddSetting);");
    substitutions["@@SettingFlagCanModify@@"]                      = (m_CanModifySettings ? "prototype.addFlag(Kernel::BoxFlag_CanModifySetting);" : "//prototype.addFlag(Kernel::BoxFlag_CanModifySetting);");
    substitutions["@@ListenerCommentIn@@"]                         = (m_UseBoxListener ? "" : "/*");
    substitutions["@@ListenerCommentOut@@"]                        = (m_UseBoxListener ? "" : "*/");
    substitutions["@@ListenerInputConnectedComment@@"]             = (m_HasOnInputConnected ? "" : "//");
    substitutions["@@ListenerInputDisconnectedComment@@"]          = (m_HasOnInputDisconnected ? "" : "//");
    substitutions["@@ListenerInputAddedComment@@"]                 = (m_HasOnInputAdded ? "" : "//");
    substitutions["@@ListenerInputRemovedComment@@"]               = (m_HasOnInputRemoved ? "" : "//");
    substitutions["@@ListenerInputTypeChangedComment@@"]           = (m_HasOnInputTypeChanged ? "" : "//");
    substitutions["@@ListenerInputNameChangedComment@@"]           = (m_HasOnInputNameChanged ? "" : "//");
    substitutions["@@ListenerOutputConnectedComment@@"]            = (m_HasOnOutputConnected ? "" : "//");
    substitutions["@@ListenerOutputDisconnectedComment@@"]         = (m_HasOnOutputDisconnected ? "" : "//");
    substitutions["@@ListenerOutputAddedComment@@"]                = (m_HasOnOutputAdded ? "" : "//");
    substitutions["@@ListenerOutputRemovedComment@@"]              = (m_HasOnOutputRemoved ? "" : "//");
    substitutions["@@ListenerOutputTypeChangedComment@@"]          = (m_HasOnOutputTypeChanged ? "" : "//");
    substitutions["@@ListenerOutputNameChangedComment@@"]          = (m_HasOnOutputNameChanged ? "" : "//");
    substitutions["@@ListenerSettingAddedComment@@"]               = (m_HasOnSettingAdded ? "" : "//");
    substitutions["@@ListenerSettingRemovedComment@@"]             = (m_HasOnSettingRemoved ? "" : "//");
    substitutions["@@ListenerSettingTypeChangedComment@@"]         = (m_HasOnSettingTypeChanged ? "" : "//");
    substitutions["@@ListenerSettingNameChangedComment@@"]         = (m_HasOnSettingNameChanged ? "" : "//");
    substitutions["@@ListenerSettingDefaultValueChangedComment@@"] = (m_HasOnSettingDefaultValueChanged ? "" : "//");
    substitutions["@@ListenerSettingValueChangedComment@@"]        = (m_HasOnSettingValueChanged ? "" : "//");
    substitutions["@@ProcessClockComment@@"]                       = (m_HasProcessClock ? "" : "//");
    substitutions["@@ProcessInputComment@@"]                       = (m_HasProcessInput ? "" : "//");
    substitutions["@@ProcessMessageComment@@"]                     = (m_HasProcessMessage ? "" : "//");
    substitutions["@@ProcessClockCommentIn@@"]                     = (m_HasProcessClock ? "" : "/*");
    substitutions["@@ProcessClockCommentOut@@"]                    = (m_HasProcessClock ? "" : "*/");
    substitutions["@@ProcessInputCommentIn@@"]                     = (m_HasProcessInput ? "" : "/*");
    substitutions["@@ProcessInputCommentOut@@"]                    = (m_HasProcessInput ? "" : "*/");
    substitutions["@@ProcessMessageCommentIn@@"]                   = (m_HasProcessMessage ? "" : "/*");
    substitutions["@@ProcessMessageCommentOut@@"]                  = (m_HasProcessMessage ? "" : "*/");
    substitutions["@@ClockFrequency@@"]                            = std::to_string(m_ClockFrequency) + "LL<<32";

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // box.h
    std::string dst     = m_directory + "/CBoxAlgorithm" + m_ClassName + ".hpp";
    std::string tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/box.hpp-skeleton").toASCIIString();

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(log.c_str()), -1);
        success = false;
    }


    auto insertData = [this, &dst](const std::string& collectionType, const std::vector<SIOS>& collection, const bool hasValue)
    {
        const std::string pattern = "\\t\\t@@" + collectionType + "s@@";
        std::string collectionTypeLower(collectionType);
        collectionTypeLower[0] = tolower(collectionTypeLower[0]);

        const std::string errorMsg = "\\/\\/No " + collectionTypeLower + " specified.To add " + collectionTypeLower + "s use :\\n\\/\\/prototype.add"
                                     + collectionType + "(\\\"" + collectionType + "Name\\\", OV_TypeId_XXXX" + (hasValue ? ", \\\"default value\\\"" : "")
                                     + ");\\n";
        std::string substitute = collection.empty() ? errorMsg : "";
        for (const auto& elem : collection) {
            substitute += std::string("\\t\\tprototype.add") + collectionType + "(\\\"" + elem.name + "\\\", OV_TypeId_"
                    + camelCase(elem.type) + (hasValue ? std::string(", \\\"") + elem.defaultValue + "\\\"" : "") + ");\\n";
        }
        return regexReplace(dst, pattern, substitute);
    };

    success &= insertData("Input", m_Inputs, false);
    success &= insertData("Output", m_Outputs, false);
    success &= insertData("Setting", m_Settings, true);

    //--------------------------------------------------------------------------------------
    //Codecs algorithms
    //--------------------------------------------------------------------------------------
    std::string sPattern    = "@@Algorithms@@";
    std::string sSubstitute = m_Inputs.empty() ? "\\/\\/ No Input decoder.\\n" : "\\/\\/ Input decoder:\\n";
    for (size_t i = 0; i < m_Inputs.size(); ++i) {
        std::string name(camelCase(m_Inputs[i].type));
        //The stream type is Stimulations but the decoder is tStimulationDecoder
        if (name == "Stimulations") { name = "Stimulation"; }
        sSubstitute += "\\tToolkit::T" + name + "Decoder<CBoxAlgorithm" + m_ClassName + "> m_input" + std::to_string(i) + "Decoder;\\n";
    }

    sSubstitute += m_Outputs.empty() ? "\\t\\/\\/ No Output decoder.\\n" : "\\t\\/\\/ Output decoder:\\n";
    for (size_t i = 0; i < m_Outputs.size(); ++i) {
        std::string name(camelCase(m_Outputs[i].type));
        //The stream type is Stimulations but the encoder is tStimulationEncoder
        if (name == "Stimulations") { name = "Stimulation"; }
        sSubstitute += "\\tToolkit::T" + name + "Encoder<CBoxAlgorithm" + m_ClassName + "> m_output" + std::to_string(i) + "Encoder;\\n";
    }
    success &= regexReplace(dst, sPattern, sSubstitute);

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // box.cpp
    dst = m_directory + "/CBoxAlgorithm" + m_ClassName + ".cpp";
    if (USE_CODEC_TOOLKIT) { tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/box.cpp-codec-toolkit-skeleton").toASCIIString(); }
    //else { tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/box.cpp-skeleton"); }
    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(log.c_str()), -1);
        success = false;
    }

    // Codec Algorithm stuff. too complicated for the simple SED primitives.
    sPattern    = "@@AlgorithmInitialisation@@";
    sSubstitute = "";
    //We initialize the codec algorithm by give them this, and the index of the input/output
    for (size_t i = 0; i < m_Inputs.size(); ++i) { sSubstitute += "\\tm_input" + std::to_string(i) + "Decoder.initialize(*this, " + std::to_string(i) + ");\n"; }
    for (size_t i = 0; i < m_Outputs.size(); ++i) { sSubstitute += "\\tm_output" + std::to_string(i) + "Encoder.initialize(*this, " + std::to_string(i) + ");\n"; }

    success &= regexReplace(dst, sPattern, sSubstitute);

    sPattern    = "@@AlgorithmUninitialisation@@";
    sSubstitute = "";
    //We initialize the codec algorithm by give them this, and the index of the input/output
    for (size_t i = 0; i < m_Inputs.size(); ++i) { sSubstitute += "\\tm_input" + std::to_string(i) + "Decoder.uninitialize();\n"; }
    for (size_t i = 0; i < m_Outputs.size(); ++i) { sSubstitute += "\\tm_output" + std::to_string(i) + "Encoder.uninitialize();\n"; }
    success &= regexReplace(dst, sPattern, sSubstitute);

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // readme-box.cpp
    dst     = m_directory + "/README.txt";
    tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/readme-box.txt-skeleton");

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(log.c_str()), -1);
        success = false;
    }

    //-------------------------------------------------------------------------------------------------------------------------------------------//

    if (success) {
        success &= cleanConfigurationFile(m_configFile);
        //re-load all entries, the internal variables may have been modified to be sed compliant.
        getCommonParameters();
        getCurrentParameters();
        //save the entries as the user typed them
        success &= saveCommonParameters(m_configFile);
        success &= save(m_configFile);
    }

    if (!success) {
        log += "Generation process did not completly succeed. Some files may have not been produced.\n";
        OV_WARNING_K("Generation process did not completly succeed. Some files may have not been produced.");
    }
    else {
        log += "Generation process successful. All entries saved in [" + m_configFile + "]\n";
        log += "Please read file [README.txt] !\n";
        getLogManager() << Kernel::LogLevel_Info << "Generation process successful. All entries saved in [" << m_configFile << "]\n";

        // opening browser to see the produced files
        std::string browser = m_kernelCtx.getConfigurationManager().expand("${Designer_WebBrowserCommand_${OperatingSystem}}").toASCIIString();
#ifdef TARGET_OS_Windows
        std::string browserCmd = browser + " file:///" + m_directory; //otherwise the browser does not find the directory (problem with / and \ char)
#else
        std::string browserCmd = browser + " \"" + m_directory + "\"";
#endif
        std::cout << browser;
        if (system(browserCmd.c_str())) { }
    }

    gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(log.c_str()), -1);
}

void CBoxAlgorithmSkeletonGenerator::ButtonTooltipCB(GtkButton* button)
{
    const std::string tooltip = m_tooltips[button];

    GtkWidget* tooltipTextview = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-tooltips-textview"));
    GtkTextBuffer* textBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tooltipTextview));
    gtk_text_buffer_set_text(textBuffer, TO_GTK_UTF8(tooltip.c_str()), -1);
}

void CBoxAlgorithmSkeletonGenerator::ButtonAddInputCB() const
{
    GtkWidget* dialog       = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-dialog"));
    GtkWidget* nameEntry    = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-name-entry"));
    GtkWidget* typeComboBox = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-type-combobox"));

    gtk_entry_set_text(GTK_ENTRY(nameEntry), "");

    const gint resp = gtk_dialog_run(GTK_DIALOG(dialog));

    if (resp == GTK_RESPONSE_APPLY) {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(nameEntry));
        //we get the two types (user/ov)
        GtkTreeIter itType;
        GtkTreeModel* treeModelType = gtk_combo_box_get_model(GTK_COMBO_BOX(typeComboBox));
        gtk_combo_box_get_active_iter(GTK_COMBO_BOX(typeComboBox), &itType);
        gchar *dataTypeUser, *dataTypeOv;
        gtk_tree_model_get(treeModelType, &itType, 0, &dataTypeUser, 1, &dataTypeOv, -1);
        //const gchar * type = gtk_combo_box_get_active_text(GTK_COMBO_BOX(typeComboBox));

        GtkWidget* treeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-inputs-treeview"));
        GtkTreeModel* listStore = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(listStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(listStore), &it, 0, name, 1, dataTypeUser, 2, dataTypeOv, -1);
        gtk_widget_hide(dialog);

        g_free(dataTypeUser);
        g_free(dataTypeOv);

        SetSensitivity("sg-box-ok-button", false);
    }
    else { gtk_widget_hide(dialog); }
}

void CBoxAlgorithmSkeletonGenerator::ButtonAddOutputCB() const
{
    GtkWidget* dialog       = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-dialog"));
    GtkWidget* nameEntry    = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-name-entry"));
    GtkWidget* typeComboBox = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-type-combobox"));

    gtk_entry_set_text(GTK_ENTRY(nameEntry), "");

    const gint resp = gtk_dialog_run(GTK_DIALOG(dialog));

    if (resp == GTK_RESPONSE_APPLY) {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(nameEntry));
        //we get the two types (user/ov)
        GtkTreeIter itType;
        GtkTreeModel* treeModelType = gtk_combo_box_get_model(GTK_COMBO_BOX(typeComboBox));
        gtk_combo_box_get_active_iter(GTK_COMBO_BOX(typeComboBox), &itType);
        gchar *dataTypeUser, *dataTypeOv;
        gtk_tree_model_get(treeModelType, &itType, 0, &dataTypeUser, 1, &dataTypeOv, -1);
        //const gchar * type = gtk_combo_box_get_active_text(GTK_COMBO_BOX(typeComboBox));

        GtkWidget* treeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-outputs-treeview"));
        GtkTreeModel* listStore = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter it;

        gtk_list_store_append(GTK_LIST_STORE(listStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(listStore), &it, 0, name, 1, dataTypeUser, 2, dataTypeOv, -1);
        gtk_widget_hide(dialog);

        g_free(dataTypeUser);
        g_free(dataTypeOv);

        SetSensitivity("sg-box-ok-button", false);
    }
    else { gtk_widget_hide(dialog); }
}

void CBoxAlgorithmSkeletonGenerator::ButtonAddSettingCB() const
{
    GtkWidget* dialog       = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-add-dialog"));
    GtkWidget* nameEntry    = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-add-name-entry"));
    GtkWidget* typeComboBox = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-add-type-combobox"));
    GtkWidget* valueEntry   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-add-default-value-entry"));

    gtk_entry_set_text(GTK_ENTRY(nameEntry), "");
    gtk_entry_set_text(GTK_ENTRY(valueEntry), "");

    const gint resp = gtk_dialog_run(GTK_DIALOG(dialog));

    if (resp == GTK_RESPONSE_APPLY) {
        const gchar* name  = gtk_entry_get_text(GTK_ENTRY(nameEntry));
        const gchar* value = gtk_entry_get_text(GTK_ENTRY(valueEntry));
        //we get the two types (user/ov)
        GtkTreeIter itType;
        GtkTreeModel* treeModelType = gtk_combo_box_get_model(GTK_COMBO_BOX(typeComboBox));
        gtk_combo_box_get_active_iter(GTK_COMBO_BOX(typeComboBox), &itType);
        gchar *dataTypeUser, *dataTypeOv;
        gtk_tree_model_get(treeModelType, &itType, 0, &dataTypeUser, 1, &dataTypeOv, -1);
        //const gchar * type = gtk_combo_box_get_active_text(GTK_COMBO_BOX(typeComboBox));

        GtkWidget* treeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-treeview"));
        GtkTreeModel* listStore = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
        GtkTreeIter it;

        gtk_list_store_append(GTK_LIST_STORE(listStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(listStore), &it, 0, name, 1, dataTypeUser, 2, value, 3, dataTypeOv, -1);
        gtk_widget_hide(dialog);

        g_free(dataTypeUser);
        g_free(dataTypeOv);

        SetSensitivity("sg-box-ok-button", false);
    }
    else { gtk_widget_hide(dialog); }
}

void CBoxAlgorithmSkeletonGenerator::ButtonAddAlgorithmCB() const
{
    GtkWidget* dialog   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-dialog"));
    GtkWidget* comboBox = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-combobox"));

    GtkWidget* inputsTreeView             = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-inputs-treeview"));
    GtkTreeModel* inputsListStore         = gtk_tree_view_get_model(GTK_TREE_VIEW(inputsTreeView));
    GtkWidget* outputsTreeView            = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-outputs-treeview"));
    GtkTreeModel* outputsListStore        = gtk_tree_view_get_model(GTK_TREE_VIEW(outputsTreeView));
    GtkWidget* inputTriggersTreeView      = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-input-triggers-treeview"));
    GtkTreeModel* inputTriggersListStore  = gtk_tree_view_get_model(GTK_TREE_VIEW(inputTriggersTreeView));
    GtkWidget* outputTriggersTreeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-output-triggers-treeview"));
    GtkTreeModel* outputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(outputTriggersTreeView));

    GtkWidget* categoryEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-category-entry"));
    gtk_entry_set_text(GTK_ENTRY(categoryEntry), "");
    GtkWidget* shortEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-short-description-entry"));
    gtk_entry_set_text(GTK_ENTRY(shortEntry), "");
    GtkWidget* detailedTextview = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-detailed-description-textview"));
    GtkTextBuffer* buffer       = gtk_text_view_get_buffer(GTK_TEXT_VIEW(detailedTextview));
    gtk_text_buffer_set_text(buffer, "", -1);


    gtk_list_store_clear(GTK_LIST_STORE(inputsListStore));
    gtk_list_store_clear(GTK_LIST_STORE(outputsListStore));
    gtk_list_store_clear(GTK_LIST_STORE(inputTriggersListStore));
    gtk_list_store_clear(GTK_LIST_STORE(outputTriggersListStore));
    gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), -1);

    const gint resp = gtk_dialog_run(GTK_DIALOG(dialog));

    if (resp == GTK_RESPONSE_APPLY) {
        const gchar* algo = gtk_combo_box_get_active_text(GTK_COMBO_BOX(comboBox));

        if (algo) {
            GtkWidget* treeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-treeview"));
            GtkTreeModel* listStore = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
            GtkTreeIter it;
            gtk_list_store_append(GTK_LIST_STORE(listStore), &it);
            gtk_list_store_set(GTK_LIST_STORE(listStore), &it, 0, algo, -1);
            SetSensitivity("sg-box-ok-button", false);
        }
        else { getLogManager() << Kernel::LogLevel_Error << "Please select an algorithm.\n"; }
        gtk_widget_hide(dialog);
    }
    else { gtk_widget_hide(dialog); }
}

void CBoxAlgorithmSkeletonGenerator::ButtonRemoveGeneric(const std::string& buttonName) const
{
    GtkWidget* treeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, buttonName.c_str()));
    GtkTreeModel* listStore = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
    GtkTreeIter it;
    GtkTreeSelection* select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
    if (gtk_tree_selection_get_selected(select, &listStore, &it)) {
        gtk_list_store_remove(GTK_LIST_STORE(listStore), &it);
        SetSensitivity("sg-box-ok-button", false);
    }
}

void CBoxAlgorithmSkeletonGenerator::ButtonAlgorithmSelectedCB(const int index) const
{
    clearStore("sg-box-algorithms-add-inputs-treeview");
    clearStore("sg-box-algorithms-add-outputs-treeview");
    clearStore("sg-box-algorithms-add-input-triggers-treeview");
    clearStore("sg-box-algorithms-add-output-triggers-treeview");

    GtkWidget* categoryEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-category-entry"));
    gtk_entry_set_text(GTK_ENTRY(categoryEntry), "");
    GtkWidget* shortEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-short-description-entry"));
    gtk_entry_set_text(GTK_ENTRY(shortEntry), "");
    GtkWidget* detailedTextview = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-detailed-description-textview"));
    GtkTextBuffer* buffer       = gtk_text_view_get_buffer(GTK_TEXT_VIEW(detailedTextview));
    gtk_text_buffer_set_text(buffer, "", -1);

    if (index != -1) {
        GtkWidget* comboBox         = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-combobox"));
        GtkTreeModel* algoListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(comboBox));
        GtkTreeIter iter;
        gchar* sID;
        gtk_tree_model_iter_nth_child(algoListStore, &iter, nullptr, index);
        gtk_tree_model_get(algoListStore, &iter, 1, &sID, -1);
        CIdentifier id;
        id.fromString(CString(sID));

        //we need to create a dummy instance of the algorithm proto to know its input/output/triggers
        const Plugins::IPluginObjectDesc* desc = m_kernelCtx.getPluginManager().getPluginObjectDesc(id);
        CDummyAlgoProto dummyProto;
        dynamic_cast<const Plugins::IAlgorithmDesc*>(desc)->getAlgorithmPrototype(dummyProto);

        addCollectionToTree("sg-box-algorithms-add-inputs-treeview", ExtractKeys(dummyProto.m_Inputs));
        addCollectionToTree("sg-box-algorithms-add-outputs-treeview", ExtractKeys(dummyProto.m_Outputs));
        addCollectionToTree("sg-box-algorithms-add-input-triggers-treeview", dummyProto.m_InputTriggers);
        addCollectionToTree("sg-box-algorithms-add-output-triggers-treeview", dummyProto.m_OutputTriggers);

        categoryEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-category-entry"));
        gtk_entry_set_text(GTK_ENTRY(categoryEntry), TO_GTK_UTF8(desc->getCategory().toASCIIString()));
        shortEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-short-description-entry"));
        gtk_entry_set_text(GTK_ENTRY(shortEntry), TO_GTK_UTF8(desc->getShortDescription().toASCIIString()));
        detailedTextview = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-detailed-description-textview"));
        buffer           = gtk_text_view_get_buffer(GTK_TEXT_VIEW(detailedTextview));
        gtk_text_buffer_set_text(buffer, TO_GTK_UTF8(desc->getDetailedDescription().toASCIIString()), -1);
    }
}

//--------------------------------------------------------------------------
bool CBoxAlgorithmSkeletonGenerator::initialize()
{
    GtkWidget* box = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-window"));

    // Main Buttons and signals
    GtkWidget* buttonCheck = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-check-button"));
    GtkWidget* buttonOk    = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-ok-button"));
    gtk_widget_set_sensitive(buttonOk, false);

    g_signal_connect(buttonCheck, "pressed", G_CALLBACK(CheckCB), this);
    g_signal_connect(buttonOk, "pressed", G_CALLBACK(OkCB), this);

    //connect all the signals in the .ui file (entry_modified_cb)
    gtk_builder_connect_signals(m_builder, this);

    // Tooltips buttons and signal
    GtkButton* buttonNameVersion       = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-name-version-tooltip-button"));
    GtkButton* buttonCategory          = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-category-tooltip-button"));
    GtkButton* buttonDesc              = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-description-tooltip-button"));
    GtkButton* buttonIcon              = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-icon-tooltip-button"));
    GtkButton* buttonInputs            = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-inputs-list-tooltip-button"));
    GtkButton* buttonInputsModify      = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-inputs-modify-tooltip-button"));
    GtkButton* buttonInputsAddRemove   = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-inputs-add-tooltip-button"));
    GtkButton* buttonOutputs           = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-outputs-list-tooltip-button"));
    GtkButton* buttonOutputsModify     = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-outputs-modify-tooltip-button"));
    GtkButton* buttonOutputsAddRemove  = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-outputs-add-tooltip-button"));
    GtkButton* buttonSettings          = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-settings-list-tooltip-button"));
    GtkButton* buttonSettingsModify    = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-settings-modify-tooltip-button"));
    GtkButton* buttonSettingsAddRemove = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-settings-add-tooltip-button"));
    GtkButton* buttonAlgorithms        = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-algorithms-tooltip-button"));
    GtkButton* buttonClassName         = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-class-name-tooltip-button"));
    GtkButton* buttonBoxListener       = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-listener-tooltip-button"));
    //::GtkButton* buttonUseCodecToolkit = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-algorithms-toolkit-tooltip-button"));

    m_tooltips[buttonNameVersion] =
            "Box Name: \nThis name will be the one displayed in the Designer.\nUsually, the box name reflects its main purpose.\nPlease also enter a version number for your box.\nAuthorized characters: letters (lower and upper case), numbers, special characters '()[]._-'\n------\nExample: Clock Stimulator (tic tac), version 1.2";
    m_tooltips[buttonCategory] =
            "Category: \nThe category decides where the box will be strored in designer's box panel.\nYou can refer to an existing category, already used in the designer, or choose a new one.\nIf you need to specifiy a subcategory, use the character '/'.\nAuthorized characters: letters (lower and upper case) and spaces.\n------\nExample: Samples/Skeleton Generator\n";
    m_tooltips[buttonDesc] =
            "Description: \nThe short description will be displayed next to the box in the designer box panel.\nThe detailed description is showed on the 'About Box...' panel.\nAll characters are authorized.\n------\nExample:\nShort Description : Periodic stimulation generator\nDetailed description : This box triggers stimulation at fixed frequency.";
    m_tooltips[buttonIcon] =
            "Box Icon: \nThe icon used in the designer box panel for this box.\nThis is an optional field.\n------\nExample: 'gtk-help' will be the corresponding gtk stock item (depending on the gtk theme used)\n\n\n";
    m_tooltips[buttonInputs] =
            "Inputs: \nUse the 'Add' and 'Remove' buttons to set all the inputs your box will have.\nWhen pressing 'Add' a dialog window will appear to know the name and type of the new input.\n------\nExample:\n'Incoming Signal' of type 'Signal'\n\n";
    m_tooltips[buttonInputsModify] =
            "Modify: \nCheck this option if the input(s) of your box can be modified (type and name) in the Designer by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanModifyInput'.\n\n\n\n\n";
    m_tooltips[buttonInputsAddRemove] =
            "Add/Remove: \nCheck this option if the user must be able to add (or remove) inputs, by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanAddInput'.\n\n\n\n";
    m_tooltips[buttonOutputs] =
            "Outputs: \nUse the 'Add' and 'Remove' buttons to set all the outputs your box will have.\nWhen pressing 'Add' a dialog window will appear to know the name and type of the new output.\n------\nExample:\n'Filtered Signal' of type 'Signal'\n\n";
    m_tooltips[buttonOutputsModify] =
            "Modify: \nCheck this option if the output(s) of your box can be modified (type and name) in the Designer by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanModifyOutput'.\n\n\n\n\n";
    m_tooltips[buttonOutputsAddRemove] =
            "Add/Remove: \nCheck this option if the user must be able to add (or remove) outputs, by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanAddOutput'.\n\n\n\n";
    m_tooltips[buttonSettings] =
            "Settings: \nUse the 'Add' and 'Remove' buttons to set all the settings your box will have.\nWhen pressing 'Add' a dialog window will appear to know the name,type and default value of the new output.\n------\nExample:\n'Filter order' of type 'int' with default value '4'\n\n";
    m_tooltips[buttonSettingsModify] =
            "Modify: \nCheck this option if the setting(s) of your box can be modified (type and name) in the Designer by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanModifySetting'.\n\n\n\n\n";
    m_tooltips[buttonSettingsAddRemove] =
            "Add/Remove: \nCheck this option if the user must be able to add (or remove) settings, by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanAddSetting'.\n\n\n\n";
    m_tooltips[buttonAlgorithms] =
            "Codec Algorithms: \nChoose the decoder(s) and encoder(s) used by the box. \nYou can choose between all the different stream codecs currently in OpenViBE.\nWhen choosing a codec, the dialog window will display the algorithm inputs and outputs that can be retrieve through getter methods. \n------\nExample: Signal Decoder, that outputs a Streamed Matrix and a Sampling Frequency value from a Memory Buffer.\n\n";
    m_tooltips[buttonClassName] =
            "Class Name: \nThis name will be used in the code to build the class name.\nUsually, the class name is close to the box name, just without any blank.\nAuthorized characters: letters (lower and upper case), numbers, NO special characters, NO blank.\n------\nExample: ClockStimulator\n";
    m_tooltips[buttonBoxListener] =
            "Box Listener: \nImplement or not a box listener class in the header.\nA box listener has various callbacks that you can overwrite, related to any modification of the box structure.\n------\nExample:\nThe Identity box uses a box listener with 2 callbacks: 'onInputAdded' and 'onOutputAdded'.\nWhenever an input (output) is added, the listener automatically add an output (input) of the same type.\n";
    //m_tooltips[buttonUseCodecToolkit] = "Codec Toolkit: \nTells the generator to use or not the Codec Toolkit in the box implementation. \nThe Codec Toolkit makes the decoding and encoding process much more simpler.\nCurrently the Skeleton Generator only creates templates using the codec toolkit.\n\n\n\n\n";

    g_signal_connect(buttonNameVersion, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonCategory, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonDesc, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonIcon, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonInputs, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonInputsModify, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonInputsAddRemove, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonOutputs, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonOutputsModify, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonOutputsAddRemove, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonSettings, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonSettingsModify, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonSettingsAddRemove, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonAlgorithms, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonClassName, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonBoxListener, "pressed", G_CALLBACK(TooltipCB), this);
    //g_signal_connect(buttonUseCodecToolkit, "pressed",G_CALLBACK(TooltipCB), this);

    //'Inputs' buttons
    GtkButton* inputsButtonAdd     = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-inputs-add-button"));
    GtkButton* inputsButtonRem     = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-inputs-remove-button"));
    GtkButton* outputsButtonAdd    = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-outputs-add-button"));
    GtkButton* outputsButtonRem    = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-outputs-remove-button"));
    GtkButton* settingsButtonAdd   = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-settings-add-button"));
    GtkButton* settingsButtonRem   = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-settings-remove-button"));
    GtkButton* algorithmsButtonAdd = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-button"));
    GtkButton* algorithmsButtonRem = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-box-algorithms-remove-button"));

    g_signal_connect(inputsButtonAdd, "pressed", G_CALLBACK(AddInputCB), this);
    g_signal_connect(inputsButtonRem, "pressed", G_CALLBACK(RemInputCB), this);
    g_signal_connect(outputsButtonAdd, "pressed", G_CALLBACK(AddOutputCB), this);
    g_signal_connect(outputsButtonRem, "pressed", G_CALLBACK(RemOutputCB), this);
    g_signal_connect(settingsButtonAdd, "pressed", G_CALLBACK(AddSettingCB), this);
    g_signal_connect(settingsButtonRem, "pressed", G_CALLBACK(RemSettingCB), this);
    g_signal_connect(algorithmsButtonAdd, "pressed", G_CALLBACK(AddAlgorithmCB), this);
    g_signal_connect(algorithmsButtonRem, "pressed", G_CALLBACK(RemAlgorithmCB), this);

    addDialogButttons("sg-box-IO-add-dialog");
    addDialogButttons("sg-box-settings-add-dialog");
    addDialogButttons("sg-box-algorithms-add-dialog");

    //initialize the icon combo box with gtk stock items
    GtkWidget* iconCombobox     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-icon-combobox"));
    GtkTreeModel* iconListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(iconCombobox));
    GSList* stockIdList         = gtk_stock_list_ids();
    while (stockIdList->next != nullptr) {
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(iconListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(iconListStore), &it, 0, static_cast<char*>(stockIdList->data), 1, static_cast<char*>(stockIdList->data), -1);
        stockIdList = g_slist_next(stockIdList);
    }
    g_slist_free(stockIdList);

    //types when adding IOS
    GtkWidget* typeCombobox            = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-IO-add-type-combobox"));
    GtkTreeModel* typeListStore        = gtk_combo_box_get_model(GTK_COMBO_BOX(typeCombobox));
    GtkWidget* settingTypeCombobox     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-add-type-combobox"));
    GtkTreeModel* settingTypeListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(settingTypeCombobox));
    //we iterate over all identifiers
    CIdentifier id = m_kernelCtx.getTypeManager().getNextTypeIdentifier(CIdentifier::undefined());
    while (id != CIdentifier::undefined()) {
        const gchar* type   = TO_GTK_UTF8(m_kernelCtx.getTypeManager().getTypeName(id).toASCIIString());
        const gchar* typeID = TO_GTK_UTF8(id.str().c_str());
        //Streams are possible inputs and outputs
        if (m_kernelCtx.getTypeManager().isStream(id)) {
            GtkTreeIter it;
            gtk_list_store_append(GTK_LIST_STORE(typeListStore), &it);
            gtk_list_store_set(GTK_LIST_STORE(typeListStore), &it, 0, type, 1, typeID, -1);
        }
        else // other types are possible settings
        {
            GtkTreeIter it;
            gtk_list_store_append(GTK_LIST_STORE(settingTypeListStore), &it);
            gtk_list_store_set(GTK_LIST_STORE(settingTypeListStore), &it, 0, type, 1, typeID, -1);
        }
        id = m_kernelCtx.getTypeManager().getNextTypeIdentifier(id);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(typeCombobox), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(settingTypeCombobox), 0);
    //types when adding Algorithms
    m_typeCorrespondances.resize(Kernel::ParameterType_Pointer + 1);
    m_typeCorrespondances[Kernel::ParameterType_None]           = "TYPE-NOT-AVAILABLE";
    m_typeCorrespondances[Kernel::ParameterType_Integer]        = "int64_t";
    m_typeCorrespondances[Kernel::ParameterType_UInteger]       = "uint64_t";
    m_typeCorrespondances[Kernel::ParameterType_Enumeration]    = "ENUMERATION-NOT-AVAILABLE";
    m_typeCorrespondances[Kernel::ParameterType_Boolean]        = "bool";
    m_typeCorrespondances[Kernel::ParameterType_Float]          = "double";
    m_typeCorrespondances[Kernel::ParameterType_String]         = "CString";
    m_typeCorrespondances[Kernel::ParameterType_Identifier]     = "CIdentifier";
    m_typeCorrespondances[Kernel::ParameterType_Matrix]         = "CMatrix *";
    m_typeCorrespondances[Kernel::ParameterType_StimulationSet] = "CStimulationSet *";
    m_typeCorrespondances[Kernel::ParameterType_MemoryBuffer]   = "CMemoryBuffer *";
    m_typeCorrespondances[Kernel::ParameterType_Object]         = "IObject *";
    m_typeCorrespondances[Kernel::ParameterType_Pointer]        = "uint8_t*";


    GtkWidget* algoCombobox     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-add-combobox"));
    GtkTreeModel* algoListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(algoCombobox));
    //we iterate over all plugin descriptor identifiers
    id = m_kernelCtx.getPluginManager().getNextPluginObjectDescIdentifier(CIdentifier::undefined());
    while (id != CIdentifier::undefined()) {
        const Plugins::IPluginObjectDesc* desc = m_kernelCtx.getPluginManager().getPluginObjectDesc(id);
        if (desc != nullptr && desc->isDerivedFromClass(OV_ClassId_Plugins_AlgorithmDesc)) // we select only algorithm descriptors
        {
            std::string algo = desc->getName().toASCIIString();

            // we only keep decoders and encoders
            // and reject the master acquisition stream
            // and reject acquisition stream encoder as toolkit doesn't support it
            if ((algo.find("encoder") != std::string::npos || algo.find("decoder") != std::string::npos)
                && algo.find("Master") == std::string::npos && algo.find("Acquisition stream encoder") == std::string::npos) {
                std::string algoID = id.str();
                GtkTreeIter it;
                gtk_list_store_append(GTK_LIST_STORE(algoListStore), &it);
                gtk_list_store_set(GTK_LIST_STORE(algoListStore), &it, 0, TO_GTK_UTF8(algo.c_str()), 1, TO_GTK_UTF8(algoID.c_str()), -1);

                // now we map every decoder/encoder to its string description that will be added in the skeleton (algorithmProxy + parameter handlers for I/O)
                std::string headerDeclaration = "\\t\\/\\/ " + algo + "\\n";
                std::string initialisation    = "\\t\\/\\/ " + algo + "\\n";
                std::string initialisationReferenceTargets;
                std::string uninitialisation;


                //we need to create a dummy instance of the algorithm proto to know its input/output/triggers
                const Plugins::IPluginObjectDesc* dummyDesc = m_kernelCtx.getPluginManager().getPluginObjectDesc(id);
                CDummyAlgoProto dummyProto;
                dynamic_cast<const Plugins::IAlgorithmDesc*>(dummyDesc)->getAlgorithmPrototype(dummyProto);
                //algorithm proxy
                std::string algoNameStdSTr(camelCase(algo));
                std::string codecTypeStdStr = algoNameStdSTr;
                std::string stream("Stream");
                codecTypeStdStr.erase(codecTypeStdStr.rfind(stream), 6);

                std::string codec     = "m_o@" + codecTypeStdStr;
                std::string codecType = codecTypeStdStr;

                // use the Codec Toolkit
                headerDeclaration              = headerDeclaration + "\\tToolkit::T" + codecType + " < @@ClassName@@ > " + codec + ";\\n";
                initialisation                 = initialisation + "\\t" + codec + ".initialize(*this);\\n";
                uninitialisation               = "\\t" + codec + ".uninitialize();\\n" + uninitialisation;
                initialisationReferenceTargets = "";

                m_AlgoDeclaration[algo]                    = headerDeclaration;
                m_AlgoInitialisations[algo]                = initialisation;
                m_AlgoUninitialisations[algo]              = uninitialisation;
                m_AlgoInitialisationReferenceTargets[algo] = initialisationReferenceTargets;
                m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "The algorithm [" << algo << "] has description [" << headerDeclaration << "\n";
            }
        }
        id = m_kernelCtx.getPluginManager().getNextPluginObjectDescIdentifier(id);
    }


    gtk_combo_box_set_active(GTK_COMBO_BOX(algoCombobox), -1);
    //callback to update algo description
    g_signal_connect(G_OBJECT(algoCombobox), "changed", G_CALLBACK(AlgorithmSelectedCB), this);

    //Close with X and "cancel" button
    g_signal_connect(G_OBJECT(box), "delete_event", G_CALLBACK(gtk_exit), nullptr);
    GtkWidget* buttonCancel = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-exit-button"));
    g_signal_connect(buttonCancel, "pressed", G_CALLBACK(ExitCB), this);

    //load everything from config file
    load(m_configFile);
    GtkWidget* listenerWidget = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-listener-checkbutton"));
    ToggleListenerCheckbuttonsStateCB((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(listenerWidget)) > 0));

    gtk_widget_show_all(box);

    return true;
}

bool CBoxAlgorithmSkeletonGenerator::save(const std::string& filename)
{
    std::ofstream file;
    file.open(filename, std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        OV_WARNING_K("Saving the box entries in [" << filename << "] failed !");
        return false;
    }
    file << "# ----------------------BOX GENERATOR-------------------------" << std::endl;
    std::string tmp(m_directory);
    for (auto it = tmp.begin(); it < tmp.end(); ++it) { if ((*it) == '\\') { tmp.replace(it, it + 1, 1, '/'); } }

    file << "SkeletonGenerator_Box_TargetDirectory = " << tmp << std::endl;
    file << "SkeletonGenerator_Box_Name = " << m_Name << std::endl;
    file << "SkeletonGenerator_Box_Version = " << m_Version << std::endl;
    file << "SkeletonGenerator_Box_Category = " << m_Category << std::endl;
    file << "SkeletonGenerator_Box_ClassName = " << m_ClassName << std::endl;

    //we need to escape with '\' the special characters of the configuration manager files
    tmp = m_ShortDesc;
    tmp.reserve(1000); // if we need to insert characters
    auto it = tmp.begin();
    while (it < tmp.end()) {
        //characters to escape
        if ((*it) == '\\' || (*it) == '=' || (*it) == '$' || (*it) == '\t') {
            tmp.insert(it, '\\');
            ++it;
        }
        ++it;
    }
    file << "SkeletonGenerator_Box_ShortDescription = " << tmp << std::endl;

    //we need to escape with '\' the special characters of the configuration manager files
    tmp = m_DetailedDesc;
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "SAVE > DESCRIPTION FROM WIDGET: " << tmp << "\n";
    tmp.reserve(1000); // if we need to insert characters
    it = tmp.begin();
    while (it < tmp.end()) {
        //characters to escape
        if ((*it) == '\\' || (*it) == '=' || (*it) == '$' || (*it) == '\t') {
            tmp.insert(it, '\\');
            ++it;
        }
        //the special character we use for \n must also be escaped when used in the text
        else if ((*it) == '@') {
            tmp.insert(it, '\\');
            tmp.insert(it, '\\');
            it += 2;
        }
        //we add a special character @ representing a \n for further loading. the \ ensure that the config manager will read the token past the \n
        else if ((*it) == '\n') {
            tmp.insert(it, '\\');
            tmp.insert(it, '@');
            it += 2;
        }
        ++it;
    }
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "SAVE > DESCR MODIFIED: " << tmp << "\n";

    file << "SkeletonGenerator_Box_DetailedDescription = " << tmp << std::endl;

    file << "SkeletonGenerator_Box_IconIndex = " << m_GtkStockItemIdx << std::endl;
    file << "SkeletonGenerator_Box_IconName = " << m_GtkStockItemName << std::endl;

    // ADD/MODIFY FLAGS
    file << "SkeletonGenerator_Box_CanModifyInputs = " << (m_CanModifyInputs ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_CanAddInputs = " << (m_CanAddInputs ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_CanModifyOutputs = " << (m_CanModifyOutputs ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_CanAddOutputs = " << (m_CanAddOutputs ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_CanModifySettings = " << (m_CanModifySettings ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_CanAddSettings = " << (m_CanAddSettings ? "TRUE" : "FALSE") << std::endl;

    // Inputs
    file << "SkeletonGenerator_Box_InputCount = " << m_Inputs.size() << std::endl;
    for (size_t i = 0; i < m_Inputs.size(); ++i) {
        file << "SkeletonGenerator_Box_Input" << i << "_Name = " << m_Inputs[i].name << std::endl;
        file << "SkeletonGenerator_Box_Input" << i << "_Type = " << m_Inputs[i].type << std::endl;
        file << "SkeletonGenerator_Box_Input" << i << "_TypeId = " << m_Inputs[i].typeID << std::endl;
    }
    // Outputs
    file << "SkeletonGenerator_Box_OutputCount = " << m_Outputs.size() << std::endl;
    for (size_t i = 0; i < m_Outputs.size(); ++i) {
        file << "SkeletonGenerator_Box_Output" << i << "_Name = " << m_Outputs[i].name << std::endl;
        file << "SkeletonGenerator_Box_Output" << i << "_Type = " << m_Outputs[i].type << std::endl;
        file << "SkeletonGenerator_Box_Output" << i << "_TypeId = " << m_Outputs[i].typeID << std::endl;
    }
    // Settings
    file << "SkeletonGenerator_Box_SettingCount = " << m_Settings.size() << std::endl;
    for (size_t i = 0; i < m_Settings.size(); ++i) {
        file << "SkeletonGenerator_Box_Setting" << i << "_Name = " << m_Settings[i].name << std::endl;
        file << "SkeletonGenerator_Box_Setting" << i << "_Type = " << m_Settings[i].type << std::endl;
        file << "SkeletonGenerator_Box_Setting" << i << "_TypeId = " << m_Settings[i].typeID << std::endl;
        file << "SkeletonGenerator_Box_Setting" << i << "_DefaultValue = " << m_Settings[i].defaultValue << std::endl;
    }
    // Algorithms
    file << "SkeletonGenerator_Box_AlgorithmCount = " << m_Algorithms.size() << std::endl;
    for (size_t i = 0; i < m_Algorithms.size(); ++i) { file << "SkeletonGenerator_Box_Algorithm" << i << "_Name = " << m_Algorithms[i] << std::endl; }

    // Listener
    file << "SkeletonGenerator_Box_UseListener = " << (m_UseBoxListener ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnInputAdded = " << (m_HasOnInputAdded ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnInputRemoved = " << (m_HasOnInputRemoved ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnInputTypeChanged = " << (m_HasOnInputTypeChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnInputNameChanged = " << (m_HasOnInputNameChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnInputConnected = " << (m_HasOnInputConnected ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnInputDisconnected = " << (m_HasOnInputDisconnected ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnOutputAdded = " << (m_HasOnOutputAdded ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnOutputRemoved = " << (m_HasOnOutputRemoved ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnOutputTypeChanged = " << (m_HasOnOutputTypeChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnOutputNameChanged = " << (m_HasOnOutputNameChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnOutputConnected = " << (m_HasOnOutputConnected ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnOutputDisconnected = " << (m_HasOnOutputDisconnected ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnSettingAdded = " << (m_HasOnSettingAdded ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnSettingRemoved = " << (m_HasOnSettingRemoved ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnSettingTypeChanged = " << (m_HasOnSettingTypeChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnSettingNameChanged = " << (m_HasOnSettingNameChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnSettingDefaultValueChanged = " << (m_HasOnSettingDefaultValueChanged ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ListenerOnSettingValueChanged = " << (m_HasOnSettingValueChanged ? "TRUE" : "FALSE") << std::endl;

    file << "SkeletonGenerator_Box_ProcessInput = " << (m_HasProcessInput ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ProcessClock = " << (m_HasProcessClock ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ProcessMessage = " << (m_HasProcessMessage ? "TRUE" : "FALSE") << std::endl;
    file << "SkeletonGenerator_Box_ClockFrequency = " << m_ClockFrequency << std::endl;


    file << "# --------------------------------------------------" << std::endl;
    file.close();
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "box entries saved in [" << filename << "]\n";

    m_configFileLoaded = false;

    return true;
}

bool CBoxAlgorithmSkeletonGenerator::load(const std::string& filename)
{
    if (!m_configFileLoaded && !m_kernelCtx.getConfigurationManager().addConfigurationFromFile(filename.c_str())) {
        OV_WARNING_K("box: Configuration file [" << filename << "] could not be loaded.");
        return false;
    }


    setEntryToConfigValue("sg-box-class-name-entry", "${SkeletonGenerator_Box_ClassName}");
    setEntryToConfigValue("sg-box-category-entry", "${SkeletonGenerator_Box_Category}");
    setEntryToConfigValue("sg-box-box-name-entry", "${SkeletonGenerator_Box_Name}");
    setEntryToConfigValue("sg-box-version-entry", "${SkeletonGenerator_Box_Version}");


    //we need to UNescape the special characters of the configuration manager files
    std::string::iterator itShort;
    std::string shortDescr = m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Box_ShortDescription}").toASCIIString();
    for (itShort = shortDescr.begin(); itShort < shortDescr.end(); ++itShort) {
        // juste erase the escape character
        if ((*itShort) == '\\' && (itShort + 1) != shortDescr.end()) { if ((*(itShort + 1)) == '\\' || (*(itShort + 1)) == '=' || (*(itShort + 1)) == '$' || (*(itShort + 1)) == '\t' || (*(itShort + 1)) == '@') { shortDescr.erase(itShort); } }
        // replace the special character @ by \n in the textview
        else if ((*itShort) == '@') {
            shortDescr.erase(itShort);
            shortDescr.insert(itShort, '\n');
        }
    }
    GtkWidget* sdEntry = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-short-description-entry"));
    gtk_entry_set_text(GTK_ENTRY(sdEntry), TO_GTK_UTF8(shortDescr.c_str()));

    GtkWidget* detailedDescrTextView   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-detailed-description-textview"));
    GtkTextBuffer* detailedDescrBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(detailedDescrTextView));
    std::string detailedDescr          = m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Box_DetailedDescription}").toASCIIString();
    getLogManager() << Kernel::LogLevel_Debug << "LOAD > DESCR LOADED: " << detailedDescr << "\n";
    for (auto itDetail = detailedDescr.begin(); itDetail < detailedDescr.end(); ++itDetail) {
        // juste erase the escape character
        if ((*itDetail) == '\\' && (itDetail + 1) != detailedDescr.end()) { if ((*(itShort + 1)) == '\\' || (*(itShort + 1)) == '=' || (*(itShort + 1)) == '$' || (*(itShort + 1)) == '\t' || (*(itShort + 1)) == '@') { detailedDescr.erase(itDetail); } }
        // replace the special character @ by \n in the textview
        else if ((*itDetail) == '@') {
            detailedDescr.erase(itDetail);
            detailedDescr.insert(itDetail, '\n');
        }
    }
    getLogManager() << Kernel::LogLevel_Debug << "LOAD > DESCR MODIFIED: " << detailedDescr << "\n";
    gtk_text_buffer_set_text(detailedDescrBuffer, TO_GTK_UTF8(detailedDescr.c_str()), gint(detailedDescr.length()));

    GtkWidget* iconComboBox = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-icon-combobox"));
    int64_t iconSelected    = m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_IconIndex}");
    gtk_combo_box_set_active(GTK_COMBO_BOX(iconComboBox), gint(iconSelected));

    setActiveFromConf("sg-box-inputs-modify-checkbutton", "${SkeletonGenerator_Box_CanModifyInputs}");
    setActiveFromConf("sg-box-inputs-add-checkbutton", "${SkeletonGenerator_Box_CanAddInputs}");
    setActiveFromConf("sg-box-outputs-modify-checkbutton", "${SkeletonGenerator_Box_CanModifyOutputs}");
    setActiveFromConf("sg-box-outputs-add-checkbutton", "${SkeletonGenerator_Box_CanAddOutputs}");
    setActiveFromConf("sg-box-settings-modify-checkbutton", "${SkeletonGenerator_Box_CanModifySettings}");
    setActiveFromConf("sg-box-settings-add-checkbutton", "${SkeletonGenerator_Box_CanAddSettings}");


    GtkWidget* inputsTreeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-inputs-treeview"));
    GtkTreeModel* inputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(inputsTreeView));
    const size_t nInput           = size_t(m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_InputCount}", 0));
    for (size_t i = 0; i < nInput; ++i) {
        const std::string base = "${SkeletonGenerator_Box_Input" + std::to_string(i);
        const gchar* name      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Name}").c_str())).toASCIIString());
        const gchar* type      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Type}").c_str())).toASCIIString());
        const gchar* typeID    = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_TypeId}").c_str())).toASCIIString());
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(inputsListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(inputsListStore), &it, 0, name, 1, type, 2, typeID, -1);
    }

    GtkWidget* outputsTreeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-outputs-treeview"));
    GtkTreeModel* outputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(outputsTreeView));
    const size_t nOutput           = size_t(m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_OutputCount}", 0));
    for (size_t i = 0; i < nOutput; ++i) {
        const std::string base = "${SkeletonGenerator_Box_Output" + std::to_string(i);
        const gchar* name      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Name}").c_str())).toASCIIString());
        const gchar* type      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Type}").c_str())).toASCIIString());
        const gchar* typeID    = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_TypeId}").c_str())).toASCIIString());
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(outputsListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(outputsListStore), &it, 0, name, 1, type, 2, typeID, -1);
    }

    GtkWidget* settingsTreeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-settings-treeview"));
    GtkTreeModel* settingsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(settingsTreeView));
    const size_t nSetting           = size_t(m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_SettingCount}", 0));
    for (size_t i = 0; i < nSetting; ++i) {
        const std::string base = "${SkeletonGenerator_Box_Setting" + std::to_string(i);
        const gchar* name      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Name}").c_str())).toASCIIString());
        const gchar* type      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Type}").c_str())).toASCIIString());
        const gchar* typeID    = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_TypeId}").c_str())).toASCIIString());
        const gchar* value     = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_DefaultValue}").c_str())).toASCIIString());
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(settingsListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(settingsListStore), &it, 0, name, 1, type, 2, value, 3, typeID, -1);
    }

    GtkWidget* algoTreeView     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-treeview"));
    GtkTreeModel* algoListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(algoTreeView));
    const size_t nAlgo          = size_t(m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_AlgorithmCount}", 0));
    for (size_t i = 0; i < nAlgo; ++i) {
        const std::string base = "${SkeletonGenerator_Box_Algorithm" + std::to_string(i);
        const gchar* name      = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(CString((base + "_Name}").c_str())).toASCIIString());
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(algoListStore), &it);
        gtk_list_store_set(GTK_LIST_STORE(algoListStore), &it, 0, name, -1);
    }

    GtkWidget* listenerWidget = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-listener-checkbutton"));
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(listenerWidget), m_kernelCtx.getConfigurationManager().expandAsBoolean("${SkeletonGenerator_Box_UseListener}", false));

    setActiveFromConfTog("sg-box-listener-input-added-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputAdded}");
    setActiveFromConfTog("sg-box-listener-input-removed-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputRemoved}");
    setActiveFromConfTog("sg-box-listener-input-type-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputTypeChanged}");
    setActiveFromConfTog("sg-box-listener-input-name-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputNameChanged}");
    setActiveFromConfTog("sg-box-listener-input-connected-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputConnected}");
    setActiveFromConfTog("sg-box-listener-input-disconnected-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputDisconnected}");
    setActiveFromConfTog("sg-box-listener-output-added-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputAdded}");
    setActiveFromConfTog("sg-box-listener-output-removed-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputRemoved}");
    setActiveFromConfTog("sg-box-listener-output-type-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputTypeChanged}");
    setActiveFromConfTog("sg-box-listener-output-name-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputNameChanged}");
    setActiveFromConfTog("sg-box-listener-output-connected-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputConnected}");
    setActiveFromConfTog("sg-box-listener-output-disconnected-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputDisconnected}");

    setActiveFromConfTog("sg-box-listener-setting-added-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingAdded}");
    setActiveFromConfTog("sg-box-listener-setting-removed-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingRemoved}");
    setActiveFromConfTog("sg-box-listener-setting-type-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingTypeChanged}");
    setActiveFromConfTog("sg-box-listener-setting-name-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingNameChanged}");
    setActiveFromConfTog("sg-box-listener-setting-default-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingDefaultValueChanged}");
    setActiveFromConfTog("sg-box-listener-setting-value-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingValueChanged}");

    setActiveFromConfTog("sg-box-process-input-checkbutton", "${SkeletonGenerator_Box_ProcessInput}");
    setActiveFromConfTog("sg-box-process-clock-checkbutton", "${SkeletonGenerator_Box_ProcessClock}");
    GtkWidget* processingMethod = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-process-frequency-spinbutton"));
    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(processingMethod), double(m_kernelCtx.getConfigurationManager().expandAsUInteger("${SkeletonGenerator_Box_ProcessClock}", 1)));
    setActiveFromConfTog("sg-box-process-message-checkbutton", "${SkeletonGenerator_Box_ProcessMessage}");

    getLogManager() << Kernel::LogLevel_Info << "box entries from [" << m_configFile << "] loaded.\n";

    return true;
}

void CBoxAlgorithmSkeletonGenerator::getCurrentParameters()
{
    m_Name      = getText("sg-box-box-name-entry");
    m_ClassName = getText("sg-box-class-name-entry");
    m_Category  = getText("sg-box-category-entry");
    m_Version   = getText("sg-box-version-entry");
    m_ShortDesc = getText("sg-box-short-description-entry");

    GtkWidget* entryDetailedDescr      = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-detailed-description-textview"));
    GtkTextBuffer* bufferDetailedDescr = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entryDetailedDescr));
    GtkTextIter itStart, itEnd;
    gtk_text_buffer_get_start_iter(bufferDetailedDescr, &itStart);
    gtk_text_buffer_get_end_iter(bufferDetailedDescr, &itEnd);
    m_DetailedDesc = gtk_text_buffer_get_text(bufferDetailedDescr, &itStart, &itEnd, false);

    GtkWidget* iconCombobox     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-icon-combobox"));
    GtkTreeModel* iconListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(iconCombobox));
    GtkTreeIter itIcon;
    m_GtkStockItemIdx = gtk_combo_box_get_active(GTK_COMBO_BOX(iconCombobox)); // can be -1 if nothing selected
    if (m_GtkStockItemIdx != -1) {
        gtk_tree_model_iter_nth_child(iconListStore, &itIcon, nullptr, m_GtkStockItemIdx);
        gchar* data;
        gtk_tree_model_get(iconListStore, &itIcon, 0, &data, -1);
        m_GtkStockItemName = data;
    }
    else { m_GtkStockItemName = ""; }

    m_CanModifyInputs   = isActive("sg-box-inputs-modify-checkbutton");
    m_CanAddInputs      = isActive("sg-box-inputs-add-checkbutton");
    m_CanModifyOutputs  = isActive("sg-box-outputs-modify-checkbutton");
    m_CanAddOutputs     = isActive("sg-box-outputs-add-checkbutton");
    m_CanModifySettings = isActive("sg-box-settings-modify-checkbutton");
    m_CanAddSettings    = isActive("sg-box-settings-add-checkbutton");

    m_UseBoxListener         = isActive("sg-box-listener-checkbutton");
    m_HasOnInputAdded        = isActive("sg-box-listener-input-added-checkbutton");
    m_HasOnInputRemoved      = isActive("sg-box-listener-input-removed-checkbutton");
    m_HasOnInputTypeChanged  = isActive("sg-box-listener-input-type-checkbutton");
    m_HasOnInputNameChanged  = isActive("sg-box-listener-input-name-checkbutton");
    m_HasOnInputConnected    = isActive("sg-box-listener-input-connected-checkbutton");
    m_HasOnInputDisconnected = isActive("sg-box-listener-input-disconnected-checkbutton");

    m_HasOnOutputAdded        = isActive("sg-box-listener-output-added-checkbutton");
    m_HasOnOutputRemoved      = isActive("sg-box-listener-output-removed-checkbutton");
    m_HasOnOutputTypeChanged  = isActive("sg-box-listener-output-type-checkbutton");
    m_HasOnOutputNameChanged  = isActive("sg-box-listener-output-name-checkbutton");
    m_HasOnOutputConnected    = isActive("sg-box-listener-output-connected-checkbutton");
    m_HasOnOutputDisconnected = isActive("sg-box-listener-output-disconnected-checkbutton");

    m_HasOnSettingAdded               = isActive("sg-box-listener-setting-added-checkbutton");
    m_HasOnSettingRemoved             = isActive("sg-box-listener-setting-removed-checkbutton");
    m_HasOnSettingTypeChanged         = isActive("sg-box-listener-setting-type-checkbutton");
    m_HasOnSettingNameChanged         = isActive("sg-box-listener-setting-name-checkbutton");
    m_HasOnSettingDefaultValueChanged = isActive("sg-box-listener-setting-default-checkbutton");
    m_HasOnSettingValueChanged        = isActive("sg-box-listener-setting-value-checkbutton");

    m_HasProcessInput        = isActive("sg-box-process-input-checkbutton");
    m_HasProcessClock        = isActive("sg-box-process-clock-checkbutton");
    GtkWidget* processMethod = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-process-frequency-spinbutton"));
    m_ClockFrequency         = uint32_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(processMethod)));
    m_HasProcessMessage      = isActive("sg-box-process-message-checkbutton");

    //USE_CODEC_TOOLKIT = isActive("sg-box-algorithms-toolkit-checkbutton");

    fillCollection("sg-box-inputs-treeview", m_Inputs);
    fillCollection("sg-box-outputs-treeview", m_Outputs);
    fillCollection("sg-box-settings-treeview", m_Settings);

    GtkWidget* algosTreeview     = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-algorithms-treeview"));
    GtkTreeModel* algosListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(algosTreeview));
    GtkTreeIter itAlgo;
    bool valid = gtk_tree_model_get_iter_first(algosListStore, &itAlgo) ? true : false;
    m_Algorithms.clear();
    while (valid) {
        /* Walk through the list, reading each row */
        gchar* name;
        gtk_tree_model_get(algosListStore, &itAlgo, 0, &name, -1);

        m_Algorithms.push_back(name);

        g_free(name);
        valid = (gtk_tree_model_iter_next(algosListStore, &itAlgo) ? true : false);
    }
}

void CBoxAlgorithmSkeletonGenerator::SetSensitivity(const std::string& widgetName, const bool isActive) const
{
    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, widgetName.c_str()));
    if (widget != nullptr) { gtk_widget_set_sensitive(widget, isActive); }
}

void CBoxAlgorithmSkeletonGenerator::clearStore(const std::string& name) const
{
    GtkWidget* view     = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    GtkTreeModel* store = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
    gtk_list_store_clear(GTK_LIST_STORE(store));
}

void CBoxAlgorithmSkeletonGenerator::addDialogButttons(const std::string& name) const
{
    GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
}

void CBoxAlgorithmSkeletonGenerator::setEntryToConfigValue(const std::string& name, const std::string& token) const
{
    GtkWidget* widget  = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    const gchar* value = TO_GTK_UTF8(m_kernelCtx.getConfigurationManager().expand(token.c_str()).toASCIIString());
    gtk_entry_set_text(GTK_ENTRY(widget), TO_GTK_UTF8(value));
}

void CBoxAlgorithmSkeletonGenerator::setActiveFromConf(const std::string& name, const std::string& token) const
{
    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    const bool value  = m_kernelCtx.getConfigurationManager().expandAsBoolean(token.c_str(), false);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
}

void CBoxAlgorithmSkeletonGenerator::setActiveFromConfTog(const std::string& name, const std::string& token) const
{
    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    const bool value  = m_kernelCtx.getConfigurationManager().expandAsBoolean(token.c_str(), false);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
}

const gchar* CBoxAlgorithmSkeletonGenerator::getText(const std::string& name) const
{
    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    return gtk_entry_get_text(GTK_ENTRY(widget));
}

void CBoxAlgorithmSkeletonGenerator::addCollectionToTree(const std::string& treeName, const std::vector<std::string>& collection) const
{
    GtkWidget* view     = GTK_WIDGET(gtk_builder_get_object(m_builder, treeName.c_str()));
    GtkTreeModel* store = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
    for (const auto& elem : collection) {
        GtkTreeIter it;
        gtk_list_store_append(GTK_LIST_STORE(store), &it);
        gtk_list_store_set(GTK_LIST_STORE(store), &it, 0, TO_GTK_UTF8(elem.c_str()), -1);
    }
}

void CBoxAlgorithmSkeletonGenerator::fillCollection(const std::string& treeName, std::vector<SIOS>& collection) const
{
    GtkWidget* widget   = GTK_WIDGET(gtk_builder_get_object(m_builder, treeName.c_str()));
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    GtkTreeIter it;
    bool valid = gtk_tree_model_get_iter_first(model, &it) ? true : false;
    collection.clear();
    while (valid) {
        /* Walk through the list, reading each row */
        gchar *name, *type, *typeOv, *value = nullptr;
        if (gtk_tree_model_get_n_columns(model) == 3) { gtk_tree_model_get(model, &it, 0, &name, 1, &type, 2, &typeOv, -1); }
        else { gtk_tree_model_get(model, &it, 0, &name, 1, &type, 2, &value, 3, &typeOv, -1); }

        SIOS ios;
        ios.name         = name;
        ios.type         = type;
        ios.typeID       = typeOv;
        ios.defaultValue = value ? value : "";
        collection.push_back(ios);

        g_free(name);
        g_free(type);
        g_free(typeOv);
        g_free(value);
        valid = gtk_tree_model_iter_next(model, &it) ? true : false;
    }
}

bool CBoxAlgorithmSkeletonGenerator::isActive(const std::string& name) const
{
    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, name.c_str()));
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? true : false;
}

}  // namespace SkeletonGenerator
}  // namespace OpenViBE
