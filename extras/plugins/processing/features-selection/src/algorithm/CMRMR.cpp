#include "CMRMR.hpp"

#include <cstdlib>
#include <cmath>
#include <map>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>

///-------------------- Public Functions --------------------
///-------------------------------------------------------------------------------------------------
void CMRMR::reset()
{
    m_nFeatures = 0;
    m_nSamples  = 0;
    //if (!m_classes.empty()) { for (auto& c : m_classes) { c.second.clear(); } }   // useless
    //if (!m_datas.empty()) { for (auto& d : m_datas) { d.clear(); } }              // useless
    //if (!m_discretized.empty()) { for (auto& d : m_discretized) { d.clear(); } }  // useless
    m_classes.clear();
    m_datas.clear();
    m_discretized.clear();
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::string CMRMR::print() const
{
    std::stringstream ss;
    ss << "Datas contain " << m_nFeatures << " Features, with " << m_nSamples << " samples, for " << m_classes.size() << " Classes (";
    size_t i = 0;
    for (auto& it : m_classes)
    {
        if (i != 0) { ss << ", "; }
        ss << "Class " << i++ << " : " << "id(" << it.first << "), " << it.second.size() << " samples";
    }
    ss << ")." << std::endl;
    ss << "Datas are " << (m_discretized.empty() ? "not " : "") << "z-scored and discretize." << std::endl;
    return ss.str();
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMRMR::readCSV(const std::string& filename)
{
    reset();
    std::ifstream file;
    file.open(filename);
    if (!file.is_open())
    {
        std::cerr << "File cannot be opened." << std::endl;
        return false;
    }

    std::string header;
    if (!getline(file, header))
    {
        std::cerr << "Header can not be read." << std::endl;
        return false;
    }

    m_nFeatures = std::count(header.begin(), header.end(), ',');
    m_nSamples  = 0;

    std::string line;
    while (getline(file, line))
    {
        if (std::count(line.begin(), line.end(), ',') != m_nFeatures)
        {
            std::cerr << "not same number of features : " << std::count(line.begin(), line.end(), ',') << "expected : " << m_nFeatures << std::endl;
            return false;
        }
        std::vector<double> sample;
        sample.reserve(m_nFeatures);
        int classId;
        double value;
        char sep;
        std::stringstream ss(line);
        ss >> classId;
        while (ss >> sep >> value) { sample.push_back(value); }
        if (sample.size() != m_nFeatures)
        {
            std::cerr << "not found good number of features : " << sample.size() << "expected : " << m_nFeatures << std::endl;
            return false;
        }
        m_datas.push_back(std::move(sample));

        // Update class list
        auto it = m_classes.find(classId);
        if (it == m_classes.end())
        {
            m_classes.emplace(classId, std::vector<size_t>());
            it = m_classes.find(classId);
        }
        it->second.push_back(m_nSamples++);
    }

    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMRMR::setDatas(const std::vector<std::vector<double>>& datas, const std::vector<int>& classes)
{
    reset();
    return addDatas(datas, classes);
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMRMR::addDatas(const std::vector<std::vector<double>>& datas, const std::vector<int>& classes)
{
    const size_t n = datas.size();
    if (n != classes.size())
    {
        std::cerr << "not same number of sample between datas and classes : " << n << " VS " << classes.size() << std::endl;
        return false;
    }
    for (size_t i = 0; i < n; ++i) { if (!addSample(datas[i], classes[i])) { return false; } }
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMRMR::addSample(const std::vector<double>& sample, const int classId)
{
    if (m_nSamples == 0) { m_nFeatures = sample.size(); }
    else if (m_nFeatures != sample.size())
    {
        std::cerr << "not same number of features between sample and previous samples : " << sample.size() << " VS " << m_nFeatures << std::endl;
        return false;
    }

    // Update datas
    m_datas.push_back(sample);

    // Update class list
    auto it = m_classes.find(classId);
    if (it == m_classes.end())
    {
        m_classes.emplace(classId, std::vector<size_t>());
        it = m_classes.find(classId);
    }
    it->second.push_back(m_nSamples++);
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::vector<size_t> CMRMR::process(const double threshold, const size_t nFeatures, const EMRMRMethod method)
{
    if (m_nSamples == 0 || m_nFeatures == 0) { return std::vector<size_t>(); }
    m_discretized.reserve(m_nSamples);
    for (auto&& v : m_datas) { m_discretized.emplace_back(std::begin(v), std::end(v)); }

    if (threshold != std::numeric_limits<double>::infinity())
    {
        m_discretized.resize(m_nSamples);
        for (auto& d : m_discretized) { d.resize(m_nFeatures); }
        zScore();                       // Compute zScore
        discretize(threshold);          // Compute discretization
    }
    return mRMR(nFeatures, method); // Apply mRMR Algorithm
}
///-------------------------------------------------------------------------------------------------

///-------------------- Private Functions --------------------
///-------------------------------------------------------------------------------------------------
std::vector<int> CMRMR::class2IdxVector(size_t& n) const
{
    std::vector<int> res;
    res.resize(m_nSamples);             // Features from all samples
    int min = std::numeric_limits<int>::max();
    int max = std::numeric_limits<int>::min();

    for (const auto& m : m_classes)
    {
        const int tmp = m.first;
        if (min > tmp) { min = tmp; }
        if (max < tmp) { max = tmp; }
        for (const auto& sampleIdx : m.second) { res[sampleIdx] = tmp; }
    }
    for (auto& v : res) { v -= min; }   // transform to 0 to n Indexes
    n = size_t(max - min + 1);
    return res;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::vector<int> CMRMR::discret2IdxVector(const size_t feature, size_t& n) const
{
    std::vector<int> res;
    res.reserve(m_nSamples);                // Features from all samples
    int min = m_discretized[0][feature];
    int max = m_discretized[0][feature];
    for (const auto& d : m_discretized)
    {
        const int tmp = int(round(d[feature]));
        res.push_back(tmp);
        if (min > tmp) { min = tmp; }
        if (max < tmp) { max = tmp; }
    }
    for (auto& id : res) { id -= min; } // transform to 0 to n Indexes
    n = size_t(max - min + 1);
    return res;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CMRMR::zScore()
{
    // We z-score by feature (so column by column)
    for (size_t j = 0; j < m_nFeatures; ++j)
    {
        double sum = 0.0;
        for (const auto& d : m_datas) { sum += d[j]; }
        const double mean = sum / double(m_nSamples);
        sum               = 0.0;
        for (const auto& d : m_datas)
        {
            const double tmp = d[j] - mean;
            sum += tmp * tmp;
        }
        const double std = (m_nSamples == 1) ? 0 : sqrt(sum / double(m_nSamples - 1));  //m_nSamples - 1 is an unbiased version for Gaussian
        for (size_t i = 0; i < m_nSamples; ++i) { m_discretized[i][j] = (m_datas[i][j] - mean) / std; }
    }
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CMRMR::discretize(const double threshold)
{
    for (auto& sample : m_discretized)
    {
        for (auto& feature : sample)
        {
            if (feature > threshold) { feature = 1; }
            else if (feature < -threshold) { feature = -1; }
            else { feature = 0; }
        }
    }
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
double CMRMR::mutualInfo(const size_t feature1, const size_t feature2) const
{
    if ((feature1 != size_t(-1) && feature1 >= m_nFeatures) || (feature2 != size_t(-1) && feature2 >= m_nFeatures)) { return -1; }

    // Copy Datas in int vector of size n_sample
    size_t nstate1, nstate2;
    std::vector<int> v1 = (feature1 != size_t(-1)) ? discret2IdxVector(feature1, nstate1) : class2IdxVector(nstate1);
    std::vector<int> v2 = (feature2 != size_t(-1)) ? discret2IdxVector(feature2, nstate2) : class2IdxVector(nstate2);

    // Joint Probabilities
    std::vector<std::vector<double>> jointProba(nstate1, std::vector<double>(nstate2, 0.0));
    for (size_t i = 0; i < m_nSamples; ++i) { jointProba[v1[i]][v2[i]]++; }
    for (auto& row : jointProba) { for (auto& cell : row) { cell /= m_nSamples; } }

    // Mutual Information
    std::vector<double> proba1(nstate1, 0.0), proba2(nstate2, 0.0);
    for (size_t i = 0; i < nstate1; ++i)
    {
        for (size_t j = 0; j < nstate2; ++j)
        {
            proba1[i] += jointProba[i][j];
            proba2[j] += jointProba[i][j];
        }
    }

    double res = 0.0;
    for (size_t i = 0; i < nstate1; ++i)
    {
        for (size_t j = 0; j < nstate2; ++j)
        {
            if (jointProba[i][j] != 0 && proba1[i] != 0 && proba2[j] != 0) { res += jointProba[i][j] * log(jointProba[i][j] / proba1[i] / proba2[j]); }
        }
    }
    res /= log(2);
    return res;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::vector<size_t> CMRMR::mRMR(const size_t nFeatures, const EMRMRMethod method) const
{
    if (nFeatures == 0) { return std::vector<size_t>(); }
    const size_t n = ((nFeatures < m_nFeatures) ? nFeatures : m_nFeatures);
    std::vector<size_t> res(n);

    // Initialize selection
    std::vector<double> mutualInfos;
    std::vector<bool> mask(m_nFeatures, true);
    mutualInfos.reserve(m_nFeatures);
    for (size_t i = 0; i < m_nFeatures; ++i) { mutualInfos.push_back(mutualInfo(size_t(-1), i)); }  // Compute Mutual infos with classId
    //const double entropy = mutualInfo(size_t(-1), size_t(-1));    // the entropy of target classification variable

    // Sort in Descending Order
    std::vector<size_t> indexes(m_nFeatures);
    iota(indexes.begin(), indexes.end(), 0);
    stable_sort(indexes.begin(), indexes.end(), [&mutualInfos](const size_t i1, const size_t i2) { return mutualInfos[i1] > mutualInfos[i2]; });
    //stable_sort(mutualInfos.begin(), mutualInfos.end(), greater<double>()); // Useless


    //mRMR selection
    res[0]           = indexes[0];  // We have the first Feature
    mask[indexes[0]] = false;       // After selection, no longer consider this feature
    for (size_t i = 1; i < n; ++i)  //the first one, res[0] has been determined already
    {
        double score = std::numeric_limits<double>::min();
        for (const auto& id : indexes)
        {
            if (!mask[id]) { continue; }        // We skeep this Id 
            const double relevance = mutualInfos[id];
            double redundancy      = 0;
            for (size_t j = 0; j < i; ++j) { redundancy += mutualInfo(res[j], id); }
            redundancy /= double(i);

            // If more methods, a switch is preferable
            const double tmp = (method == EMRMRMethod::MID) ? relevance - redundancy : relevance / (redundancy + 0.0001);
            if (score < tmp)                    //update the best feature found and the score
            {
                score  = tmp;
                res[i] = id;
            }
        }
        // Remove from the id list the final selection with the mask,  we can remove index from indexes vector instead of use mask 
        // (a test can be make to find wich method of mask and vector modification is faster)
        mask[res[i]] = false;
    }
    return res;
}
///-------------------------------------------------------------------------------------------------
