///-------------------------------------------------------------------------------------------------
/// 
/// \file RendererTools.hpp
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

#if defined TARGET_OS_Windows
#include <Windows.h>
#endif // TARGET_OS_Windows

#if defined TARGET_OS_MacOS
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <cmath>

inline void cube(const float size = 1)
{
    glBegin(GL_QUADS);

    glVertex3f(size, -size, size);
    glVertex3f(size, size, size);
    glVertex3f(size, size, -size);
    glVertex3f(size, -size, -size);

    glVertex3f(-size, -size, size);
    glVertex3f(-size, -size, -size);
    glVertex3f(-size, size, -size);
    glVertex3f(-size, size, size);

    glVertex3f(-size, size, size);
    glVertex3f(size, size, size);
    glVertex3f(size, size, -size);
    glVertex3f(-size, size, -size);

    glVertex3f(-size, -size, size);
    glVertex3f(-size, -size, -size);
    glVertex3f(size, -size, -size);
    glVertex3f(size, -size, size);

    glVertex3f(-size, size, size);
    glVertex3f(-size, -size, size);
    glVertex3f(size, -size, size);
    glVertex3f(size, size, size);

    glVertex3f(-size, size, -size);
    glVertex3f(-size, -size, -size);
    glVertex3f(size, -size, -size);
    glVertex3f(size, size, -size);

    glEnd();
}

void perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar );

void build1DMipmapsGL(GLenum target, GLint internalFormat, GLsizei width, GLenum format, GLenum type, const void* data);

void ortho2DGL(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
