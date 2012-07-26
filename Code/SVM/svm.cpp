/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include "AppVerify.h"
#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DesktopServices.h"
#include "ModelServices.h"
#include "PlugInArg.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "PlugInResource.h"
#include "ProgressTracker.h"
#include "RasterDataDescriptor.h"
#include "RasterUtilities.h"
#include "Signature.h"
#include "SignatureSelector.h"
#include "SpectralUtilities.h"
#include "SpectralGsocVersion.h"
#include "svmDlg.h"
#include "svm.h"
#include "svmModel.h"
#include "smo.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
using std::string;
using std::vector;

REGISTER_PLUGIN_BASIC(SpectralSVM, SVM);

namespace
{
    // Computes error rate for points using all models.
    double computeOverallError(vector<point>& points, vector<int>& target, vector<svmModel>& models, std::map<int, string>& idToClass)
    {
        double errors = 0;
        double errorRate;
        for (unsigned int i = 0; i < points.size(); i++)
        {
            double prediction = -1;
            string className;
            for (unsigned int m = 0; m < models.size(); m++)
            {
                // Normalize before prediction
                point normPoint(points[i].size());
                for (int d = 0; d < models[m].attributes; d++)
                {
                    normPoint[d] = (points[i][d] - models[m].mu[d])/models[m].stdv[d];
                }
                // Predict
                double p = models[m].predict(normPoint);
                if (p > prediction)
                {
                    prediction = p;
                    className = models[m].className;
                }
            }
            // If no class matches
            if (prediction < 0) className = "UNKNOWN";
            if (idToClass[target[i]] != className)
                errors++;
        }
        errorRate = 100*errors/points.size();
        return errorRate;
    }
};

SVM::SVM()
{
    setName("SVM");
    setDescription("SVM Supervised Classification");
    setDescriptorId("{BB502AE1-081B-46F8-A415-2B3AB03D6CD4}");
    setCopyright(SPECTRAL_GSOC_COPYRIGHT);
    setVersion(SPECTRAL_GSOC_VERSION_NUMBER);
    setProductionStatus(SPECTRAL_GSOC_IS_PRODUCTION_RELEASE);
    setAbortSupported(true);
    setMenuLocation("[SpectralGsoc]/SVM");
}

SVM::~SVM()
{}

bool SVM::getInputSpecification(PlugInArgList*& pInArgList)
{
    pInArgList = Service<PlugInManagerServices>()->getPlugInArgList();
    VERIFY(pInArgList != NULL);
    VERIFY(pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, Executable::ProgressArgDescription()));

    if (isBatch() == true)
    {
        VERIFY(pInArgList->addArg<bool>("Is Predict", static_cast<bool>(false), "True if plugin is run for prediction."
            "False if plugin is run for training on input data."));
        VERIFY(pInArgList->addArg< vector<Signature*> >("Signatures to predict", NULL, "Signature that will be predicted using the model."));

        VERIFY(pInArgList->addArg<string>("Kernel Type", static_cast<string>("RBF"), "Kernel that will be used to train SVM."));
        VERIFY(pInArgList->addArg<string>("Model File", NULL, "Model that will be used for prediction."));
        VERIFY(pInArgList->addArg<string>("Input Data File", NULL, "Input data to train the SVM."));
        VERIFY(pInArgList->addArg<string>("Output Model File", NULL, "Model generated by training will be saved in this file."));

        VERIFY(pInArgList->addArg<double>("C regularisation perameter", static_cast<double>(0.1), "Regularisation perameter for SMO."));
        VERIFY(pInArgList->addArg<double>("Epsilon", static_cast<double>(0.001), "Epsilon value for double comparisons in SMO."));
        VERIFY(pInArgList->addArg<double>("Tolerance", static_cast<double>(0.001), "Tolerance value for SMO algorithm."));
        VERIFY(pInArgList->addArg<double>("Sigma", static_cast<double>(1.0), "Sigma value of RBG kernel function."));
    }
    return true;
}

bool SVM::getOutputSpecification(PlugInArgList*& pOutArgList)
{
    return true;
}

