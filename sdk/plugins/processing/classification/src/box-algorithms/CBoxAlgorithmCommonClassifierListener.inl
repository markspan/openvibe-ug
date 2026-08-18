///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCommonClassifierListener.inl
/// \brief Base Classes for the classifiers Box.
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
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>
#include <iomanip>

//#define OV_DEBUG_CLASSIFIER_LISTENER

#ifdef OV_DEBUG_CLASSIFIER_LISTENER
#define DEBUG_PRINT(x) x
#else
#define DEBUG_PRINT(x)
#endif

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CBoxAlgorithmCommonClassifierListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
	explicit CBoxAlgorithmCommonClassifierListener(const size_t customSettingBase) : m_customSettingBase(customSettingBase) { }

	///-------------------------------------------------------------------------------------------------
	bool initialize() override
	{
		//Even if everything should have been set in constructor, we still set everything in initialize (in case of)
		m_classifierClassID = CIdentifier::undefined();
		m_classifier        = nullptr;

		//CIdentifier::undefined() is already use for the native, We initialize to an unused identifier in the strategy list
		m_strategyClassID = 0x0;
		m_strategy        = nullptr;

		//This value means that we need to calculate it
		m_strategyAmountSettings = -1;
		return true;
	}

	///-------------------------------------------------------------------------------------------------
	bool uninitialize() override
	{
		if (m_classifier) {
			m_classifier->uninitialize();
			this->getAlgorithmManager().releaseAlgorithm(*m_classifier);
			m_classifier = nullptr;
		}
		if (m_strategy) {
			m_strategy->uninitialize();
			this->getAlgorithmManager().releaseAlgorithm(*m_strategy);
			m_strategy = nullptr;
		}
		return true;
	}

	///-------------------------------------------------------------------------------------------------
	bool onInputAdded(Kernel::IBox& box, const size_t index) override
	{
		//index represent the number of the class (because of rejected offset)
		const std::string name = "Class " + std::to_string(index) + " label";
		std::stringstream stim;
		stim.fill('0');
		stim << "OVTK_StimulationId_Label_" << std::setw(2) << index;
		box.addSetting(name.c_str(), OV_TypeId_Stimulation, stim.str().c_str(), 3 - 1 + getStrategySettingsCount(box) + index);

		//Rename input
		return onInputAddedOrRemoved(box);
	}

	///-------------------------------------------------------------------------------------------------
	bool onInputRemoved(Kernel::IBox& box, const size_t index) override
	{
		//First remove the removed input from settings
		box.removeSetting(3 - 1 + getStrategySettingsCount(box) + index);

		//Then rename the remains inputs in settings
		for (size_t i = 1; i < box.getInputCount(); ++i) {
			const std::string name = "Class " + std::to_string(i) + " label";
			box.setSettingName(3 - 1 + getStrategySettingsCount(box) + i, name.c_str());
		}

		//Then rename input
		return onInputAddedOrRemoved(box);
	}

	///-------------------------------------------------------------------------------------------------
	bool onInitialized(Kernel::IBox& box) override
	{
		//We need to know if the box is already initialized (can be called after a restore state)
		CString strategyName;
		box.getSettingName(getStrategyIndex() + 2, strategyName);//this one is a class label
		const std::string settingName(strategyName.toASCIIString());

		if (settingName.find("Class ") == std::string::npos)//We haven't initialized the box so let's do it
		{
			//Now added Settings for classes
			for (size_t i = 1; i < box.getInputCount(); ++i) {
				const std::string name = "Class " + std::to_string(i) + " label";
				std::stringstream stim;
				stim.fill('0');
				stim << "OVTK_StimulationId_Label_" << std::setw(2) << i;
				box.addSetting(name.c_str(), OV_TypeId_Stimulation, stim.str().c_str(), 3 - 1 + getStrategySettingsCount(box) + i);
				DEBUG_PRINT(std::cout << "Add setting (type D) " << buffer << " " << stimulation << "\n";)
			}
			return this->onAlgorithmClassifierChanged(box);
		}
		return true;
		//return this->onAlgorithmClassifierChanged(box);
	}

	///-------------------------------------------------------------------------------------------------
	bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
	{
			if (index == getClassifierIndex(box)) {
			return this->onAlgorithmClassifierChanged(box);
		}
		if (index == getStrategyIndex()) {
			return this->onStrategyChanged(box);
		}
		return true;
	}

	_IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())

