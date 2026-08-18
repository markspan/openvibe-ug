/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameters/parameterColorGradient.h"
#include "parameters/parameterFactory.h"
#include <sstream>

namespace OpenViBE {
namespace Parameters {

ParameterColorGradient::ParameterColorGradient(void) : ParameterBase<std::string, ParameterColorGradient>()
{
}

ParameterColorGradient::ParameterColorGradient(const std::string& value) : ParameterBase<std::string, ParameterColorGradient>(value)
{
    this->setValue(value, false); // No notification on initial value set
}

ParameterColorGradient::ParameterColorGradient(const std::string& name, const std::string& description, std::string value) : ParameterBase<std::string, ParameterColorGradient>(name, description, value)
{
    this->setValue(value, false); // No notification on initial value set
}

ParameterColorGradient::ParameterColorGradient(ParameterColorGradient&& param) noexcept : ParameterBase<std::string, ParameterColorGradient>(std::move(param))
{
    m_colors = std::move(param.m_colors); // Move the colors vector
}

ParameterColorGradient::ParameterColorGradient(const ParameterColorGradient& param) : ParameterBase<std::string, ParameterColorGradient>(param)
{
    m_colors = param.m_colors; // Copy the colors vector
}

ParameterColorGradient::~ParameterColorGradient(void)
{
}

ParameterColorGradient& ParameterColorGradient::operator=(ParameterColorGradient&& param) noexcept
{
    if (this != &param) {
        ParameterBase<std::string, ParameterColorGradient>::operator=(std::move(param));
        m_colors = std::move(param.m_colors);
    }
    return *this;
}

ParameterTraits* ParameterColorGradient::clone() const
{
    return new ParameterColorGradient(*this);
}

std::string ParameterColorGradient::toString() const
{
    return this->value();
}

void ParameterColorGradient::fromString(const std::string& value)
{
    this->setValue(value);
}

void ParameterColorGradient::setValue(const std::string& value, bool notify) 
{
    //1 check if value is correctly formatted
    // it will be in the format: 0:2,36,58; 50:100,100,100; 100:83,17,20
    if (value.empty()) {
        return;
    }

    auto &&newColors = validateValue(value);
    if(newColors) {
        m_colors = *newColors;
        m_value = value;
        if (notify) {
            sync(); // Notify the change if required
        }
    } else {
        sync(); // Notify the GUI that the change is incorrect
    }  
}

//interpolate the color gradient for a given number of steps
//color is r,g,b with range 0-255
std::vector<std::array<int, 4> > ParameterColorGradient::interpolate(int steps) const
{
    if (steps <= 1) {
        return {{0,0,0,0}};
    }
    std::vector<std::array<int, 4> > originColors = m_colors; // colors are stored as {percentage, r, g, b}
    std::vector<std::array<int, 4> > interpolatedColors = m_colors; // Store interpolated colors as {step, r, g, b}

    // Ensure we have colors for 0% and 100%
    if(originColors.front()[0] != 0 ) {
        std::array<int, 4> firstColor = {0, originColors.front()[1], originColors.front()[2], originColors.front()[3]};
        originColors.insert(originColors.begin(), firstColor);
    }
    if(originColors.back()[0] != 100 ) {
        std::array<int, 4> lastColor = {100, originColors.back()[1], originColors.back()[2], originColors.back()[3]};
        originColors.push_back(lastColor);
    }

    // Interpolate colors
    auto it1 = originColors.begin();
    auto it2 = originColors.begin();
    ++it2; //it2 is the next color after it1

    for (int i = 0; i < steps; ++i) {
        double t = double(100 * i) / (steps - 1);
        while (it2->at(0) < t) { 
            ++it1;
            ++it2;
        }

        double a = it2->at(0) - t;
        double b = t - it1->at(0);
        double d = it2->at(0) - it1->at(0);

        int red   = static_cast<int>((it1->at(1) * a + it2->at(1) * b) / d);
        int green = static_cast<int>((it1->at(2) * a + it2->at(2) * b) / d);
        int blue  = static_cast<int>((it1->at(3) * a + it2->at(3) * b) / d);

        interpolatedColors.push_back({i, red, green, blue});
    }

    // Return the number of steps and the interpolated colors
    return interpolatedColors;
}


std::optional<std::vector<std::array<int, 4>>> ParameterColorGradient::validateValue(const std::string& value)
{
    std::string buffer;
    std::stringstream ss(value);
    std::vector<std::array<int, 4> > newColors; // Store colors as {percentage, r, g, b}
    try {
        while(getline(ss,buffer, ';')) {
            // Check if the buffer is formatted correctly
            if (buffer.find(':') == std::string::npos || buffer.find(',') == std::string::npos) {
                throw std::invalid_argument("Invalid color gradient format: " + buffer + " in " + value);
            }

            // Check if the percentage is a valid integer
            std::string percentage = buffer.substr(0, buffer.find(':'));
            int perc = std::stoi(percentage);
            if (perc < 0 || perc > 100) {
                throw std::invalid_argument("Percentage out of range (0-100): " + value);
            }
            // Check if the color values are valid integers
            std::string colorValues = buffer.substr(buffer.find(':') + 1);
            std::stringstream colorStream(colorValues);
            std::string colorValue;
            int nb_colors = 0;
            int rgb[3];
            while (getline(colorStream, colorValue, ',')) {
                int color = std::stoi(colorValue);
                if (color < 0 || color > 255) {
                    throw std::invalid_argument("Color value out of range (0-255): " + colorValue + " in " + value);
                }
                rgb[nb_colors++] = color;
            }
            if (nb_colors != 3) {
                throw std::invalid_argument("Invalid number of color values (expected 3): " + colorValues + " in " + value);
            }

            auto it = std::lower_bound(newColors.begin(), newColors.end(), std::array<int, 4>{perc, rgb[0], rgb[1], rgb[2]},
                [](const std::array<int, 4>& a, const std::array<int, 4>& b) {
                    return a[0] < b[0]; // Compare based on the percentage
                });
            if (it != newColors.end() && it->at(0) == perc) {
                // If the percentage already exists, update the color
                it->at(1) = rgb[0];
                it->at(2) = rgb[1];
                it->at(3) = rgb[2];
            } else {
                // Insert the new color at the correct position
                newColors.insert(it, {perc, rgb[0], rgb[1], rgb[2]});
            }
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
        return std::nullopt;
    } catch (const std::out_of_range& e) {
        std::cerr << "Out of range error: " << e.what() << std::endl;
        return std::nullopt;
    } catch (...) {
        std::cerr << "An unexpected error occurred while parsing the color gradient." << std::endl;
        return std::nullopt;
    }

    return newColors;
}

// Register parameter type with factory
REGISTER_PARAMETER_TYPE("colorgradient", ParameterColorGradient);

} // namespace Parameters
} // namespace OpenViBE