bool SVM::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
    if (pInArgList == NULL)
    {
        return false;
    }
    // Begin extracting input arguments.
    progress = ProgressTracker(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
        "Training SVM", "spectral", "{2A47920B-1847-4316-AE79-6E0C166258DB}");

    string kernelType;
    string inputFileName, outputModelFileName, modelFileName;
    vector<Signature*> sigToPredict;
    bool isPredict;
    double C, epsilon, tolerance, sigma;
    // If the application is executing in batch mode
    if (isBatch() == true)
    {
        VERIFY(pInArgList->getPlugInArgValue("Is Predict", isPredict) == true);
        // When the plugin is run to predict only modelFileName and the signatures are required.
        if (isPredict == true)
        {
            VERIFY(pInArgList->getPlugInArgValue("Signatures to predict", sigToPredict) == true);
            if (sigToPredict.size() == 0)
            {
                progress.report("No signature selected to predict.", 0, ERRORS, true);
                return false;
            }
            VERIFY(pInArgList->getPlugInArgValue("Model File", modelFileName) == true);
        }
        else
        {
            VERIFY(pInArgList->getPlugInArgValue("Kernel Type", kernelType) == true);
            if ((kernelType != "RBF") && (kernelType != "Linear"))
            {
                progress.report("Invalid kernel selected", 0, ERRORS, true);
                return false;
            }
            VERIFY(pInArgList->getPlugInArgValue("Input Data File", inputFileName) == true);
            VERIFY(pInArgList->getPlugInArgValue("Output Model File", outputModelFileName) == true);
            VERIFY(pInArgList->getPlugInArgValue("C regularisation perameter", C) == true);
            VERIFY(pInArgList->getPlugInArgValue("Epsilon", epsilon) == true);
            VERIFY(pInArgList->getPlugInArgValue("Tolerance", tolerance) == true);
            // Sigma is only needed for the RBF kernel
            if (kernelType == "RBF")
            {
                VERIFY(pInArgList->getPlugInArgValue("Sigma", sigma) == true);
            }
        }
    }
    else
    {
        // Show the GUI dialog
        svmDlg svmDlg(Service<DesktopServices>()->getMainWidget());
        if (svmDlg.exec() != QDialog::Accepted)
        {
            progress.report("Unable to obtain input parameters.", 0, ABORT, true);
            return false;
        }

        isPredict = svmDlg.getIsPredict();
        if (isPredict == true)
        {
            // Get the sinatures to predict
            SignatureSelector signatureSelector(progress.getCurrentProgress(), Service<DesktopServices>()->getMainWidget());
            if (signatureSelector.exec() != QDialog::Accepted)
            {
                progress.report("User Aborted.", 0, ABORT, true);
                return false;
            }
            sigToPredict = signatureSelector.getExtractedSignatures();
            // Get the model file name
            modelFileName = svmDlg.getModelFileName();
        }
        else
        {
            kernelType = svmDlg.getkernelType();
            inputFileName = svmDlg.getInputFileName();
            outputModelFileName = svmDlg.getOutputModelFileName();
            C = svmDlg.getC();
            epsilon = svmDlg.getEpsilon();
            tolerance = svmDlg.getTolerance();
            sigma = svmDlg.getSigma();
        }
    }
    // end extracting input arguments

    if (isPredict)
    {
        // Make predictions on the signatures in sigToPredict
        if (sigToPredict.size() == 0)
        {
            progress.report("No signatures selected to predict.", 0, ERRORS, true);
        }

        std::ifstream modelFile(modelFileName.c_str());
        if (modelFile.good() == false)
        {
            progress.report("Invalid model file", 0, ERRORS, true);
            return false;
        }
        // Read the models
        vector<svmModel> models = readModel(modelFile);

        vector<string> names, classes;
        for (unsigned int i = 0; i < sigToPredict.size(); i++)
        {
            DataVariant reflectanceVariant = sigToPredict[i]->getData("Reflectance");
            point toPredict;
            reflectanceVariant.getValue(toPredict);

            if (toPredict.size() != models[0].attributes)
            {
                progress.report("Model file not compitable.", 0, ERRORS, true);
            }
            // Make predictions using all classes, chose the one for which prediction is greatest.
            double prediction = -1;
            string className;
            for (unsigned int m = 0; m < models.size(); m++)
            {
                // Normalise before prediction
                point normToPredict(toPredict.size());
                for (int d = 0; d < models[m].attributes; d++)
                {
                    normToPredict[d] = (toPredict[d] - models[m].mu[d])/models[m].stdv[d];
                }
                double p = models[m].predict(normToPredict);
                if (p > prediction)
                {
                    prediction = p;
                    className = models[m].className;
                }
            }
            // If prediction < 0, then no class matches the signature.
            if (prediction < 0) 
                className = "UNKNOWN";
            names.push_back(sigToPredict[i]->getName());
            classes.push_back(className);
        }
        // Display the results
        predictionResultDlg predictionResultDlg(names, classes, Service<DesktopServices>()->getMainWidget());
        predictionResultDlg.exec();
        progress.report("Finished Prediction", 100, NORMAL, true);
    }
    else
    {
        // For training SVM on the data present in the inputFileName(generated by classificationData plugin), the following approach is used:
        // 1.) Read the data from the inputFile.
        // 2.) for each class:
        //         set it as positive, and the rest of the classes negative.(For multiclass we use OneAgainstAll strategy)
        //         Train the SVM using SMO on that class and obtain the model.
        // 3.) Save the models obtained in outputModelFile.

        // Read the data
        // We break the original points in 3 parts:
        //   -- Train Set (60%)
        //   -- Test Set (20%)
        //   -- Cross Validation Set (20%)
        vector<point> points;
        vector<int> target;
        vector<point> testSet;
        vector<int> yTest;
        vector<point> crossValidationSet;
        vector<int> yCV;
        int numberOfPoints, numberOfClasses;
        int attributes;
        std::map<int, string> idToClass;
        vector<int> classes;

        std::ifstream inputFile(inputFileName.c_str());
        if (inputFile.good() == false)
        {
            progress.report("Invalid input file", 0, ERRORS, true);
            return false;
        }

        string dummy;
        inputFile>>dummy;
        if (dummy != "BEGIN")
        {
            progress.report("Invalid input file", 0, ERRORS, true);
            return false;
        }

        inputFile>>numberOfPoints>>attributes;
        inputFile>>numberOfClasses;
        for (int c = 0; c < numberOfClasses; c++)
        {
            string className;
            int id;
            inputFile>>className>>id;
            idToClass[id] = className;
            classes.push_back(id);
        }

        // 60% train set
        for (int n = 1; n <= numberOfPoints*60/100; n++)
        {
            progress.report("Reading input data", n*100/numberOfPoints, NORMAL, true);
            point p;
            double a;
            int c;
            for (int d = 0;d < attributes; d++)
            {
                inputFile>>a;
                p.push_back(a);
            }
            points.push_back(p);
            inputFile>>c;
            target.push_back(c);
        }
        // 20% test set
        for (int n = numberOfPoints*60/100 + 1; n <= numberOfPoints*80/100; n++)
        {
            progress.report("Reading input data", n*100/numberOfPoints, NORMAL, true);
            point p;
            double a;
            int c;
            for (int d = 0;d < attributes; d++)
            {
                inputFile>>a;
                p.push_back(a);
            }
            testSet.push_back(p);
            inputFile>>c;
            yTest.push_back(c);
        }
        // 20% cross validation set
        for (int n = numberOfPoints*80/100 + 1; n <= numberOfPoints; n++)
        {
            progress.report("Reading input data", n*100/numberOfPoints, NORMAL, true);
            point p;
            double a;
            int c;
            for (int d = 0;d < attributes; d++)
            {
                inputFile>>a;
                p.push_back(a);
            }
            crossValidationSet.push_back(p);
            inputFile>>c;
            yCV.push_back(c);
        }
        // Validate the input file
        inputFile>>dummy;
        if (dummy != "END")
        {
            progress.report("Invalid input file", 0, ERRORS, true);
            return false;
        }
        // End reading data

        // Models for all classes
        vector<svmModel> models;
        // Train SVM for all classes.
        for (int c = 0; c < numberOfClasses; c++)
        {
            svmModel model;
            vector<int> newTarget = target;
            vector<int> newYTest = yTest;
            vector<int> newYCV = yCV;
            // OneAgainstAll
            for (unsigned int i = 0; i < newTarget.size(); i++)
            {
                if (newTarget[i] == classes[c])
                    newTarget[i] = 1;
                else
                    newTarget[i] = -1;
            }
            for (unsigned int i = 0; i < newYTest.size(); i++)
            {
                if (newYTest[i] == classes[c])
                    newYTest[i] = 1;
                else
                    newYTest[i] = -1;
            }
            for (unsigned int i = 0; i < newYCV.size(); i++)
            {
                if (newYCV[i] == classes[c])
                    newYCV[i] = 1;
                else
                    newYCV[i] = -1;
            }
            // Train SMO on this class and obtain the model.
            SMO smo(this, C, sigma, epsilon, tolerance, kernelType, idToClass[classes[c]], points, newTarget, 
                testSet, newYTest, crossValidationSet, newYCV);
            model = smo.train();
            if (isAborted() == true)
            {
                progress.report("User Aborted", 0, ABORT, true);
                return false;
            }
            models.push_back(model);
        }

        // Compute overall error using all models
        double errorRate;
        progress.report("Computing error terms", 0, NORMAL, true);
        errorRate = computeOverallError(points, target, models, idToClass);
        progress.report(QString("Overall Train Error = %1").arg(errorRate).toStdString(), 100, WARNING, true);
        progress.report("Computing error terms", 33, NORMAL, true);
        
        errorRate = computeOverallError(testSet, yTest, models, idToClass);
        progress.report(QString("Overall Test Error = %1").arg(errorRate).toStdString(), 100, WARNING, true);
        progress.report("Computing error terms", 66, NORMAL, true);
        
        errorRate = computeOverallError(crossValidationSet, yCV, models, idToClass);
        progress.report(QString("Overall Cross Validation Error = %1").arg(errorRate).toStdString(), 100, WARNING, true);
        progress.report("Computing error terms", 100, NORMAL, true);
       
        // Save the models
        std::ofstream outputModelFile(outputModelFileName.c_str());
        saveModel(outputModelFile, models);
        progress.report("Finished training SVM", 100, NORMAL, true);
    }
    return true;
}
