///-------------------------------------------------------------------------------------------------
/// 
/// \file CMRMR.hpp
/// \brief mRMR (minimum Redundancy Maximum Relevance) Feature Selection Class.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 03/02/2020.
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
/// - This Algorithm is the same principle as <a href="http://home.penglab.com/proj/mRMR/">Hanchuan Peng et al.</a> (<a href="http://home.penglab.com/proj/mRMR/FAQ_mrmr.htm#Q1.8">License</a>). The difference is in the implementation in C++.
/// - Paper : Feature selection based on mutual information: criteria of max-dependency, max-relevance, and min-redundancy, <a href="https://ieeexplore.ieee.org/document/1453511">here</a> \n
/// Hanchuan Peng, Fuhui Long, and Chris Ding, IEEE Transactions on Pattern Analysis and Machine Intelligence, Vol. 27, No. 8, pp.1226-1238, 2005.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <sstream>
#include <map>
#include <limits>

enum class EMRMRMethod { MID, MIQ };

inline std::string toString(const EMRMRMethod& e)
{
    switch (e) {
        case EMRMRMethod::MIQ: return "MIQ";
        case EMRMRMethod::MID: return "MID";
        default: return "MID";
    }
}

class CMRMR
{
public:
    /// <summary> Initializes a new instance of the <see cref="CMRMR"/> class. </summary>
    CMRMR() = default;

    /// <summary> Finalizes an instance of the <see cref="CMRMR"/> class. </summary>
    ~CMRMR() { reset(); }

    /// <summary> Reset all member of this object. </summary>
    void reset();

    /// <summary> Prints some informations.</summary>
    //// <returns> the string with some informations. </returns>
    std::string print() const;

    /// <summary>Reads the CSV file (with example format in http://home.penglab.com/proj/mRMR/ website). </summary>
    /// <param name="filename">The filename.</param>
    /// <returns> True if Succes, False if Fail. </returns>
    bool readCSV(const std::string& filename);

    /// <summary> Reset previous datas and set this datas. </summary>
    /// <param name="datas">The datas.</param>
    /// <param name="classes">The classes of each sample.</param>
    /// <returns> True if success, False if fail (not same number of sample on two parameters or not same feature on each datas). </returns>
    bool setDatas(const std::vector<std::vector<double>>& datas, const std::vector<int>& classes);

    /// <summary> add this datas to actual. </summary>
    /// <param name="datas">The datas.</param>
    /// <param name="classes">The classes of each sample.</param>
    /// <returns> True if success, False if fail (not same number of sample on two parameters or not same number of feature on each datas). </returns>
    bool addDatas(const std::vector<std::vector<double>>& datas, const std::vector<int>& classes);

    /// <summary> add this sample to datas. </summary>
    /// <param name="sample">The sample.</param>
    /// <param name="classId">The classes of this sample.</param>
    /// <returns> True if success, False if fail (not same number feature than previous datas). </returns>
    bool addSample(const std::vector<double>& sample, const int classId);

    /// <summary> Gets the datas. </summary>
    /// <returns> datas (std::vector<std::vector<double>>) </returns>
    std::vector<std::vector<double>> getDatas() const { return m_datas; }

    /// <summary> Gets the discretized datas. </summary>
    /// <returns> discretized datas (std::vector<std::vector<double>>) </returns>
    std::vector<std::vector<double>> getDiscretized() const { return m_discretized; }
    /// <summary> Gets the classes list with for each classes the Ids of sample concerned. </summary>
    /// <returns> classes (std::map<int, std::vector<size_t>>) </returns>
    std::map<int, std::vector<size_t>> getClasses() const { return m_classes; }
    size_t getSampleCount() const { return m_nSamples; }
    size_t getFeaturesCount() const { return m_nFeatures; }

    /// <summary> Apply the mRMR algorithm after compute zscore and discretisation. </summary>
    /// <param name="threshold">The threshold for discretization (if infinity we only use z-score).</param>
    /// <param name="nFeatures"> The number of features to keep. Default 500 is the original theorical max of the method (not needed but a wink to the origin). </param>
    /// <param name="method"> Method Used for mRMR. </param>
    /// <returns></returns>
    std::vector<size_t> process(const double threshold   = std::numeric_limits<double>::infinity(), const size_t nFeatures = 500,
                                const EMRMRMethod method = EMRMRMethod::MID);

