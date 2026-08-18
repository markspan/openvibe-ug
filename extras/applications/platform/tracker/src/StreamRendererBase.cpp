//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
// @todo add horizontal scaling support
// @todo add event handlers
// @todo add ruler, stimulations, channel names, a million of other things

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <system/ovCTime.h>

#include <mensia/advanced-visualization.hpp>

#include "StreamRendererBase.h"

namespace OpenViBE {
namespace Tracker {

bool StreamRendererBase::initialize()
{
    GtkBuilder* builder    = gtk_builder_new();
    const CString filename = Directories::getDataDir() + "/applications/tracker/advanced-visualization.ui";
    GError* errorCode      = nullptr;
    if (!gtk_builder_add_from_file(builder, filename, &errorCode)) {
        log() << Kernel::LogLevel_Error << "Problem loading [" << filename << "] : "
                << (errorCode ? errorCode->code : 0) << " " << (errorCode ? errorCode->message : "") << "\n";
        g_object_unref(builder);
        return false;
    }

    GtkWidget* window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    m_main            = GTK_WIDGET(gtk_builder_get_object(builder, "table"));
    // ::GtkWidget* toolbar = GTK_WIDGET(gtk_builder_get_object(pBuilder, "toolbar-window"));

    gtk_widget_ref(m_main);
    gtk_container_remove(GTK_CONTAINER(window), m_main);
    // We keep one ref, the caller can unref after having assigned the widget

    m_viewport    = GTK_WIDGET(gtk_builder_get_object(builder, "viewport"));
    m_top         = GTK_WIDGET(gtk_builder_get_object(builder, "label_top")); // caption
    m_left        = GTK_WIDGET(gtk_builder_get_object(builder, "drawingarea_left"));
    m_right       = GTK_WIDGET(gtk_builder_get_object(builder, "drawingarea_right"));
    m_bottom      = GTK_WIDGET(gtk_builder_get_object(builder, "drawingarea_bottom"));
    m_cornerLeft  = GTK_WIDGET(gtk_builder_get_object(builder, "label_corner_left"));
    m_cornerRight = GTK_WIDGET(gtk_builder_get_object(builder, "label_corner_right"));

    // @note this or something similar is needed or otherwise the widgets will all be crammed to the 
    // same fixed size aperture with no scrolling
    // @todo give users some scaling options
    // @fixme for some reason this causes a mess on the 'message' bar below in the UI, as if it didn't Redraw properly
    gtk_widget_set_size_request(m_main, 640, 200);

    m_color.r = 1;
    m_color.g = 1;
    m_color.b = 1;

    g_object_unref(builder);
    builder = nullptr;

    return true;
}

bool StreamRendererBase::uninitialize()
{
    for (size_t i = 0; i < m_renderers.size(); ++i) { AdvancedVisualization::IRenderer::Release(m_renderers[i]); }
    m_renderers.clear();

    if (m_rendererCtx) {
        delete m_rendererCtx;
        m_rendererCtx = nullptr;
    }

    if (m_subRendererCtx) {
        delete m_subRendererCtx;
        m_subRendererCtx = nullptr;
    }

    if (m_ruler) {
        delete m_ruler;
        m_ruler = nullptr;
    }

    return true;
}

bool StreamRendererBase::setTitle(const char* title)
{
    if (title) { gtk_label_set_text(GTK_LABEL(m_top), title); }
    else { gtk_label_set_text(GTK_LABEL(m_top), ""); }
    return true;
}

bool StreamRendererBase::setRulerVisibility(const bool isVisible)
{
    m_isScaleVisible = isVisible;
    return updateRulerVisibility();
}

bool StreamRendererBase::updateRulerVisibility()
{
    /*
    if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_pScaleVisible)) ? true : false) != m_isScaleVisible)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pScaleVisible), m_isScaleVisible);
    }
    */

    void (*action)(GtkWidget*) = m_isScaleVisible ? gtk_widget_show : gtk_widget_hide;
    (*action)(this->m_top);
    (*action)(this->m_left);
    (*action)(this->m_right);
    (*action)(this->m_bottom);
    (*action)(this->m_cornerLeft);
    (*action)(this->m_cornerRight);

    return true;
}

bool StreamRendererBase::realize()
{
    gtk_widget_realize(m_top);
    gtk_widget_realize(m_left);
    gtk_widget_realize(m_right);
    gtk_widget_realize(m_bottom);
    gtk_widget_realize(m_viewport);
    gtk_widget_realize(m_main);

    return true;
}


bool StreamRendererBase::Redraw(const bool bImmediate /* = false */)
{
    m_gtkGLWidget.Redraw(bImmediate);
    m_gtkGLWidget.RedrawLeft(bImmediate);
    m_gtkGLWidget.RedrawRight(bImmediate);
    m_gtkGLWidget.RedrawBottom(bImmediate);

    return true;
}

bool StreamRendererBase::Reshape(const uint32_t width, const uint32_t height)
{
    m_width  = uint32_t(width);
    m_height = uint32_t(height);
    m_rendererCtx->SetAspect(width * 1.0F / height);

    return true;
}

bool StreamRendererBase::Draw()
{
    StreamRendererBase::PreDraw();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    //::glClearColor(1.0,1.0,1.0,1.0);
    glColor4f(m_color.r, m_color.g, m_color.b, m_rendererCtx->GetTranslucency());

    if (m_rotate) {
        glScalef(1, -1, 1);
        glRotatef(-90, 0, 0, 1);
    }

    m_renderers[0]->Render(*m_rendererCtx);
    glPopAttrib();

    StreamRendererBase::PostDraw();

    return true;
}

void StreamRendererBase::DrawLeft() { if (m_ruler) { m_ruler->DoRenderLeft(m_left); } }
void StreamRendererBase::DrawRight() { if (m_ruler) { m_ruler->DoRenderRight(m_right); } }
void StreamRendererBase::DrawBottom() { if (m_ruler) { m_ruler->DoRenderBottom(m_bottom); } }

bool StreamRendererBase::PreDraw()
{
    this->updateRulerVisibility();

    //  auto m_sColorGradient=CString("0:0,0,0; 100:100,100,100");

    const char* gradient =
            "0:100, 100, 100; 12:50, 100, 100; 25:0, 50, 100; 38:0, 0, 50; 50:0, 0, 0; 62:50, 0, 0; 75:100, 50, 0; 88:100, 100, 50; 100:100, 100, 100";

    if (!m_textureID) { m_textureID = m_gtkGLWidget.CreateTexture(gradient); }
    glBindTexture(GL_TEXTURE_1D, m_textureID);

    m_rendererCtx->SetAspect(m_viewport->allocation.width * 1.0F / m_viewport->allocation.height);

    return true;
}


bool StreamRendererBase::PostDraw()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if (m_ruler) { m_ruler->DoRender(); }
    glPopAttrib();

