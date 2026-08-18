///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines list for Setting, Shortcut Macro and const.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 26/10/2018.
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
/// \remarks 
/// - List of Estimator inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a> (<a href="https://github.com/alexandrebarachant/pyRiemann/blob/master/LICENSE">License</a>).
/// - List of Metrics inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a> (<a href="https://github.com/alexandrebarachant/pyRiemann/blob/master/LICENSE">License</a>).
/// 
///-------------------------------------------------------------------------------------------------

#pragma once
namespace OpenViBE {
// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_CovarianceMeanCalculator                            CIdentifier(0x67955ea4, 0x7c643c0f)
#define Box_CovarianceMeanCalculatorDesc                        CIdentifier(0x62e8f759, 0xd59d82a9)
#define Box_MatrixClassifierTrainer                             CIdentifier(0xc0b79b42, 0x4150c837)
#define Box_MatrixClassifierTrainerDesc                         CIdentifier(0x26f4fa93, 0x704d39dd)
#define Box_MatrixClassifierProcessor                           CIdentifier(0x918f6952, 0xb22ddf0d)
#define Box_MatrixClassifierProcessorDesc                       CIdentifier(0x8cf29eec, 0x223fbfc5)
#define Box_CovarianceMatrixToFeatureVector                     CIdentifier(0x7c265dba, 0x202c1f70)
#define Box_CovarianceMatrixToFeatureVectorDesc                 CIdentifier(0xc0fb0445, 0x0d1cd546)
#define Box_FeatureVectorToCovarianceMatrix                     CIdentifier(0x7c265dba, 0x202c1f71)
#define Box_FeatureVectorToCovarianceMatrixDesc                 CIdentifier(0xc0fb0445, 0x0d1cd541)
#define Box_CovarianceMatrixCalculator                          CIdentifier(0x9a93af80, 0x6449c826)
#define Box_CovarianceMatrixCalculatorDesc                      CIdentifier(0x12fcd91f, 0xd1d8f678)
#define Box_MatrixAffineTransformation                          CIdentifier(0x1BAA7180, 0x52CB19B8)
#define Box_MatrixAffineTransformationDesc                      CIdentifier(0x0AF511E5, 0x27137BBA)
#define Box_RiemannianPotatoField                               CIdentifier(0x3BF31561, 0xD40ED513)
#define Box_RiemannianPotatoFieldDesc                           CIdentifier(0x9F32840F, 0xB38B01A4)
#define Box_RiemannianPotatoFieldTrainer                        CIdentifier(0x8071F84E, 0x3BC49FC0)
#define Box_RiemannianPotatoFieldTrainerDesc                    CIdentifier(0x3F2DBBF3, 0xAA833423)

// Methodes/Types Lists
//---------------------------------------------------------------------------------------------------
#define TypeId_Estimator                                        CIdentifier(0x5261636B, 0x45535449)
#define TypeId_Metric                                           CIdentifier(0x5261636B, 0x4D455452)
#define TypeId_Matrix_Classifier                                CIdentifier(0x5261636B, 0x436C6173)
#define TypeId_Classifier_Adaptation                            CIdentifier(0x5261636B, 0x41646170)
//---------------------------------------------------------------------------------------------------
}  // namespace OpenViBE
