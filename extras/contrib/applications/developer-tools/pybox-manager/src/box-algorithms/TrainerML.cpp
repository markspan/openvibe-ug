#include "TrainerML.hpp"
#include <tuple>
#include <vector>

#if !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))

namespace OpenViBE {
namespace Plugins {
namespace PyBox {

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> ADA_BOOST_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("n_estimators", OV_TypeId_Integer, "50"),
    std::make_tuple("learning_rate", OV_TypeId_Float, "1.0"),
    std::make_tuple("algorithm", OVPoly_ClassId_ADA_algorithm, "SAMME.R"),
    std::make_tuple("random_state", OV_TypeId_String, "None")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> BAGGING_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("max_features", OV_TypeId_Float, "1.0"),
    std::make_tuple("random_state", OV_TypeId_String, "None"),
    std::make_tuple("n_estimators", OV_TypeId_Integer, "10"),
    std::make_tuple("bootstrap", OV_TypeId_Boolean, "true"),
    std::make_tuple("bootstrap_features", OV_TypeId_Boolean, "false"),
    std::make_tuple("oob_score", OV_TypeId_Boolean, "false"),
    std::make_tuple("warm_start", OV_TypeId_Boolean, "false"),
    std::make_tuple("n_jobs", OV_TypeId_String, "None"),
    std::make_tuple("verbose", OV_TypeId_Integer, "0")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> TREE_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("criterion", OVPoly_ClassId_Criterion, "gini"),
    std::make_tuple("splitter", OVPoly_ClassId_DecisionTree_splitter, "best"),
    std::make_tuple("max_depth", OV_TypeId_String, "None"),
    std::make_tuple("min_samples_split", OV_TypeId_Float, "2"),
    std::make_tuple("min_samples_leaf", OV_TypeId_Float, "1"),
    std::make_tuple("min_weight_fraction_leaf", OV_TypeId_Float, "0"),
    std::make_tuple("max_features", OV_TypeId_String, "None"),
    std::make_tuple("random_state", OV_TypeId_String, "None"),
    std::make_tuple("max_leaf_nodes", OV_TypeId_String, "None"),
    std::make_tuple("min_impurity_decrease", OV_TypeId_Float, "0"),
    std::make_tuple("min_impurity_split", OV_TypeId_Float, "1e-7")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> XTREE_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("criterion", OVPoly_ClassId_Criterion, "gini"),
    std::make_tuple("max_depth", OV_TypeId_String, "None"),
    std::make_tuple("min_samples_split", OV_TypeId_Float, "2"),
    std::make_tuple("min_samples_leaf", OV_TypeId_Float, "1"),
    std::make_tuple("min_weight_fraction_leaf", OV_TypeId_Float, "0"),
    std::make_tuple("max_features", OV_TypeId_String, "auto"),
    std::make_tuple("random_state", OV_TypeId_String, "None"),
    std::make_tuple("max_leaf_nodes", OV_TypeId_String, "None"),
    std::make_tuple("min_impurity_decrease", OV_TypeId_Float, "0"),
    std::make_tuple("min_impurity_split", OV_TypeId_Float, "1e-7"),
    std::make_tuple("bootstrap", OV_TypeId_Boolean, "false"),
    std::make_tuple("oob_score", OV_TypeId_Boolean, "false"),
    std::make_tuple("verbose", OV_TypeId_Integer, "0"),
    std::make_tuple("warm_start", OV_TypeId_Boolean, "false"),
    std::make_tuple("max_samples", OV_TypeId_String, "None"),
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> GAUSSIAN_SETTING = {
    std::make_tuple("Discriminator", OV_TypeId_String, ""),
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("var_smoothing", OV_TypeId_Float, "0.000000001"),
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> KNN_SETTING = {
    std::make_tuple("Discriminator", OV_TypeId_String, ""),
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("n_neighbors", OV_TypeId_Integer, "5"),
    std::make_tuple("weights", OVPoly_ClassId_Knn_Weights, "uniform"),
    std::make_tuple("algorithm", OVPoly_ClassId_Knn_Algorithm, "auto"),
    std::make_tuple("leaf_size", OV_TypeId_Integer, "30"),
    std::make_tuple("p", OV_TypeId_Integer, "2"),
    std::make_tuple("metric", OVPoly_ClassId_Metric, "minkowski"),
};


static const std::vector<std::tuple<std::string, CIdentifier, std::string>> LDA_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("solver", OVPoly_ClassId_LDA_solver, "svd"),
    std::make_tuple("n_components", OV_TypeId_String, ""),
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> LR_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("penalty", OVPoly_ClassId_Penalty, "l2"),
    std::make_tuple("dual", OV_TypeId_Boolean, "false"),
    std::make_tuple("tol", OV_TypeId_Float, "1e-4"),
    std::make_tuple("fit_intercept", OV_TypeId_Boolean, "true"),
    std::make_tuple("intercept_scaling", OV_TypeId_Float, "1"),
    std::make_tuple("random_state", OV_TypeId_String, "None"),
    std::make_tuple("solver", OVPoly_ClassId_Log_reg_solver, "lbfgs"),
    std::make_tuple("max_iter", OV_TypeId_Integer, "100"),
    std::make_tuple("verbose", OV_TypeId_Integer, "0"),
    std::make_tuple("warm_start", OV_TypeId_Boolean, "false"),
    std::make_tuple("n_jobs", OV_TypeId_String, "None"),
    std::make_tuple("multi_class", OVPoly_ClassId_Log_reg_multi_class, "auto"),
    std::make_tuple("C", OV_TypeId_Float, "1.0")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> MLP_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("hidden_layer_sizes", OV_TypeId_String, "(100,)"),
    std::make_tuple("activation", OVPoly_ClassId_MLP_activation, "relu"),
    std::make_tuple("learning_rate", OVPoly_ClassId_MLP_learning_rate, "constant"),
    std::make_tuple("solver", OVPoly_ClassId_MLP_solver, "adam"),
    std::make_tuple("alpha", OV_TypeId_Float, "0.0001"),
    std::make_tuple("batch_size", OV_TypeId_String, "auto"),
    std::make_tuple("learning_rate_init", OV_TypeId_Float, "0.001"),
    std::make_tuple("power_t", OV_TypeId_Float, "0.5"),
    std::make_tuple("max_iter", OV_TypeId_Integer, "200"),
    std::make_tuple("shuffle", OV_TypeId_Boolean, "true"),
    std::make_tuple("random_state", OV_TypeId_String, "None"),
    std::make_tuple("tol", OV_TypeId_Float, "1e-4"),
    std::make_tuple("verbose", OV_TypeId_Boolean, "true"),
    std::make_tuple("warm_start", OV_TypeId_Boolean, "false"),
    std::make_tuple("momentum", OV_TypeId_Float, "0.9"),
    std::make_tuple("nesterovs_momentum", OV_TypeId_Boolean, "true"),
    std::make_tuple("early_stopping", OV_TypeId_Boolean, "false"),
    std::make_tuple("validation_fraction", OV_TypeId_Float, "0.1"),
    std::make_tuple("beta_1", OV_TypeId_Float, "0.9"),
    std::make_tuple("beta_2", OV_TypeId_Float, "0.999"),
    std::make_tuple("epsilon", OV_TypeId_Float, "1e-8"),
    std::make_tuple("n_iter_no_change", OV_TypeId_Integer, "10")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> NC_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("metric", OVPoly_ClassId_Metric, "euclidean"),
    std::make_tuple("shrink_threshold", OV_TypeId_String, "")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> RF_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("criterion", OVPoly_ClassId_Criterion, "gini"),
    std::make_tuple("max_depth", OV_TypeId_String, "None"),
    std::make_tuple("min_samples_split", OV_TypeId_Float, "2"),
    std::make_tuple("min_samples_leaf", OV_TypeId_Float, "1"),
    std::make_tuple("min_weight_fraction_leaf", OV_TypeId_Float, "0"),
    std::make_tuple("max_features", OV_TypeId_String, "auto"),
    std::make_tuple("random_state", OV_TypeId_String, "None"),
    std::make_tuple("max_leaf_nodes", OV_TypeId_String, "None"),
    std::make_tuple("min_impurity_decrease", OV_TypeId_Float, "0"),
    std::make_tuple("min_impurity_split", OV_TypeId_Float, "1e-7"),
    std::make_tuple("bootstrap", OV_TypeId_Boolean, "false"),
    std::make_tuple("oob_score", OV_TypeId_Boolean, "false"),
    std::make_tuple("verbose", OV_TypeId_Integer, "0"),
    std::make_tuple("warm_start", OV_TypeId_Boolean, "false"),
    std::make_tuple("max_samples", OV_TypeId_String, "None"),
    std::make_tuple("n_estimators", OV_TypeId_Integer, "100")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> RTS_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("Discriminator", OVPoly_ClassId_Classifier_Algorithm, "Linear Discriminant Analysis")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> RMDM_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("Discriminator", OVPoly_ClassId_Classifier_Algorithm, "None")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> SGD_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("loss", OVPoly_ClassId_SGD_loss, "hinge"),
    std::make_tuple("penalty", OVPoly_ClassId_Penalty, "l2"),
    std::make_tuple("alpha", OV_TypeId_Float, "0.0001"),
    std::make_tuple("l1_ratio", OV_TypeId_Float, "0.15"),
    std::make_tuple("fit_intercept", OV_TypeId_Boolean, "true"),
    std::make_tuple("max_iter", OV_TypeId_Integer, "1000"),
    std::make_tuple("tol", OV_TypeId_Float, "0.001"),
    std::make_tuple("shuffle", OV_TypeId_Boolean, "true"),
    std::make_tuple("verbose", OV_TypeId_Integer, "0"),
    std::make_tuple("epsilon", OV_TypeId_Float, "0.1"),
    std::make_tuple("random_state", OV_TypeId_String, ""),
    std::make_tuple("learning_rate", OVPoly_ClassId_SGD_learning_rate, "optimal"),
    std::make_tuple("early_stopping", OV_TypeId_Boolean, "false"),
    std::make_tuple("n_iter_no_change", OV_TypeId_Integer, "5")
};

static const std::vector<std::tuple<std::string, CIdentifier, std::string>> SVM_SETTING = {
    std::make_tuple("Test set share", OV_TypeId_Float, "0.2"),
    std::make_tuple("Labels", OV_TypeId_String, ""),
    std::make_tuple("penalty", OVPoly_ClassId_Penalty, "l2"),
    std::make_tuple("loss", OVPoly_ClassId_SVM_Loss, "squared_hinge"),
    std::make_tuple("dual", OV_TypeId_Boolean, "true"),
    std::make_tuple("tol", OV_TypeId_Float, "0.0001"),
    std::make_tuple("C", OV_TypeId_Float, "1.0"),
    std::make_tuple("multi_class", OVPoly_ClassId_SVM_MultiClass, "ovr"),
    std::make_tuple("fit_intercept", OV_TypeId_Boolean, "true"),
    std::make_tuple("intercept_scaling", OV_TypeId_Float, "1"),
    std::make_tuple("verbose", OV_TypeId_Integer, "0"),
    std::make_tuple("max_iter", OV_TypeId_Integer, "1000")
};


static void ClearSetting(Kernel::IBox& box) { while (box.getSettingCount() > 4) { box.removeSetting(4); } }

static bool SetSetting(Kernel::IBox& box, const std::vector<std::tuple<std::string, CIdentifier, std::string>>& settings)
{
    for (const auto& t : settings) { box.addSetting(std::get<0>(t).c_str(), std::get<1>(t), std::get<2>(t).c_str()); }
    return true;
}

bool CBoxAlgorithmTrainerMLListener::onSettingValueChanged(Kernel::IBox& box, const size_t index)
{
    if (index == 3)
    {
        CString value;
        box.getSettingValue(index, value);
        ClearSetting(box);

        if (std::string(value.toASCIIString()) == toString(EClassifier::NearestCentroid)) { return SetSetting(box, NC_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::NearestNeighbors)) { return SetSetting(box, KNN_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::GaussianNaiveBayes)) { return SetSetting(box, GAUSSIAN_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::StochasticGradientDescent)) { return SetSetting(box, SGD_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::LogisticRegression)) { return SetSetting(box, LR_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::DecisionTree)) { return SetSetting(box, TREE_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::ExtraTrees)) { return SetSetting(box, XTREE_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::Bagging)) { return SetSetting(box, BAGGING_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::RandomForest)) { return SetSetting(box, RF_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::SVM)) { return SetSetting(box, SVM_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::LDA)) { return SetSetting(box, LDA_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::AdaBoost)) { return SetSetting(box, ADA_BOOST_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::MultiLayerPerceptron)) { return SetSetting(box, MLP_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::MDM)) { return SetSetting(box, RMDM_SETTING); }
        if (std::string(value.toASCIIString()) == toString(EClassifier::TangentSpace)) { return SetSetting(box, RTS_SETTING); }
        return true;
    }
    return false;
}

}  // namespace PyBox
}  // namespace Plugins
}  // namespace OpenViBE

#endif // !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))
