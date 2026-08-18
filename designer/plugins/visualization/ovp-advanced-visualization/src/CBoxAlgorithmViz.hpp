///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmViz.hpp
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

#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <visualization-toolkit/ovviz_all.h>

#include <mensia/advanced-visualization.hpp>

#include <system/ovCTime.h>

#include "TGtkGLWidget.hpp"
#include "CMouse.hpp"

#include "IRuler.hpp"

#include <vector>
#include <string>
#include <cstdarg>
#include <algorithm>
#include <iostream>

namespace OpenViBE {
namespace AdvancedVisualization {
enum EParameter
{
    // Parameter
    P_None,

    // Input
    I_Matrix, I_Signal, I_Spectrum, I_TimeFrequency, I_Covariance, I_Stimulations,

    // Setting
    S_ChannelLocalisation, S_TemporalCoherence, S_TimeScale, S_ElementCount, S_DataScale, S_Caption, S_DataPositive,
    S_Translucency, S_FlowerRingCount, S_Color, S_ColorGradient, S_ShowAxis, S_XYZPlotHasDepth,

    // Flag
    F_CanAddInput, F_FixedChannelOrder, F_FixedChannelSelection, F_Unstable
};

class CParameterSet
{
public:
    explicit CParameterSet(int p, ...)
    {
        va_list args;
        va_start(args, p);
        while (p != P_None) {
            m_parameters.push_back(p);
            p = va_arg(args, int);
        }
        va_end(args);
    }

    explicit operator const std::vector<int>&() const { return m_parameters; }

protected:
    std::vector<int> m_parameters;
};

class CBoxAlgorithmViz : public Toolkit::TBoxAlgorithm<Plugins::IBoxAlgorithm>
{
public:
    typedef struct
    {
        float r, g, b;
    } color_t;

    CBoxAlgorithmViz(const CIdentifier& classID, const std::vector<int>& parameters)
        : m_ClassID(classID), m_Parameters(parameters), m_MouseHandler(*this)
    {
        m_Color.r = 1;
        m_Color.g = 1;
        m_Color.b = 1;
    }

    void release() override { delete this; }

    uint64_t getClockFrequency() override { return (32LL << 32); }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t /*index*/) override { return true; }
    bool processClock(Kernel::CMessageClock& msg) override;

    virtual void RedrawTopLevelWindow(const bool immediate = false) { m_GtkGlWidget.RedrawTopLevelWindow(immediate); }

    virtual void Redraw(const bool immediate = false)
    {
        const uint64_t currentTime = System::Time::zgetTime();
        if (m_RedrawNeeded || currentTime - m_LastRenderTime > ((1LL << 32) / 16)) {
            // immediate |= (currentTime - m_lastRenderTime > ((1LL<<32)/4));
            m_GtkGlWidget.Redraw(immediate);
            m_GtkGlWidget.RedrawLeft(immediate);
            m_GtkGlWidget.RedrawRight(immediate);
            m_GtkGlWidget.RedrawBottom(immediate);
            m_LastRenderTime = currentTime;
            m_RedrawNeeded   = false;
        }
    }

    virtual void UpdateRulerVisibility();
    virtual void Reshape(int width, int height);
    virtual void PreDraw();
    virtual void PostDraw();
    virtual void Draw() { }
    virtual void DrawLeft() { if (m_Ruler != nullptr) { m_Ruler->DoRenderLeft(m_Left); } }
    virtual void DrawRight() { if (m_Ruler != nullptr) { m_Ruler->DoRenderRight(m_Right); } }
    virtual void DrawBottom() { if (m_Ruler != nullptr) { m_Ruler->DoRenderBottom(m_Bottom); } }
    virtual void MouseButton(int x, int y, int button, int status);
    virtual void MouseMotion(int x, int y);
    virtual void Keyboard(int x, int y, size_t key, bool status);

protected:
    static void parseColor(color_t& rColor, const std::string& sColor);

public:
    CIdentifier m_ClassID = CIdentifier::undefined();
    std::vector<int> m_Parameters;
    uint64_t m_LastProcessTime = 0;

    TGtkGLWidget<CBoxAlgorithmViz> m_GtkGlWidget;
    std::map<std::string, CVertex> m_ChannelPositions;

    CRendererContext* m_RendererCtx    = nullptr;
    CRendererContext* m_SubRendererCtx = nullptr;
    IRuler* m_Ruler                    = nullptr;
    CMouse m_MouseHandler;

    CString m_Localisation;
    ETemporalCoherence m_TemporalCoherence = ETemporalCoherence::TimeLocked;
    uint64_t m_TimeScale                   = 0;
    size_t m_NElement                      = 0;
    double m_DataScale                     = 0.0;
    CString m_Caption;
    uint32_t m_TextureID  = 0;
    size_t m_NFlowerRing  = 0;
    double m_Translucency = 0.0;
    CString m_ColorGradient;
    bool m_ShowAxis        = false;
    bool m_XYZPlotHasDepth = false;
    bool m_IsPositive      = false;
    bool m_IsTimeLocked    = false;
    bool m_IsScaleVisible  = false;
    std::vector<color_t> m_Colors;
    color_t m_Color;

