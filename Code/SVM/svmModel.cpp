/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include "svmModel.h"
#include "svm.h"
#include <fstream>
#include <vector>
#include <string>
using std::string;
using std::vector;

// Read saved models from modelFile and return them.
vector<svmModel> readModel(std::ifstream& modelFile)
{
    vector<svmModel> models;
    string className;
    string kernelType;
    double threshold;
    int attributes;
    // For linear kernel
    vector<double> w;
    //For RBF kernel
    double sigma = 1.0;
    int numberOfSupportVectors = 0;
    vector<double> alpha;
    vector<point> supportVector;
    vector<int> target;
    vector<double> mu, stdv;
    while (modelFile.good())
    {
        modelFile>>className;
        modelFile>>kernelType;
        modelFile>>threshold;
        modelFile>>attributes;

        mu.resize(attributes);
        for (int d = 0; d < attributes; d++)
        {
            modelFile>>mu[d];
        }
        stdv.resize(attributes);
        for (int d = 0; d < attributes; d++)
        {
            modelFile>>stdv[d];
        }

        if (kernelType == "Linear")
        {
            w.resize(attributes);
            for (int d = 0; d < attributes; d++)
            {
                modelFile>>w[d];
            }
        }
        else if (kernelType == "RBF")
        {
            modelFile>>sigma;
            modelFile>>numberOfSupportVectors;
            alpha.resize(numberOfSupportVectors);
            for (int i = 0; i < numberOfSupportVectors; i++)
                modelFile>>alpha[i];

            supportVector.resize(numberOfSupportVectors);
            target.resize(numberOfSupportVectors);
            for (int i = 0; i < numberOfSupportVectors; i++)
            {
                point x(attributes);
                for (int d = 0; d < attributes; d++)
                    modelFile>>x[d];
                supportVector[i] = x;
                modelFile>>target[i];
            }
        }

        models.push_back(svmModel(className, kernelType, threshold, attributes, w, sigma, numberOfSupportVectors, alpha, supportVector, target, mu, stdv));
    }
    return models;
}

// Save the models in outputModelFile
bool saveModel(std::ofstream& outputModelFile, vector<svmModel>& models)
{
    for (unsigned int m = 0; m < models.size(); m++)
    {
        outputModelFile<<models[m].className<<"\n";
        outputModelFile<<models[m].kernelType<<"\n";
        outputModelFile<<models[m].threshold<<"\n";
        outputModelFile<<models[m].attributes<<"\n";
        int d;
        for (d = 0; d < models[m].attributes - 1; d++)
            outputModelFile<<models[m].mu[d]<<" ";
        outputModelFile<<models[m].mu[d]<<"\n";

        for (d = 0; d < models[m].attributes - 1; d++)
            outputModelFile<<models[m].stdv[d]<<" ";
        outputModelFile<<models[m].stdv[d]<<"\n";

        if (models[m].kernelType == "Linear")
        {

            for (d = 0; d < models[m].attributes - 1; d++)
                outputModelFile<<models[m].w[d]<<" ";
            outputModelFile<<models[m].w[d]<<"\n";
        }
        else if (models[m].kernelType == "RBF")
        {
            int i;
            outputModelFile<<models[m].sigma<<"\n";
            outputModelFile<<models[m].numberOfSupportVectors<<"\n";
            for (i = 0; i < models[m].numberOfSupportVectors - 1; i++)
                outputModelFile<<models[m].alpha[i]<<" ";
            outputModelFile<<models[m].alpha[i]<<"\n";

            for (i = 0; i < models[m].numberOfSupportVectors; i++)
            {
                for (d = 0; d < models[m].attributes; d++)
                    outputModelFile<<models[m].supportVector[i][d]<<" ";
                outputModelFile<<models[m].target[i]<<"\n";
            }
        }
    }
    return true;
}

double svmModel::kernel(const point& x, const point& y)
{
    double sim = 0.0;
    if (kernelType == "Linear")
    {
        for (int d = 0; d < attributes; d++)
            sim += x[d]*y[d];
    }
    else if (kernelType == "RBF")
    {
        for (int d = 0; d < attributes; d++)
            sim += (x[d] - y[d])*(x[d] - y[d]);
        sim *= -1;
        sim /= (2*sigma*sigma);
        sim = exp(sim);
    }
    return sim;
}

// Make predictions for x using the model.
double svmModel::predict(const point& x)
{
    double p = 0;
    if (kernelType == "Linear")
    {
        for (int d = 0; d < attributes; d++)
            p += w[d] * x[d];
        p -= threshold;
    }
    else if (kernelType == "RBF")
    {
        for (int i = 0; i < numberOfSupportVectors; i++)
            p += alpha[i]*target[i]*kernel(x, supportVector[i]);
        p -= threshold;
    }
    return p;
}
