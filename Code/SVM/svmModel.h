/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef SVMMODEL_H
#define SVMMODEL_H

#include "svm.h"
#include <fstream>
#include <vector>
#include <string>
using std::vector;
using std::string;

struct svmModel
{
    svmModel()
    {}
    svmModel(string _className, string _kernelType, double _threshold, int _attributes, vector<double>& _w, double _sigma, int N,
        vector<double>& _alpha, vector<point>& _supportV, vector<int>& _target) : className(_className), kernelType(_kernelType), 
        threshold(_threshold), w(_w), sigma(_sigma), numberOfSupportVectors(N), attributes(_attributes), alpha(_alpha), 
        supportVector(_supportV), target(_target)
    {}
    // Predict for x using this model
    double predict(const point&);
    double kernel(const point&, const point&);

    string className;
    string kernelType;
    double threshold;
    int attributes;
    // For linear kernel
    vector<double> w;
    // For RBF kernel
    double sigma;
    int numberOfSupportVectors;
    vector<double> alpha;
    vector<point> supportVector;
    vector<int> target;
};

vector<svmModel> readModel(std::ifstream& modelFile);

bool saveModel(std::ofstream& outputModelFile, vector<svmModel>& models);

#endif