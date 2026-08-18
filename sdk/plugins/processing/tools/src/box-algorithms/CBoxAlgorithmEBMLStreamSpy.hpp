///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmEBMLStreamSpy.hpp
/// \brief Classes for the Box EBML stream spy.
/// \author Yann Renard (Inria).
/// \version 1.0.
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

#include "../defines.hpp"

#include <toolkit/ovtk_all.h>
#include <ebml/IReader.h>
#include <ebml/IReaderHelper.h>

#include <map>
#include <stack>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace Tools {

class CBoxAlgorithmEBMLStreamSpy final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>, virtual public EBML::IReaderCallback
{
public:
    CBoxAlgorithmEBMLStreamSpy() { }
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override { m_nodes.pop(); }
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_EBMLStreamSpy)

protected:
    template <class T>
    void processBinaryBlock(const void* buffer, size_t size);

    std::stack<EBML::CIdentifier> m_nodes;
    std::map<EBML::CIdentifier, std::string> m_names;
    std::map<EBML::CIdentifier, std::string> m_types;
    size_t m_nExpandValues        = 0;
    Kernel::ELogLevel m_logLevel  = Kernel::ELogLevel::LogLevel_None;
    EBML::IReader* m_reader       = nullptr;
    EBML::IReaderHelper* m_helper = nullptr;
};

class CBoxAlgorithmEBMLStreamSpyListener final : public Toolkit::TBoxListener<IBoxListener>
{
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputName(i, ("Spied EBML stream " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_EBMLStream);
        }

        return true;
    }

public:
    bool onInputRemoved(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }
    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmEBMLStreamSpyDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "EBML stream spy"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "EBML stream tree viewer"; }
    CString getDetailedDescription() const override { return "This sample EBML stream analyzer prints the EBML tree structure to the console"; }
    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_EBMLStreamSpy; }
    IPluginObject* create() override { return new CBoxAlgorithmEBMLStreamSpy(); }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmEBMLStreamSpyListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Spied EBML stream 1", OV_TypeId_EBMLStream);
        prototype.addSetting("EBML nodes description", OV_TypeId_Filename, "${Path_Data}/plugins/tools/config-ebml-stream-spy.txt");
        prototype.addSetting("Log level to use", OV_TypeId_LogLevel, "Information");
        prototype.addSetting("Expand binary blocks", OV_TypeId_Boolean, "false");
        prototype.addSetting("Number of values in expanded blocks", OV_TypeId_Integer, "4");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_EBMLStreamSpyDesc)
};

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
