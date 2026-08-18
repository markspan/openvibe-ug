#include "ovvizColorGradient.h"

#include <cstdio>
#include <string>
#include <map>

namespace OpenViBE {
namespace VisualizationToolkit {
namespace {
typedef struct
{
    double percent, red, green, blue;
} SColor;
}  // namespace

bool ColorGradient::parse(CMatrix& colorGradient, const CString& string)
{
    const std::string colorString(string.toASCIIString());
    size_t startPosition = 0;

    std::map<double, SColor> colorGradientVector;

    do {
        size_t endPosition = colorString.find(OV_Value_EnumeratedStringSeparator, startPosition);
        if (endPosition == std::string::npos) { endPosition = colorString.length(); }

        std::string colorSubString;
        colorSubString.assign(colorString, startPosition, endPosition - startPosition);

        int p, r, g, b;
        if (sscanf(colorSubString.c_str(), "%i:%i,%i,%i", &p, &r, &g, &b) == 4) {
            SColor color;
            color.percent                      = p;
            color.red                          = r;
            color.green                        = g;
            color.blue                         = b;
            colorGradientVector[color.percent] = color;
        }

        startPosition = endPosition + 1;
    } while (startPosition < colorString.length());

    colorGradient.resize(4, colorGradientVector.size());

    size_t i = 0;
    for (auto it = colorGradientVector.begin(); it != colorGradientVector.end(); ++it, i++) {
        colorGradient[i * 4]     = it->second.percent;
        colorGradient[i * 4 + 1] = it->second.red;
        colorGradient[i * 4 + 2] = it->second.green;
        colorGradient[i * 4 + 3] = it->second.blue;
    }

    return true;
}

bool ColorGradient::format(CString& string, const CMatrix& colorGradient)
{
    if (colorGradient.getDimensionCount() != 2) { return false; }

    if (colorGradient.getDimensionSize(0) != 4) { return false; }

    std::string separator("  ");
    separator[0] = OV_Value_EnumeratedStringSeparator;

    std::string result;
    for (size_t i = 0; i < colorGradient.getDimensionSize(1); ++i) {
        char buffer[1024];
        sprintf(buffer, "%.0lf:%i,%i,%i", colorGradient[i * 4], int(colorGradient[i * 4 + 1]), int(colorGradient[i * 4 + 2]), int(colorGradient[i * 4 + 3]));
        result += (i == 0 ? "" : separator);
        result += buffer;
    }

    string = result.c_str();
    return true;
}

bool ColorGradient::interpolate(CMatrix& interpolatedColorGradient, const CMatrix& colorGradient, const size_t steps)
{
    size_t i;

    if (colorGradient.getDimensionCount() != 2 || colorGradient.getDimensionSize(0) != 4) { return false; }

    if (steps <= 1) { return false; }

    interpolatedColorGradient.resize(4, steps);

    std::map<double, SColor> colors;

    for (i = 0; i < colorGradient.getDimensionSize(1); ++i) {
        SColor color;
        color.percent         = colorGradient[i * 4];
        color.red             = colorGradient[i * 4 + 1];
        color.green           = colorGradient[i * 4 + 2];
        color.blue            = colorGradient[i * 4 + 3];
        colors[color.percent] = color;
    }

    if (colors.find(0) == colors.end()) {
        SColor color  = colors.begin()->second;
        color.percent = 0;
        colors[0]     = color;
    }

    if (colors.find(100) == colors.end()) {
        SColor color  = colors.rbegin()->second;
        color.percent = 100;
        colors[100]   = color;
    }

    auto it1 = colors.begin();
    auto it2 = colors.begin();
    ++it2;

    for (i = 0; i < steps; ++i) {
        const double t = double(100 * i) / (steps - 1);
        while (it2->first < t) {
            ++it1;
            ++it2;
        }

        const double a = it2->first - t;
        const double b = t - it1->first;
        const double d = it2->first - it1->first;

        interpolatedColorGradient[i * 4]     = t;
        interpolatedColorGradient[i * 4 + 1] = (it1->second.red * a + it2->second.red * b) / d;
        interpolatedColorGradient[i * 4 + 2] = (it1->second.green * a + it2->second.green * b) / d;
        interpolatedColorGradient[i * 4 + 3] = (it1->second.blue * a + it2->second.blue * b) / d;
    }

    return true;
}

}  // namespace VisualizationToolkit
}  // namespace OpenViBE