    CIdentifier m_TypeID = CIdentifier::undefined();
    uint64_t m_Time1     = 0;
    uint64_t m_Time2     = 0;

    float m_FastForwardMaxFactorHd = 0.0;
    float m_FastForwardMaxFactorLd = 0.0;

    std::vector<float> m_Swaps;

    GtkBuilder* m_Builder = nullptr;

    GtkWidget* m_Viewport    = nullptr;
    GtkWidget* m_Top         = nullptr;
    GtkWidget* m_Left        = nullptr;
    GtkWidget* m_Right       = nullptr;
    GtkWidget* m_Bottom      = nullptr;
    GtkWidget* m_CornerLeft  = nullptr;
    GtkWidget* m_CornerRight = nullptr;

    GtkWidget* m_TimeScaleW       = nullptr;
    GtkWidget* m_NElementW        = nullptr;
    GtkWidget* m_ERPRange         = nullptr;
    GtkWidget* m_ERPPlayerButton  = nullptr;
    GtkWidget* m_ERPPlayer        = nullptr;
    GtkWidget* m_ScaleVisible     = nullptr;
    GtkWidget* m_FrequencyBandMin = nullptr;
    GtkWidget* m_FrequencyBandMax = nullptr;

    GtkTreeView* m_ChannelTreeView   = nullptr;
    GtkListStore* m_ChannelListStore = nullptr;

    size_t m_Width  = 0;
    size_t m_Height = 0;

    bool m_RebuildNeeded      = false;
    bool m_RefreshNeeded      = false;
    bool m_RedrawNeeded       = false;
    uint64_t m_LastRenderTime = 0;

    bool m_IsVideoOutputEnabled = false; // for video output
    bool m_IsVideoOutputWorking = false;
    size_t m_FrameId            = 0;
    CString m_FrameFilenameFormat;

private:
    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
};

class CBoxAlgorithmVizListener : public Toolkit::TBoxListener<Plugins::IBoxListener>
{
public:
    explicit CBoxAlgorithmVizListener(const std::vector<int>& parameters) : m_Parameters(parameters) { }

    size_t GetBaseSettingCount() const
    {
        size_t result = 0;
        for (const auto& p : m_Parameters) {
            // if(p == I_Matrix) { result++; }
            // if(p == I_Signal) { result++; }
            // if(p == I_Spectrum) { result++; }
            // if(p == I_Covariance) { result++; }
            // if(p == I_Stimulations) { result++; }
            if (p == S_ChannelLocalisation) { result++; }
            if (p == S_TemporalCoherence) { result++; }
            if (p == S_TimeScale) { result++; }
            if (p == S_ElementCount) { result++; }
            if (p == S_DataScale) { result++; }
            if (p == S_Caption) { result++; }
            if (p == S_DataPositive) { result++; }
            if (p == S_Translucency) { result++; }
            if (p == S_FlowerRingCount) { result++; }
            if (p == S_Color) { result++; }
            if (p == S_ColorGradient) { result++; }
            if (p == S_ShowAxis) { result++; }
            if (p == S_XYZPlotHasDepth) { result++; }
            // if(p == F_CanAddInput) { result++; }
            // if(p == F_FixedChannelOrder) { result++; }
            // if(p == F_FixedChannelSelection) { result++; }
        }
        return result;
    }

    bool onInitialized(Kernel::IBox& /*box*/) override
    {
#ifdef TARGET_OS_Windows
        //box.addAttribute(OV_AttributeId_Box_DocumentationURLBase, OpenViBE::CString("${Path_Root}/doc/Mensia Advanced Visualization Toolkit/Mensia Advanced Visualization Toolkit.chm::"));
#endif
        return true;
    }

    bool onDefaultInitialized(Kernel::IBox& box) override
    {
        const bool isSignal     = (std::find(m_Parameters.begin(), m_Parameters.end(), I_Signal) != m_Parameters.end());
        const bool isSpectrum   = (std::find(m_Parameters.begin(), m_Parameters.end(), I_Spectrum) != m_Parameters.end());
        const bool isCovariance = (std::find(m_Parameters.begin(), m_Parameters.end(), I_Covariance) != m_Parameters.end());
        CIdentifier typeID      = CIdentifier::undefined();

        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.getInputType(i, typeID);
            if (typeID == OV_TypeId_StreamedMatrix) {
                if (isSignal) { box.setInputType(i, OV_TypeId_Signal); }
                if (isSpectrum) { box.setInputType(i, OV_TypeId_Spectrum); }
                if (isCovariance) { box.setInputType(i, OV_TypeId_CovarianceMatrix); }
            }
        }
        return true;
    }

