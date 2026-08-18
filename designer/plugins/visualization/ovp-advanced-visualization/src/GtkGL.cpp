///-------------------------------------------------------------------------------------------------
/// 
/// \file GtkGL.cpp
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

#include "GtkGL.hpp"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#if defined TARGET_OS_Windows
#include <Windows.h>
#include <gdk/gdkwin32.h>
#elif defined TARGET_OS_Linux
#include <gdk/gdkx.h>
#include <GL/glx.h>
#elif defined TARGET_OS_MacOS
#include <gdk/gdk.h>
#include <GL/glx.h>
#else
#error unsupported platform
#endif

// ###########################################################################################################################################################
// ###########################################################################################################################################################
//
// GtkGL implementation
//
// ###########################################################################################################################################################
// ###########################################################################################################################################################

#define GTK_GL_RENDERING_CONTEXT_NAME "GL Rendering Context"
#define GTK_GL_DEVICE_CONTEXT_NAME "Device Context"
#define GTK_GL_DEBUG(s) // g_debug("GtkGL : "#s);
#define GTK_GL_WARNING(s) g_warning("GtkGL : "#s);

#if defined TARGET_OS_Windows

namespace {
typedef bool (*gl_swap_interval_ext_t)(int);
gl_swap_interval_ext_t wglSwapIntervalEXT = nullptr;
}  // namespace

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

namespace
{
    typedef void (*glXSwapIntervalEXT_t)(Display*, GLXDrawable, int);
    glXSwapIntervalEXT_t glXSwapIntervalEXT = nullptr;
}

#endif

// ##  WINDOWS  ##############################################################################################################################################

#if defined TARGET_OS_Windows

namespace {
void OnRealizeCB(GtkWidget* widget, void* /*data*/)
{
    GTK_GL_DEBUG("realize-callback");

    gdk_window_ensure_native(gtk_widget_get_window(widget));

    const HWND window    = HWND(GDK_WINDOW_HWND(gtk_widget_get_window(widget)));
    const HDC drawingCtx = GetDC(window);

    PIXELFORMATDESCRIPTOR pixelFormatDesc;
    pixelFormatDesc.nSize      = sizeof(pixelFormatDesc);
    pixelFormatDesc.nVersion   = 1;
    pixelFormatDesc.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
    pixelFormatDesc.cColorBits = 24;
    pixelFormatDesc.cAlphaBits = 8;
    pixelFormatDesc.cDepthBits = 32;
    pixelFormatDesc.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormatID = ChoosePixelFormat(drawingCtx, &pixelFormatDesc);

    if (pixelFormatID == 0) {
        GTK_GL_WARNING("ChoosePixelFormat failed")
        GTK_GL_DEBUG("realize-callback::failed");
        return;
    }

    if (SetPixelFormat(drawingCtx, pixelFormatID, &pixelFormatDesc) == 0) {
        GTK_GL_WARNING("SetPixelFormat failed")
        GTK_GL_DEBUG("realize-callback::failed");
        return;
    }

    const HGLRC glRenderingCtx = wglCreateContext(drawingCtx);
    if (glRenderingCtx == nullptr) {
        GTK_GL_WARNING("wglCreateContext failed")
        GTK_GL_DEBUG("realize-callback::failed");
        return;
    }

    g_object_set_data(G_OBJECT(widget), GTK_GL_RENDERING_CONTEXT_NAME, glRenderingCtx);
    g_object_set_data(G_OBJECT(widget), GTK_GL_DEVICE_CONTEXT_NAME, drawingCtx);

    gtk_widget_queue_resize(widget);
    gtk_widget_set_double_buffered(widget, FALSE);

    wglSwapIntervalEXT = gl_swap_interval_ext_t(wglGetProcAddress("wglSwapIntervalEXT"));

    GTK_GL_DEBUG("realize-callback::success");
}
}  // namespace

void OpenViBE::AdvancedVisualization::GtkGL::initialize(GtkWidget* widget)
{
    GTK_GL_DEBUG("initialize");
    g_signal_connect(widget, "realize", G_CALLBACK(OnRealizeCB), nullptr);
    GTK_GL_DEBUG("initialize::success");
}

void OpenViBE::AdvancedVisualization::GtkGL::uninitialize(GtkWidget* widget)
{
    GTK_GL_DEBUG("uninitialize");

    const HWND window = HWND(GDK_WINDOW_HWND(gtk_widget_get_window(widget)));

    const auto glRenderingCtx = HGLRC(g_object_get_data(G_OBJECT(widget), GTK_GL_RENDERING_CONTEXT_NAME));
    wglDeleteContext(glRenderingCtx);

    const HDC drawingCtx = HDC(g_object_get_data(G_OBJECT(widget), GTK_GL_DEVICE_CONTEXT_NAME));
    ReleaseDC(window, drawingCtx);

    GTK_GL_DEBUG("uninitialize::success");
}

void OpenViBE::AdvancedVisualization::GtkGL::preRender(GtkWidget* widget, const bool verticalSync)
{
    GTK_GL_DEBUG("pre-render");

    const HDC drawingCtx      = HDC(g_object_get_data(G_OBJECT(widget), GTK_GL_DEVICE_CONTEXT_NAME));
    const auto glRenderingCtx = HGLRC(g_object_get_data(G_OBJECT(widget), GTK_GL_RENDERING_CONTEXT_NAME));

    if (glRenderingCtx == nullptr) {
        GTK_GL_DEBUG("Rendering context not ready");
        GTK_GL_DEBUG("pre-render::failed");
        return;
    }

    if (wglMakeCurrent(drawingCtx, glRenderingCtx) == 0) {
        GTK_GL_WARNING("wglMakeCurrent failed")
        GTK_GL_DEBUG("pre-render::failed");
        return;
    }

    // Enable / Disable vsync
    if (wglSwapIntervalEXT != nullptr) { wglSwapIntervalEXT(verticalSync ? 1 : 0); }

    GTK_GL_DEBUG("pre-render::success");
}

