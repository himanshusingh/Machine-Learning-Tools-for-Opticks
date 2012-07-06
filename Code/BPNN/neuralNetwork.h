/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include "Progress.h"
#include "bpnn.h"
#include <vector>
#include <string>
#include <map>
using std::vector;
using std::string;

#define HIGH 0.9f
#define LOW 0.1f
// Three Layers BackPropagation Neural Network
class NeuralNetwork
{
    BPNN* plugin;
    int iterations;
    // Number of units in each layer
    int inputUnits;
    int hiddenUnits;
    int outputUnits;
    // Activations of each layer
    vector<double> inputActiv;
    vector<double> hiddenActiv;
    vector<double> outputActiv;
    vector<double> target;
    // Classes present in the data
    vector<string> classNames;
    std::map<int, string> idToClass;

    vector< vector<double> > trainSet;
    vector<int> yTrain;
    vector< vector<double> > testSet;
    vector<int> yTest;

    // Weight matrix
    vector< vector<double> > inputWeight;
    vector< vector<double> > hiddenWeight;
    // Change in weight matrix
    vector< vector<double> > inputWeightDelta;
    vector< vector<double> > hiddenWeightDelta;
    // Error terms
    vector<double> hiddenDelta;
    vector<double> outputDelta;
    // mean and standard deviation
    vector<double> mu, stdv;
    // training parameters
    double learningRate;
    double momentum;

    double sigmoid(const double x);
    double dsigmoid(const double x);

    void normalizeFeatures();
    void initialize();
    void feedForward();
    double backPropagate();
    void computeAccuracy();

public:
    NeuralNetwork(BPNN* _plugin) : plugin(_plugin)
    {}
    NeuralNetwork(BPNN* _plugin, double _learningRate, double _momentum, int _iterations) :
      plugin(_plugin),
      learningRate(_learningRate),
      momentum(_momentum),
      iterations(_iterations)
      {}

    bool train();
    bool readData(const string& inputFileName);
    bool saveModel(const string& outputModelFileName);
    bool readModel(const string& modelFileName);
    std::string predict(vector<double>& toPredict);
};
#endif

