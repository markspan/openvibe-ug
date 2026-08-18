#include <mensia/advanced-visualization.hpp>

namespace OAV = OpenViBE::AdvancedVisualization;

int main()
{
    OAV::CRendererContext context;
    context.Clear();

    context.SetDataType(OAV::CRendererContext::EDataType::Matrix);

    OAV::IRenderer* rend = OAV::IRenderer::Create(OAV::ERendererType::Bitmap, false);

    rend->SetChannelCount(10);
    auto* tmp = new float[666];
    rend->Feed(tmp);
    rend->Refresh(context);
}
