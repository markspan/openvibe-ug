///-------------------------------------------------------------------------------------------------
/// 
/// \file NewBoxPattern.h
/// \brief Class NewBoxPattern
/// \author Thibaut Monseigne (Inria) & Jimmy Leblanc (Polymont) & Yannis Bendi-Ouis (Polymont) 
/// \version 1.0.
/// \date 12/03/2020.
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

#include "CPolyBox.hpp"

#if !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))

namespace OpenViBE { namespace Plugins
{
    namespace PyBox
    {
        class CBoxAlgorithmNewBoxPattern final : public CPolyBox
        {
        public:
            CBoxAlgorithmNewBoxPattern() { m_script = "NewScript.py";}
            _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm < IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_NewBoxPattern)
        };
        
        class CBoxAlgorithmNewBoxPatternListener final : public Toolkit::TBoxListener<IBoxListener>
        {
        public:
            bool onInputAdded(Kernel::IBox& box, const size_t index) override
            {
                box.setInputType(index, OV_TypeId_StreamedMatrix);
                return true;
            }

            bool onOutputAdded(Kernel::IBox& box, const size_t index) override
            {
                box.setOutputType(index, OV_TypeId_StreamedMatrix);
                return true;
            }
            _IsDerivedFromClass_Final_(Toolkit::TBoxListener < IBoxListener >, CIdentifier::undefined())
        };
        
        class CBoxAlgorithmNewBoxPatternDesc final : virtual public IBoxAlgorithmDesc
        {
        public:

            void release() override { }

            CString getName() const override                { return "NewBoxPattern"; }
            CString getAuthorName() const override          { return "NewAuthor"; }
            CString getAuthorCompanyName() const override   { return "NewCompany"; }
            CString getShortDescription() const override    { return "Default Python Description"; }
            CString getDetailedDescription() const override { return ""; }
            CString getCategory() const override            { return "Scripting/Pybox/"; }
            CString getVersion() const override             { return "0.1"; }
            CString getStockItemName() const override       { return "gtk-convert"; }
        
    CIdentifier getCreatedClass() const override    { return OVP_ClassId_BoxAlgorithm_NewBoxPattern; }
            IPluginObject* create() override       { return new CBoxAlgorithmNewBoxPattern; }
            IBoxListener* createBoxListener() const override              { return new CBoxAlgorithmNewBoxPatternListener; }
            void releaseBoxListener(IBoxListener* pBoxListener) const override{ delete pBoxListener; }

            bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
            {
                prototype.addSetting("Clock frequency (Hz)", OV_TypeId_Integer, "64");
                // <tag> settings
                
                prototype.addFlag(Kernel::BoxFlag_CanAddInput);
                prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
                prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
                prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
                prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
                prototype.addFlag(Kernel::BoxFlag_CanModifySetting);

                prototype.addInputSupport(OV_TypeId_Signal);
                prototype.addInputSupport(OV_TypeId_Stimulations);
                prototype.addInputSupport(OV_TypeId_StreamedMatrix);

                prototype.addOutputSupport(OV_TypeId_Signal);
                prototype.addOutputSupport(OV_TypeId_Stimulations);
                prototype.addOutputSupport(OV_TypeId_StreamedMatrix);

                // <tag> input & output
                
                return true;
            }

            _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_NewBoxPatternDesc)
        };
    }
}
}

#endif // !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))
