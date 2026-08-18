///-------------------------------------------------------------------------------------------------
/// 
/// \file ovp_defines.h
/// \brief Defines list for Setting, Shortcut Macro and const.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/02/2020.
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
#include <string>

// Boxes
//---------------------------------------------------------------------------------------------------
#define OVP_ClassId_BoxAlgorithm_FeaturesSelection          OpenViBE::CIdentifier(0xee36249f, 0x22a32e6e)
#define OVP_ClassId_BoxAlgorithm_FeaturesSelectionDesc      OpenViBE::CIdentifier(0xee36249f, 0x22a32e6f)
#define OVP_ClassId_BoxAlgorithm_FeaturesSelector           OpenViBE::CIdentifier(0xee36249f, 0x22a32e7e)
#define OVP_ClassId_BoxAlgorithm_FeaturesSelectorDesc       OpenViBE::CIdentifier(0xee36249f, 0x22a32e7f)

// Types Lists
//---------------------------------------------------------------------------------------------------
#define OVP_TypeId_Features_Selection_Method                OpenViBE::CIdentifier(0x5261636B, 0x46534D45)
#define OVP_TypeId_mRMR_Method                              OpenViBE::CIdentifier(0x5261636B, 0x6D524D52)

enum class EFeatureSelection { MRMR };

//inline std::string toString(const EFeatureSelection& e) { switch (e) { default: return "mRMR (minimum Redundancy Maximum Relevance)"; } }
inline std::string toString(const EFeatureSelection& /*e*/) { return "mRMR (minimum Redundancy Maximum Relevance)"; }