    _IsDerivedFromClass_Final_(OpenViBE::Toolkit::TBoxListener<OpenViBE::Plugins::IBoxListener>, OpenViBE::CIdentifier::undefined())

    std::vector<int> m_Parameters;
};

class CBoxAlgorithmVizDesc : public Plugins::IBoxAlgorithmDesc
{
public:
    CString m_Name;
    CString m_CategoryName;
    CString m_ShortDesc;
    CString m_DetailedDesc;
    CIdentifier m_DescClassID = CIdentifier::undefined();
    CIdentifier m_ClassID     = CIdentifier::undefined();
    std::vector<int> m_Parameters;

    CBoxAlgorithmVizDesc(const CString& name, const CIdentifier& descClassID, const CIdentifier& classID,
                         const CParameterSet& parameterSet, const CString& shortDesc, const CString& detailedDesc)
        : m_ShortDesc(shortDesc), m_DetailedDesc(detailedDesc), m_DescClassID(descClassID), m_ClassID(classID),
          m_Parameters(parameterSet)
    {
        const std::string fullname(name.toASCIIString());
        const size_t i = fullname.rfind('/');
        if (i != std::string::npos) {
            m_Name         = CString(fullname.substr(i + 1).c_str());
            m_CategoryName = CString(fullname.substr(0, i).c_str());
        }
        else {
            m_Name         = CString(name);
            m_CategoryName = CString("");
        }
    }

    void release() override { }

    CString getName() const override { return m_Name; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies SA"; }
    CString getShortDescription() const override { return m_ShortDesc; }
    CString getDetailedDescription() const override { return m_DetailedDesc; }
    // CString getCategory() const override { return ""; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-find"; }

    CIdentifier getCreatedClass() const override { return m_ClassID; }

    void releaseBoxListener(Plugins::IBoxListener* listener) const override { delete listener; }

    bool hasFunctionality(const Plugins::EPluginFunctionality functionality) const override
    {
        return functionality == Plugins::EPluginFunctionality::Visualization;
    }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        for (const auto p : m_Parameters) {
            if (p == I_Matrix) { prototype.addInput("Matrix", OV_TypeId_StreamedMatrix); }
            if (p == I_Signal) { prototype.addInput("Matrix", OV_TypeId_StreamedMatrix); }          // This is later changed in the listener 
            if (p == I_Spectrum) { prototype.addInput("Matrix", OV_TypeId_StreamedMatrix); }        // This is later changed in the listener 
            if (p == I_TimeFrequency) { prototype.addInput("Matrix", OV_TypeId_TimeFrequency); }    // This is later changed in the listener
            if (p == I_Covariance) { prototype.addInput("Matrix", OV_TypeId_StreamedMatrix); }      // This is later changed in the listener 
            if (p == I_Stimulations) { prototype.addInput("Markers", OV_TypeId_Stimulations); }
            // "../share/electrode_sets/electrode_set_standard_cartesian.txt" 
            if (p == S_ChannelLocalisation) { prototype.addSetting("Channel Localisation", OV_TypeId_Filename, "${AdvancedViz_ChannelLocalisation}"); }
            if (p == S_DataPositive) { prototype.addSetting("Positive Data Only ?", OV_TypeId_Boolean, "false"); }
            if (p == S_TemporalCoherence) { prototype.addSetting("Temporal Coherence", TypeId_TemporalCoherence, "Time Locked"); }
            if (p == S_TimeScale) { prototype.addSetting("Time Scale", OV_TypeId_Float, "20"); }
            if (p == S_ElementCount) { prototype.addSetting("Matrix Count", OV_TypeId_Integer, "50"); }
            if (p == S_DataScale) { prototype.addSetting("Gain", OV_TypeId_Float, "1"); }
            if (p == S_Caption) { prototype.addSetting("Caption", OV_TypeId_String, ""); }
            if (p == S_FlowerRingCount) { prototype.addSetting("Flower Ring Count", OV_TypeId_Integer, "1"); }
            if (p == S_Translucency) { prototype.addSetting("Translucency", OV_TypeId_Float, "1"); }
            if (p == S_ShowAxis) { prototype.addSetting("Show Axis", OV_TypeId_Boolean, "true"); }
            if (p == S_XYZPlotHasDepth) { prototype.addSetting("Use third channel as depth", OV_TypeId_Boolean, "false"); } // XYZ Plot
            if (p == S_Color) { prototype.addSetting("Color", OV_TypeId_Color, "${AdvancedViz_DefaultColor}"); }
            if (p == S_ColorGradient) { prototype.addSetting("Color", OV_TypeId_ColorGradient, "${AdvancedViz_DefaultColorGradient}"); }
            if (p == F_CanAddInput) { prototype.addFlag(Kernel::BoxFlag_CanAddInput); }
            if (p == F_FixedChannelOrder) {} // prototype.addFlag(OpenViBE::Kernel::BoxFlag_);
            if (p == F_FixedChannelSelection) {} // prototype.addFlag(OpenViBE::Kernel::BoxFlag_);
        }
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