    /// <summary>   Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns>   Return the modified ostream. </returns>
    friend std::ostream& operator <<(std::ostream& os, const CMRMR& obj) { return (os << obj.print()); }

private:
    size_t m_nFeatures = 0;                         // Number of Features
    size_t m_nSamples  = 0;                         // Number of Samples
    std::map<int, std::vector<size_t>> m_classes;   // Datas in the format class -> vector id sample
    std::vector<std::vector<double>> m_datas;       // Datas in the format sample -> features
    std::vector<std::vector<double>> m_discretized; // Datas in the format sample -> features discretized (z-score or z-score + discretization)


    std::vector<int> class2IdxVector(size_t& n) const;
    std::vector<int> discret2IdxVector(const size_t feature, size_t& n) const;

    /// <summary> Compute the z-score. </summary>
    /// z-score is compute for each feature separatly.\n
    /// -# Compute Mean of feature \f$ \mu = \frac{1}{n}\sum_{i=1}^{n}(x_{i}) \f$ \n
    /// -# Compute The Unbiased estimation of standard deviation \f$ \sigma = \sqrt{\frac{\sum_{i=1}^{n}(x_{i} - \mu)^{2}}{n - 1}} \f$\n
    /// -# Compute The z-score \f$ z_i = \frac{x_{i} - \mu}{\sigma} \f$
    void zScore();

    /// <summary> Discretizes the datas with the specified threshold (value became in set  \f$ \{-1,0,1\} \f$). </summary>
    /// <param name="threshold">The threshold for discretization.</param>
    void discretize(const double threshold);

    /// <summary> Mutuals the information. </summary>
    /// The mutual Information is the a measure of the mutual dependence between the two features.\n
    /// -# We get the value of the feature (with z-score the value is in set \f$ \{-1,0,1\} \f$ and became in set \f$ \{0,1,2\} \f$ for the next step.\n
    /// -# We make a matrix with the probability of each value sample by sample.\n
    /// with \f$ s_i \f$ the sample \f$ i \f$, \f$ v^1_i \f$ the value of the first feature for \f$ s_i \f$ and \f$ v^2_i \f$ the value of the second feature for \f$ s_i \f$\n
    /// \f$ m \f$ is the count of the common values of each sample (\f$\text{count}\left(s_i\left(v^1_i,v^2_i\right)\right)\text{, for each } i \in n\f$ \n
    /// -# We commpute the mutal information.\n
    /// With \f$ n_1 \f$ the number of state for feature 1, \f$ n_2 \f$ the number of state for feature 2. \f$ p \f$ the probability.\n
    /// \f[ mi = \sum_{i\in n_1, j \in n_2}{p_{i,j} * \log\left(\frac{p_{i,j}}{p_{i} \times p_{j}}\right)} \f]
    /// <param name="feature1">The feature number 1 (default (-1) is for use the classification target instead of feature).</param>
    /// <param name="feature2">The feature number 2 (default (-1) is for use the classification target instead of feature).</param>
    /// <returns> the mutal information. </returns>
    double mutualInfo(const size_t feature1 = size_t(-1), const size_t feature2 = size_t(-1)) const;

    /// <summary> mRMR (minimum Redundancy Maximum Relevance Feature Selection) algorithm. </summary>
    /// We use the method describe in the <a href="https://ieeexplore.ieee.org/document/1453511">paper</a>.\n
    /// -# We Compute the relevance of each feature with the mutal info with the classification target.\n
    /// -# We sort all in descending value and take the first feature.\n
    /// -# We loop on all nexted features and compute for each the redundancy of the feature with previous selected features.\n
    /// The feature with the best score, the score is compute with two method (<see cref="EMRMRMethod"/>)\n
    /// \f[ \text{MID} = \text{relevance} - \text{redundancy}\text{, }\quad\text{MIQ} = \frac{\text{relevance}}{\text{redundancy} + 10^{-3}}\f]
    /// <param name="nFeatures"> The number of features to keep. Default 500 is the original theorical max of the method (not needed but a wink to the origin). </param>
    /// <param name="method"> Method Used for mRMR. </param>
    /// <returns> The selected indexes. </returns>
    std::vector<size_t> mRMR(const size_t nFeatures = 500, const EMRMRMethod method = EMRMRMethod::MID) const;
};