protected:
	CIdentifier m_classifierClassID       = CIdentifier::undefined();
	CIdentifier m_strategyClassID         = 0x0;	// CIdentifier::undefined() is already use, We initialize to an unused identifier in the strategy list
	Kernel::IAlgorithmProxy* m_classifier = nullptr;
	Kernel::IAlgorithmProxy* m_strategy   = nullptr;
	const size_t m_customSettingBase      = 0;
	int m_strategyAmountSettings          = -1;

	///-------------------------------------------------------------------------------------------------
	bool initializedStrategy(const Kernel::IBox& box)
	{
		CString name;
		box.getSettingName(getStrategyIndex() + 1, name);
		if (name == CString(PAIRWISE_STRATEGY_ENUMERATION_NAME)) { m_strategyAmountSettings = 1; }
		else { m_strategyAmountSettings = 0; }
		return true;
	}

	///-------------------------------------------------------------------------------------------------
	int getStrategySettingsCount(const Kernel::IBox& box)
	{
		if (m_strategyAmountSettings < 0) { initializedStrategy(box); }	//The value have never been initialized
		return m_strategyAmountSettings;
	}

	///-------------------------------------------------------------------------------------------------
	static bool onInputAddedOrRemoved(Kernel::IBox& box)
	{
		box.setInputType(0, OV_TypeId_Stimulations);
		box.setInputName(0, "Stimulations");
		for (size_t i = 1; i < box.getInputCount(); ++i) {
			box.setInputName(i, ("Features for class " + std::to_string(i)).c_str());
			box.setInputType(i, OV_TypeId_FeatureVector);
		}
		return true;
	}

	///-------------------------------------------------------------------------------------------------
	//Return the index of the combo box used to select the strategy (native/ OnevsOne...)
	static size_t getStrategyIndex() { return 2; }

	///-------------------------------------------------------------------------------------------------
	//Return the index of the combo box used to select the classification algorithm
	size_t getClassifierIndex(const Kernel::IBox& box) { return getStrategySettingsCount(box) + 3 + box.getInputCount() - 1; }

	///-------------------------------------------------------------------------------------------------
	bool updateDecision(Kernel::IBox& box)
	{
		const size_t i = getStrategyIndex() + 1;
		if (m_strategyClassID == Algorithm_ClassifierOneVsOne) {
			CString classifierName = "Unknown";
			box.getSettingValue(getClassifierIndex(box), classifierName);
			const CIdentifier typeID = this->getTypeManager().getEnumerationEntryValueFromName(TypeId_OneVsOne_DecisionAlgorithms, classifierName);

			OV_ERROR_UNLESS_KRF(typeID != CIdentifier::undefined(),
								"Unable to find Pairwise Decision for the algorithm [" << m_classifierClassID.str() << "] (" << classifierName << ")",
								Kernel::ErrorType::BadConfig);

			Kernel::IParameter* param = m_strategy->getInputParameter(OneVsOneStrategy_InputParameterId_DecisionType);
			Kernel::TParameterHandler<uint64_t> ip_parameter(param);

			const CString entry = this->getTypeManager().getTypeName(typeID);
			uint64_t value      = ip_parameter;
			uint64_t idx;
			CString name;

			box.getSettingValue(i, name);

			const uint64_t oldID = this->getTypeManager().getEnumerationEntryValueFromName(typeID, name);
			//The previous strategy does not exists in the new enum, let's switch to the default value (the first)
			if (oldID == CIdentifier::undefined().id()) { idx = 0; }
			else { idx = oldID; }

			this->getTypeManager().getEnumerationEntry(typeID, idx, name, value);
			ip_parameter = value;

			box.setSettingType(i, typeID);
			box.setSettingName(i, entry);
			box.setSettingValue(i, name);
		}
		return true;
	}

	///-------------------------------------------------------------------------------------------------
	bool onStrategyChanged(Kernel::IBox& box)
	{
		CString settingValue;

		box.getSettingValue(getStrategyIndex(), settingValue);

		const CIdentifier id = this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationStrategy, settingValue);
		if (id == m_strategyClassID) { return true; }	//If we don't change the strategy we just have to return

		if (id == Algorithm_ClassifierOneVsOne) {
			//Validate the classifier/strategy combination *before* tearing down the current strategy, so that on
			//an unsupported combination we can revert the box setting to the previous strategy without any side effect
			CString classifierName;
			box.getSettingValue(getClassifierIndex(box), classifierName);
			const CIdentifier typeID = this->getTypeManager().getEnumerationEntryValueFromName(TypeId_OneVsOne_DecisionAlgorithms, classifierName);
			if (typeID == CIdentifier::undefined()) {
				CString previousStrategyName = this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_ClassificationStrategy,
																										m_strategyClassID.id());
				if (previousStrategyName.length() == 0) {
					uint64_t unusedValue;
					this->getTypeManager().getEnumerationEntry(OVTK_TypeId_ClassificationStrategy, 0, previousStrategyName, unusedValue);
				}
				box.setSettingValue(getStrategyIndex(), previousStrategyName);

				this->getLogManager() << Kernel::LogLevel_Warning << "OneVsOne pairing strategy is not supported for algorithm ["
						<< classifierName << "], reverting to [" << previousStrategyName << "]\n";
				return true;
			}
		}

		if (m_strategy) {
			m_strategy->uninitialize();
			this->getAlgorithmManager().releaseAlgorithm(*m_strategy);
			m_strategy        = nullptr;
			m_strategyClassID = CIdentifier::undefined();
		}
		if (id != CIdentifier::undefined()) {
			m_strategy = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(id));
			m_strategy->initialize();
			m_strategyClassID = id;
		}

		for (size_t i = getStrategyIndex() + getStrategySettingsCount(box); i > getStrategyIndex(); --i) {
			DEBUG_PRINT(std::cout << "Remove pairing strategy setting at idx " << i - 1 << "\n";)
			box.removeSetting(i);
		}
		m_strategyAmountSettings = 0;

		if (m_strategy) {
			box.getSettingValue(getClassifierIndex(box), settingValue);
			const size_t i = getStrategyIndex() + 1;
			if (m_strategyClassID == Algorithm_ClassifierOneVsOne) {
				const CIdentifier typeID = this->getTypeManager().getEnumerationEntryValueFromName(TypeId_OneVsOne_DecisionAlgorithms, settingValue);
				//typeID is guaranteed valid here: the classifier/strategy combination was already validated above

				//As we just switch to this strategy, we take the default value set in the strategy to initialize the value
				Kernel::IParameter* param = m_strategy->getInputParameter(OneVsOneStrategy_InputParameterId_DecisionType);
				Kernel::TParameterHandler<uint64_t> ip_param(param);
				uint64_t value = ip_param;
				settingValue   = this->getTypeManager().getEnumerationEntryNameFromValue(typeID, value);

				//The strategy has no valid value yet (e.g. just initialized), fall back to the first entry
				if (settingValue.length() == 0) {
					this->getTypeManager().getEnumerationEntry(typeID, 0, settingValue, value);
					ip_param = value;
				}

				const CString paramName = this->getTypeManager().getTypeName(typeID);

				DEBUG_PRINT(std::cout << "Adding setting (case C) " << paramName << " : '" << settingValue << "' to index " << i << "\n";)
				box.addSetting(paramName, typeID, settingValue, i);

				m_strategyAmountSettings = 1;
			}
		}

		return true;
	}

	///-------------------------------------------------------------------------------------------------
	bool onAlgorithmClassifierChanged(Kernel::IBox& box)
	{
		CString name;
		box.getSettingValue(getClassifierIndex(box), name);
		CIdentifier id = this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationAlgorithm, name);
		if (id != m_classifierClassID) {
			if (m_classifier) {
				m_classifier->uninitialize();
				this->getAlgorithmManager().releaseAlgorithm(*m_classifier);
				m_classifier        = nullptr;
				m_classifierClassID = CIdentifier::undefined();
			}
			if (id != CIdentifier::undefined()) {
				m_classifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(id));
				m_classifier->initialize();
				m_classifierClassID = id;
			}

			//Disable the graphical refresh to avoid abusive redraw (not really a problem)
			while (box.getSettingCount() >= m_customSettingBase + box.getInputCount() + getStrategySettingsCount(box)) {
				box.removeSetting(getClassifierIndex(box) + 1);
			}
		}
		else { return true; }//If we don't change the algorithm we just have to return

		if (m_classifier) {
			id       = CIdentifier::undefined();
			size_t i = getClassifierIndex(box) + 1;
			while ((id = m_classifier->getNextInputParameterIdentifier(id)) != CIdentifier::undefined()) {
				if ((id != OVTK_Algorithm_Classifier_InputParameterId_FeatureVector)
					&& (id != OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet)
					&& (id != OVTK_Algorithm_Classifier_InputParameterId_Config)
					&& (id != OVTK_Algorithm_Classifier_InputParameterId_NClasses)
					&& (id != OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter)) {
					CIdentifier typeID;
					CString paramName         = m_classifier->getInputParameterName(id);
					Kernel::IParameter* param = m_classifier->getInputParameter(id);
					Kernel::TParameterHandler<int64_t> ip_iParameter(param);
					Kernel::TParameterHandler<uint64_t> ip_uiParameter(param);
					Kernel::TParameterHandler<double> ip_dParameter(param);
					Kernel::TParameterHandler<bool> ip_bParameter(param);
					Kernel::TParameterHandler<CString*> ip_sParameter(param);
					std::string buffer;
					bool valid = true;
					switch (param->getType()) {
						case Kernel::ParameterType_Enumeration:
							buffer = this->getTypeManager().getEnumerationEntryNameFromValue(param->getSubTypeIdentifier(), ip_uiParameter).toASCIIString();
							typeID = param->getSubTypeIdentifier();
							break;

						case Kernel::ParameterType_Integer:
						case Kernel::ParameterType_UInteger:
							buffer = std::to_string(int64_t(ip_iParameter));
							typeID = OV_TypeId_Integer;
							break;

						case Kernel::ParameterType_Boolean:
							buffer = (bool(ip_bParameter)) ? "true" : "false";
							typeID = OV_TypeId_Boolean;
							break;

						case Kernel::ParameterType_Float:
							buffer = std::to_string(double(ip_dParameter));
							typeID = OV_TypeId_Float;
							break;
						case Kernel::ParameterType_String:
							buffer = static_cast<CString*>(ip_sParameter)->toASCIIString();
							typeID = OV_TypeId_String;
							break;
						default:
							std::cout << "Invalid parameter type " << param->getType() << "\n";
							valid = false;
							break;
					}

					if (valid) {
						// @FIXME argh, the -2 is a hard coding that the classifier trainer has 2 settings after the classifier setting... ouch
						DEBUG_PRINT(
							std::cout << "Adding setting (case A) " << paramName << " : " << buffer << " to slot " << box.getSettingCount() - 2 << "\n";)
						box.addSetting(paramName, typeID, buffer.c_str(), box.getSettingCount() - 2);
						i++;
					}
				}
			}
		}
		// This changes the pairwise strategy decision voting type of the box settings allowing
		// designer to list the correct choices in the combo box.
		updateDecision(box);
		return true;
	}
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
