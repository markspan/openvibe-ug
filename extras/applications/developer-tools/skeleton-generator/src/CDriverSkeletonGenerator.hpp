#pragma once

#include "CSkeletonGenerator.hpp"

#include <vector>
#include <map>

namespace OpenViBE {
namespace SkeletonGenerator {
class CDriverSkeletonGenerator final : public CSkeletonGenerator
{
public:
    CDriverSkeletonGenerator(Kernel::IKernelContext& ctx, GtkBuilder* builder) : CSkeletonGenerator(ctx, builder) { }
    ~CDriverSkeletonGenerator() override { }

    bool initialize() override;
    bool save(const std::string& filename) override;
    bool load(const std::string& filename) override;
    void getCurrentParameters() override;

    std::string m_DriverName;
    std::string m_ClassName;
    std::string m_Samplings;
    std::vector<std::string> m_SamplingSeparate;
    std::string m_MinChannel;
    std::string m_MaxChannel;

    void ButtonCheckCB();
    void ButtonOkCB();
    void ButtonTooltipCB(GtkButton* button);
    void ButtonExitCB();

private:
    Kernel::ILogManager& getLogManager() const override { return m_kernelCtx.getLogManager(); }
    Kernel::CErrorManager& getErrorManager() const override { return m_kernelCtx.getErrorManager(); }

    enum class EWidgetName
    {
        DriverName,
        ClassName,
        ChannelCount,
        Sampling,
        Directory,
    };

    std::map<GtkButton*, EWidgetName> m_widgetNames;
};
}  // namespace SkeletonGenerator
}  // namespace OpenViBE