    return true;
}

bool StreamRendererBase::MouseButton(int /*x*/, int /*y*/, const int button, const int status)
{
    m_buttons[button] = status;

    /*
    if (button == 1 && status == 1)
    {
        m_isScaleVisible = !m_isScaleVisible;
        m_pRendererContext->setScaleVisibility(m_isScaleVisible);
    }
    */

    return true;
}

bool StreamRendererBase::MouseMotion(const int x, const int y)
{
    if (!m_mouseInitialized) {
        m_mouseX           = x;
        m_mouseY           = y;
        m_mouseInitialized = true;
    }

    if (m_buttons[3]) {
        const float value = powf(0.99F, float(y - m_mouseY));
        //      std::cout << "scale " << value << "\n";
        m_rendererCtx->ScaleBy(value);
        Redraw();
    }
    if (m_buttons[2]) {
        const float value = powf(0.99F, float(y - m_mouseY));
        //      std::cout << "zoom " << value << "\n";
        m_rendererCtx->ZoomBy(value);
    }
    if (m_buttons[1]) {
        //      std::cout << "Rotate\n";
        m_rendererCtx->RotateByY(float(x - m_mouseX) * 0.1F);
        m_rendererCtx->RotateByX(float(y - m_mouseY) * 0.1F);
    }

    m_mouseX = x;
    m_mouseY = y;

    return true;
}

bool StreamRendererBase::finalize()
{
    for (size_t i = 0; i < m_renderers.size(); ++i) {
        m_renderers[i]->Rebuild(*m_rendererCtx);
        m_renderers[i]->Refresh(*m_rendererCtx);
    }
    Redraw(true);

    return true;
}

bool StreamRendererBase::restoreSettings(const std::string& prefix)
{
    if (!m_rendererCtx) { return false; }

    // Lets see if we have a scale token
    const std::string token = std::string("${") + prefix + "_Scale}";
    const float newScale    = float(m_kernelCtx.getConfigurationManager().expandAsFloat(token.c_str(), double(m_rendererCtx->GetScale())));
    m_rendererCtx->SetScale(newScale);

    return true;
}

bool StreamRendererBase::storeSettings(const std::string& prefix)
{
    if (!m_rendererCtx) { return false; }

    const std::string token = prefix + "_Scale";
    std::stringstream value;
    value << m_rendererCtx->GetScale();
    m_kernelCtx.getConfigurationManager().addOrReplaceConfigurationToken(token.c_str(), value.str().c_str());

    return true;
}


CString StreamRendererBase::renderAsText(const size_t indent) const
{
    return (std::string(indent, ' ') + "Detail printing unimplemented for stream type or placeholder renderer in use\n").c_str();
}

void add_column(GtkTreeView* treeView, const char* name, const uint32_t id, const uint32_t minWidth)
{
    GtkTreeViewColumn* column = gtk_tree_view_column_new();
    GtkCellRenderer* cell     = gtk_cell_renderer_text_new();
    gtk_tree_view_column_set_title(column, name);
    gtk_tree_view_column_pack_start(column, cell, TRUE);
    gtk_tree_view_column_set_attributes(column, cell, "text", id, nullptr);
    gtk_tree_view_column_set_sort_column_id(column, id);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_min_width(column, minWidth);
    gtk_tree_view_append_column(treeView, column);
    gtk_tree_view_column_set_sort_indicator(column, TRUE);
}

}  // namespace Tracker
}  // namespace OpenViBE
