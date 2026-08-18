///-------------------------------------------------------------------------------------------------
/// 
/// \file CVertex.hpp
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

#include <cmath>
#include <float.h>

#pragma pack(1)
namespace OpenViBE {
namespace AdvancedVisualization {
class CVertex
{
public:
    explicit CVertex(const double _x = 0, const double _y = 0, const double _z = 0, const double _u = 0, const double _v = 0)
        : x(float(_x)), y(float(_y)), z(float(_z)), u(float(_u)), v(float(_v)) { }

    CVertex(const CVertex& a, const CVertex& b)
        : x(b.x - a.x), y(b.y - a.y), z(b.z - a.z), u(b.u - a.u), v(b.v - a.v) { }

    float x = 0;
    float y = 0;
    float z = 0;
    float u = 0;
    float v = 0;

    CVertex& Normalize()

    {
        const float n = this->Length();
        if (n > FLT_EPSILON) {
            const float in = 1.0F / n;
            this->x *= in;
            this->y *= in;
            this->z *= in;
        }
        return *this;
    }

    float Length() const { return sqrt(this->SqrLength()); }

    float SqrLength() const { return Dot(*this, *this); }

    static float Dot(const CVertex& a, const CVertex& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    static CVertex Cross(const CVertex& a, const CVertex& b)
    {
        CVertex r;
        r.x = a.y * b.z - a.z * b.y;
        r.y = a.z * b.x - a.x * b.z;
        r.z = a.x * b.y - a.y * b.x;
        return r;
    }

    static CVertex Cross(const CVertex& a1, const CVertex& b1, const CVertex& a2, const CVertex& b2)
    {
        const CVertex v1(a1, b1);
        const CVertex v2(a2, b2);
        return Cross(v1, v2);
    }

    static bool IsOnSameSide(const CVertex& p1, const CVertex& p2, const CVertex& a, const CVertex& b)
    {
        const CVertex cp1 = Cross(a, b, a, p1);
        const CVertex cp2 = Cross(a, b, a, p2);
        return Dot(cp1, cp2) >= 0;
    }

    static bool IsInTriangle(const CVertex& p, const CVertex& a, const CVertex& b, const CVertex& c)
    {
        return IsOnSameSide(p, a, b, c) && IsOnSameSide(p, b, c, a) && IsOnSameSide(p, c, a, b);
    }
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
#pragma pack()
