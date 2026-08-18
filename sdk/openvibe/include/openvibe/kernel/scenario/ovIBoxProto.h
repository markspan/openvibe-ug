#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \brief This enum lists all the flags a box can be have
 * \sa IBoxProto::addFlag
 */
enum EBoxFlag
{
    BoxFlag_CanAddInput,
    BoxFlag_CanModifyInput,
    BoxFlag_CanAddOutput,
    BoxFlag_CanModifyOutput,
    BoxFlag_CanAddSetting,
    BoxFlag_CanModifySetting,
    BoxFlag_IsDeprecated,
    BoxFlag_ManualUpdate
};

/**
 * \class IBoxProto
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-07-05
 * \brief OpenViBE box prototype
 * \ingroup Group_Scenario
 * \ingroup Group_Kernel
 * \ingroup Group_Extend
 *
 * This class is used by a plugin algorithm descriptor
 * to let the OpenViBE platform know what an algorithm
 * box looks like. It declares several things, like
 * it input types, output types and settings.
 *
 * \sa IBoxAlgorithmDesc
 */
class OV_API IBoxProto : public IKernelObject
{
public:

    /**
     * \brief Adds an input to the box
     * \param name [in] : the name of the input to add
     * \param typeID [in] : the type of the input
     * \param id [in] : The input id
     * \param notify [in]: if true, activate notification callback (true by default)
     * \return true if successful
     */
    virtual bool addInput(const CString& name, const CIdentifier& typeID, const CIdentifier& id = CIdentifier::undefined(), const bool notify = true) = 0;

    /**
     * \brief Adds an output to the box
     * \param name [in] : the name of the output to add
     * \param typeID [in] : the type of the output
     * \param id [in] : The output id
     * \param notify [in]: if true, activate notification callback (true by default)
     * \return true if successful
     */
    virtual bool addOutput(const CString& name, const CIdentifier& typeID, const CIdentifier& id = CIdentifier::undefined(),
                           const bool notify                                                     = true) = 0;

    /**
     * \brief Add an setting to the box
     * \param name [in] : the name of the setting to add
     * \param typeID [in] : the type of the setting
     * \param value [in] : the default value of this
     *        setting (used to initialize the box itself)
     * \param modifiable [in] : true if modifiable setting 
     * \param id [in] : The setting id
     * \param notify [in]: if true, activate notification callback (true by default)
     * \return true if successful
     */
    virtual bool addSetting(const CString& name, const CIdentifier& typeID, const CString& value, const bool modifiable = false,
                            const CIdentifier& id                                                                       = CIdentifier::undefined(),
                            const bool notify                                                                           = true) = 0;
    /**
     * \brief Adds a flag to the box
     * \param flag [in] : the flag to add to the box
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool addFlag(const EBoxFlag flag) = 0;

    /**
     * \brief Adds a flag to the box
     * \param flagID [in] : the flag to add to the box
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool addFlag(const CIdentifier& flagID) = 0;

    /**
     * \brief Adds a new type supported by inputs of the box
      * \param typeID [in] : The type identifier
      * \return \e true in case of success.
      * \return \e false in case of error.
      */
    virtual bool addInputSupport(const CIdentifier& typeID) = 0;
    /**
     * \brief Adds a new type supported by outputs of the box
      * \param typeID [in] : The type identifier
      * \return \e true in case of success.
      * \return \e false in case of error.
      */
    virtual bool addOutputSupport(const CIdentifier& typeID) = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Scenario_BoxProto)
};
}  // namespace Kernel
}  // namespace OpenViBE
