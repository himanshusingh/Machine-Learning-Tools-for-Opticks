/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef GMMEM_H
#define GMMEM_H

#include <vector>
#include "ProgressTracker.h"

#define G_PI 3.14159265358979323846

class GMM
{
public:
    double weight;
    double mean;
    double stdDev;

    GMM(double _weight, double _mean, double _stdDev) : weight(_weight), mean(_mean), stdDev(_stdDev)
    {}
    GMM()
    {}
    double probabilityFunction(double x) const;
};

typedef std::vector<double> dataPoints_t;
typedef std::vector<GMM> estimates_t;


estimates_t EM(const estimates_t& initial, const dataPoints_t& points, unsigned int maxIterations, Progress* progress = NULL);

#endif