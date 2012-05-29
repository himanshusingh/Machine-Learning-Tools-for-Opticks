/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include "GmmEM.h"
#include "ProgressTracker.h"
#include <cmath>
#include <numeric>
#include <functional>
#include <algorithm>

typedef std::vector<std::vector<double> > prob_matrix_t;

double GMM::probabilityFunction(double x) const
{
    return (1 / (sqrt(2*G_PI) * stdDev)) * (exp(-0.5*pow((x - mean) / stdDev, 2)));
}

struct getProb: public std::unary_function<double, double>
{
    const GMM& g;
    getProb(const GMM& _g): g(_g) {}
    double operator()(double x) const
    {
        return g.weight * g.probabilityFunction(x);
    }
};

// Compute porbability of every point in X and also the sum of probabilities of classes
void computeProbabilities(prob_matrix_t& prob, std::vector<double>& sums, 
    const dataPoints_t& X, const estimates_t& estimate) 
{
    std::vector<double>& psums = prob.back();
    std::fill(psums.begin(), psums.end(), 0);
    prob_matrix_t::iterator probItr = prob.begin();

    for(estimates_t::const_iterator estimateItr = estimate.begin(); estimateItr != estimate.end(); ++estimateItr, ++probItr)
    {
        std::transform(X.begin(), X.end(), probItr->begin(), getProb(*estimateItr));
        std::transform(probItr->begin(), probItr->end(), psums.begin(), psums.begin(), std::plus<double>());
    }
    probItr = prob.begin();
    for(unsigned int dist = 0; dist < estimate.size(); ++dist, ++probItr)
    {
        std::transform(probItr->begin(), probItr->end(), psums.begin(), probItr->begin(), 
            std::divides<double>());
        sums[dist] = std::accumulate(probItr->begin(), probItr->end(), 0.0);
    }
}


void updateWeights(GMM& g, double sum, int numPts) 
{
    g.weight = sum / numPts;
}

void updateMeans(GMM& g, double sum, const std::vector<double>& ps, const dataPoints_t& X) 
{
    double mean = 0.0;
    for (int i = 0; i < X.size(); i++)
    {
        mean += X[i]*ps[i];
    }
    mean = mean/sum;
    g.mean = mean;
}

void updateStdDevs(GMM& g, double sum, const std::vector<double>& ps, const dataPoints_t& X,
    const GMM& prev) 
{
    double stdDev = 0.0;
    double mean = prev.mean;
    for (int i = 0; i < X.size(); i++)
    {
        stdDev += pow(X[i] - mean, 2)*ps[i];
    }
    stdDev = sqrt(stdDev/sum);

    g.stdDev = stdDev;
}

estimates_t EM(const estimates_t& initial, const dataPoints_t& X, unsigned int maxIterations, Progress *progress) 
{
    // This will be used for probabilites calculation at each step
    prob_matrix_t prob;
    // Number of classes
    const unsigned int classes = initial.size();
    // Sum of probabilites of all classes
    std::vector<double> sums(classes, 0);

    for(unsigned int i = 0; i < classes + 1; ++i)
    {
        prob.push_back(std::vector<double>());
        std::vector<double>& ps = prob.back();
        ps.resize(X.size());
    }
    // Estimates after each iteration
    estimates_t next = initial;

    for(unsigned int iteration = 1; iteration <= maxIterations; ++iteration)
    {
        if (progress != NULL)
        {
            progress->updateProgress("EM running on difference image", 100*iteration/maxIterations, NORMAL);
        }
        std::fill(sums.begin(), sums.end(), 0.0);
        computeProbabilities(prob, sums, X, next);

        estimates_t::iterator estimateItr = next.begin();
        estimates_t::const_iterator prev_estimateItr = next.begin();
        std::vector<double>::const_iterator sumItr = sums.begin();
        prob_matrix_t::const_iterator probItr = prob.begin();

        for(; estimateItr != next.end(); ++estimateItr, ++sumItr, ++probItr, ++prev_estimateItr)
        {
            updateStdDevs(*estimateItr, *sumItr, *probItr, X, *prev_estimateItr);
            updateMeans(*estimateItr, *sumItr, *probItr, X);
            updateWeights(*estimateItr, *sumItr, X.size());
        }
    }
    estimates_t final = next;
    return final;
}