void OpenViBE::AdvancedVisualization::GtkGL::postRender(GtkWidget* widget)
{
    GTK_GL_DEBUG("post-render");

    const HDC drawingCtx = HDC(g_object_get_data(G_OBJECT(widget), GTK_GL_DEVICE_CONTEXT_NAME));

    if (drawingCtx == nullptr) {
        GTK_GL_DEBUG("Rendering context not ready");
        GTK_GL_DEBUG("post-render::failed");
    }
    else if (SwapBuffers(drawingCtx) == 0) {
        GTK_GL_WARNING("SwapBuffers failed")
        GTK_GL_DEBUG("post-render::failed");
    }
    else if (wglMakeCurrent(drawingCtx, nullptr) == 0) {
        GTK_GL_WARNING("wglMakeCurrent failed")
        GTK_GL_DEBUG("post-render::failed");
    }
    else { GTK_GL_DEBUG("post-render::success"); }
}

// ##  WINDOWS  ##############################################################################################################################################

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

// ##  LINUX  ################################################################################################################################################

void OpenViBE::AdvancedVisualization::GtkGL::initialize(GtkWidget * widget)
{
    GTK_GL_DEBUG("initialize");

    // ::gdk_window_ensure_native(gtk_widget_get_window(widget));

    ::GdkScreen* screen = ::gdk_screen_get_default();
    ::Display* display = GDK_SCREEN_XDISPLAY(screen);
    ::gint screenNumber = GDK_SCREEN_XNUMBER(screen);

    int visualInfoAttributes[] =
    {
        GLX_RGBA,
        GLX_RED_SIZE,    1,
        GLX_GREEN_SIZE,  1,
        GLX_BLUE_SIZE,   1,
        GLX_ALPHA_SIZE,  1,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE,  1,
        None
    };

    if (!::glXQueryVersion(display, nullptr, nullptr))
    {
        GTK_GL_WARNING("initialize::failed");
        return;
    }

    ::XVisualInfo* visualInfo = ::glXChooseVisual(display, screenNumber, visualInfoAttributes);
    ::GLXContext glRenderingCtx = glXCreateContext(display, visualInfo, nullptr, True);
    g_object_set_data(G_OBJECT(widget), GTK_GL_RENDERING_CONTEXT_NAME, glRenderingCtx);

#if 1
    /* Fix up colormap */
    ::GdkVisual* visual = ::gdk_x11_screen_lookup_visual(screen, visualInfo->visualid);
    ::GdkColormap* colorMap = ::gdk_colormap_new(visual, FALSE);
    ::gtk_widget_set_colormap(widget, colorMap);
#endif

    ::gtk_widget_queue_resize(widget);
    ::gtk_widget_set_double_buffered(widget, FALSE);

    glXSwapIntervalEXT = (glXSwapIntervalEXT_t) ::glXGetProcAddressARB(reinterpret_cast <const unsigned char*>("glXSwapIntervalEXT"));

    GTK_GL_DEBUG("initialize::success");
}

void OpenViBE::AdvancedVisualization::GtkGL::uninitialize(GtkWidget * widget)
{
    GTK_GL_DEBUG("uninitialize");

    ::Display* display = GDK_SCREEN_XDISPLAY(gtk_widget_get_screen(widget));
    ::GLXContext glRenderingCtx = (GLXContext) g_object_get_data(G_OBJECT(widget), GTK_GL_RENDERING_CONTEXT_NAME);
    if (!display || !glRenderingCtx)
    {
        GTK_GL_WARNING("uninitialize::failed");
        return;
    }
    ::glXMakeCurrent(display, None, nullptr);
    ::glXDestroyContext(display, glRenderingCtx);

    GTK_GL_DEBUG("uninitialize::success");
}

void OpenViBE::AdvancedVisualization::GtkGL::preRender(GtkWidget * widget, bool bVerticalSync)
{
    GTK_GL_DEBUG("pre-render");

    ::Display* display = GDK_SCREEN_XDISPLAY(gtk_widget_get_screen(widget));
    ::Window window = GDK_WINDOW_XID(gtk_widget_get_window(widget));
    ::GLXContext glRenderingCtx = (GLXContext) g_object_get_data(G_OBJECT(widget), GTK_GL_RENDERING_CONTEXT_NAME);
    if (!display || !glRenderingCtx)
    {
        GTK_GL_WARNING("pre-render::failed");
        return;
    }
    ::glXMakeCurrent(display, window, glRenderingCtx);

    // Enable / Disable vsync
    if (glXSwapIntervalEXT)
    {
        glXSwapIntervalEXT(display, ::glXGetCurrentDrawable(), bVerticalSync ? 1 : 0);
    }

    GTK_GL_DEBUG("pre-render::success");
}

void OpenViBE::AdvancedVisualization::GtkGL::postRender(GtkWidget * widget)
{
    GTK_GL_DEBUG("post-render");

    ::Display* display = GDK_SCREEN_XDISPLAY(gtk_widget_get_screen(widget));
    ::Window window = GDK_WINDOW_XID(gtk_widget_get_window(widget));
    if (!display)
    {
        GTK_GL_WARNING("post-render::failed");
        return;
    }
    ::glXSwapBuffers(display, window);

    GTK_GL_DEBUG("post-render::success");
}

// ##  LINUX  ################################################################################################################################################

#endif
