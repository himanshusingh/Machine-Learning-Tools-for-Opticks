/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include "smo.h"
#include "svm.h"
#include <QtCore/QString>

double SMO::predict(const point& x)
{
    double p = 0;
    if (kernelType == "Linear")
    {
        for (unsigned int d = 0; d < x.size(); d++)
            p += w[d] * x[d];
        p -= threshold;
    }
    else if (kernelType == "RBF")
    {
        for (unsigned int i = 0; i < points.size(); i++)
            if (alpha[i] > 0)
                p += alpha[i]*target[i]*kernel(x, points[i]);
        p -= threshold;
    }
    return p;
}

double SMO::kernel(const point& x, const point& y)
{
    double sim = 0.0;
    if (kernelType == "Linear")
    {
        for (unsigned int d = 0; d < x.size(); d++)
            sim += x[d]*y[d];
    }
    else if (kernelType == "RBF")
    {
        for (unsigned int d = 0; d < x.size(); d++)
            sim += (x[d] - y[d])*(x[d] - y[d]);
        sim *= -1;
        sim /= (2.0*sigma*sigma);
        sim = exp(sim);
    }
    return sim;
}

void SMO::normalizeFeatures()
{
    unsigned int features = points[0].size();
    mu.resize(features);
    stdv.resize(features);
    // Calculate mean of all features
    for (unsigned int feature = 0; feature < features; feature++)
    {
        double mean = 0.0;
        for (unsigned int p = 0; p < points.size(); p++)
        {
            mean += points[p][feature];
        } 
        mean /= points.size();
        mu[feature] = mean;
    }
    // Calculate Standard Deviation of all features
    for (unsigned int feature = 0; feature < features; feature++)
    {
        double standardDeviation = 0.0;
        for (unsigned int p = 0; p < points.size(); p++)
        {
            standardDeviation += (points[p][feature] - mu[feature])*(points[p][feature] - mu[feature]);
        }
        standardDeviation /= points.size();
        standardDeviation = sqrt(standardDeviation);
        stdv[feature] = standardDeviation;
    }
    // To avoid division by 0
    for (unsigned int feature = 0; feature < features; feature++)
        if (stdv[feature] == 0)
            stdv[feature] = 1;
    // Normalize features
    for (unsigned int feature = 0; feature < features; feature++)
    {
        for (unsigned int p = 0; p < points.size(); p++)
        {
            points[p][feature] = (points[p][feature] - mu[feature])/stdv[feature];
        }
        for (unsigned int p = 0; p < testSet.size(); p++)
        {
            testSet[p][feature] = (testSet[p][feature] - mu[feature])/stdv[feature];
        }
        for (unsigned int p = 0; p < crossValidationSet.size(); p++)
        {
            crossValidationSet[p][feature] = (crossValidationSet[p][feature] - mu[feature])/stdv[feature];
        }
    }
}

svmModel SMO::train()
{
    int passes = 0;
    int maxPasses = 25;
    int numChanged = 0;
    int examineAll = 1;
    alpha.resize(points.size(), 0);
    errorCache.resize(points.size(), 0);
    w.resize(points.size(), 0);
    threshold = 0;
    // Normalize the input points
    normalizeFeatures();
    // SMO outer loop:
    // Every iteration altranates between sweep through all points examineAll = 1 and sweep through non-boundary points examineAll = 0.
    while ((numChanged > 0 || examineAll) && (passes < maxPasses)) {
        numChanged = 0;
        if (examineAll) { 
            for (unsigned int i = 0; i < points.size(); i++)
            {
                if (plugin->isAborted() == true)
                {
                    plugin->progress.report("User Aborted", 0, ABORT, true);
                    return svmModel();
                }
                plugin->progress.report("Training SVM for class " + className, passes*100/maxPasses + (i+1)*100/maxPasses/points.size(), NORMAL);
                numChanged += examineExample (i);
            }
        }
        else { 
            for (unsigned int i = 0; i < points.size(); i++)
                if (alpha[i] != 0 && alpha[i] != C)
                {
                    if (plugin->isAborted() == true)
                    {
                        plugin->progress.report("User Aborted", 0, ABORT, true);
                        return svmModel();
                    }
                    plugin->progress.report("Training SVM for class " + className, passes*100/maxPasses + (i+1)*100/maxPasses/points.size(), NORMAL);
                    numChanged += examineExample (i);
                }
        }
        if (examineAll == 1)
            examineAll = 0;
        else if (numChanged == 0)
            examineAll = 1;
        /*       
        double s = 0.0;
        for (unsigned int i=0; i<points.size(); i++)
        s += alpha[i];
        double t = 0.;
        for (unsigned int i=0; i<points.size(); i++)
        for (unsigned int j=0; j<points.size(); j++)
        t += alpha[i]*alpha[j]*target[i]*target[j]*kernel(points[i],points[j]);
        double objFunc = (s - t/2.0); 
        plugin->progress.report(QString("The value of objective function should increase with each iteration.\n The value of objective function = %1").arg(objFunc).toStdString(), (passes*100)/maxPasses, NORMAL);
        */
        passes++;
    }
    plugin->progress.report("Finished training SVM for class " + className, 100, NORMAL, true);

    // Get the model for this class
    vector<double> m_alpha;
    vector<int> m_target;
    int numberOfsupportVectors = 0;
    vector<point> supportVectors;
    int attributes = points[0].size();
    for (unsigned int i = 0; i < alpha.size() ; i++)
    {
        if (alpha[i] > 0)
        {
            m_alpha.push_back(alpha[i]);
        }
    }
    for (unsigned int i = 0; i < alpha.size(); i++)
    {
        if (alpha[i] > 0)
        {
            numberOfsupportVectors++;
            supportVectors.push_back(points[i]);
            m_target.push_back(target[i]);
        }
    }
    svmModel model = svmModel(className, kernelType, threshold, attributes, w, sigma, numberOfsupportVectors, m_alpha, supportVectors, m_target, mu, stdv);

    // Compute the error rates.
    plugin->progress.report("Computing Error Rates using the model for class " + className, 0, NORMAL, true);
    double trainErrorRate = 0;
    double testErrorRate = 0;
    double crossValidationErrorRate = 0;

    for (unsigned int i = 0; i < points.size(); i++)
    {
        plugin->progress.report("Comuting Error Rates using the model for class " + className, (i+1)*60.0/points.size(), NORMAL, true);

        if (model.predict(points[i]) > 0 != target[i] > 0)
            trainErrorRate++;
    }
    trainErrorRate = trainErrorRate*100/points.size();

    for (unsigned int i = 0; i < testSet.size(); i++)
    {
        plugin->progress.report("Comuting Error Rates using the model for class " + className, 60 + (i+1)*20.0/testSet.size(), NORMAL, true);
        if (model.predict(testSet[i]) > 0 != yTest[i] > 0)
            testErrorRate++;
    }
    testErrorRate = testErrorRate*100/points.size();

    for (unsigned int i = 0; i < crossValidationSet.size(); i++)
    {
        plugin->progress.report("Comuting Error Rates using the model for class " + className, 80 + (i+1)*20/crossValidationSet.size(), NORMAL, true);
        if (model.predict(crossValidationSet[i]) > 0 != yCV[i] > 0)
            crossValidationErrorRate++;
    }
    crossValidationErrorRate = crossValidationErrorRate*100/crossValidationSet.size();

    plugin->progress.report(QString("%1\nTrain error = %2\nCrossValidation error = %3\nTest error = %4\n").arg(className.c_str()).arg(trainErrorRate).arg(crossValidationErrorRate).arg(testErrorRate).toStdString(), 100, WARNING, true);

    return model;
}

int SMO::examineExample(int i1)
{
    double y1, alpha1, E1, r1;
    y1 = target[i1];
    alpha1 = alpha[i1];
    // Check E1 in error cache, if not present then E1 = SVM output on point[i1] - y1.
    if (alpha1 > 0 && alpha1 < C)
        E1 = errorCache[i1];
    else 
        E1 = predict(points[i1]) - y1;

    r1 = y1 * E1;
    // Check if alpha1 violated KKT condition by more than tolerance, if it does then look for alpha2 and optimise them by calling takeStep(i1,i2)
    if ((r1 < -tolerance && alpha1 < C)
        || (r1 > tolerance && alpha1 > 0))
    { 
        unsigned int k;
        int i2;
        double dEmax = 0.0;
        i2 = -1;
        // Try i2 using second choice heuristic as described in section 2.2 by choosing an error to maximize step size.
        // The i2 is taken which maximize dE.
        for (k = 0; k < points.size(); k++)
            if (alpha[k] > 0 && alpha[k] < C)
            {
                double E2, dE;
                E2 = errorCache[k];
                dE = fabs(E1 - E2);
                if (dE > dEmax)
                {
                    dEmax = dE;
                    i2 = k;
                }
            }
            if (i2 >= 0) 
            {
                if (takeStep (i1, i2))
                    return 1;
            }
            // Loop over all non-zero and non-C alpha, starting at a random point.  
            i2 = (int)(rand() % points.size());
            for (k = 0; k < points.size(); k++)
            {
                if (alpha[i2] > 0 && alpha[i2] < C) 
                {
                    if (takeStep(i1, i2))
                        return 1;
                }
                i2 = (i2 + 1)%points.size();
            }

            // Loop over all possible i2, starting at a random point.
            i2 = (int)(rand() % points.size());
            for (k = 0; k < points.size(); k++) 
            {
                if (takeStep(i1, i2))
                    return 1;
                i2 = (i2 + 1)%points.size();
            }
    }

    return 0;
}

// Optimise the two lagrange multipliers, if successful then return 1
int SMO::takeStep(int i1, int i2)
{
    // Old values of alpha[i1] and alpha[i2]
    double alpha1, alpha2;
    // New values for alpha[i1] and alpha[i2]
    double a1, a2;
    int y1, y2, s;
    double E1, E2, L, H, k11, k22, k12, eta, Lobj, Hobj, dT;

    if (i1 == i2) return 0;

    alpha1 = alpha[i1];
    y1 = target[i1];
    // Check E1 in error cache, if not present then E1 = SVM output on point[i1] - y1.
    if (alpha1 > 0 && alpha1 < C)
        E1 = errorCache[i1];
    else 
        E1 = predict(points[i1]) - y1;

    alpha2 = alpha[i2];
    y2 = target[i2];
    if (alpha2 > 0 && alpha2 < C)
        E2 = errorCache[i2];
    else 
        E2 = predict(points[i2]) - y2;

    s = y1 * y2;
    // Compute L and H
    if (y1 == y2)
    {
        L = std::max(0.0, alpha1 + alpha2 - C);
        H = std::min(C, alpha1 + alpha2);
    }
    else
    {
        L = std::max(0.0, alpha2 - alpha1);
        H = std::min(C, alpha2 - alpha1 + C);
    }

    if (L == H)
        return 0;

    k11 = kernel(points[i1], points[i1]);
    k12 = kernel(points[i1], points[i2]);
    k22 = kernel(points[i2], points[i2]);

    eta = 2 * k12 - k11 - k22;

    if (eta < 0)
    {
        a2 = alpha2 + y2 * (E2 - E1) / eta;
        if (a2 < L) a2 = L;
        else if (a2 > H) a2 = H;
    }
    else
    {
        double c1 = eta/2;
        double c2 = y2 * (E1-E2)- eta * alpha2;
        Lobj = c1 * L * L + c2 * L;
        Hobj = c1 * H * H + c2 * H;

        if (Lobj > Hobj+epsilon) a2 = L;
        else if (Lobj < Hobj-epsilon) a2 = H;
        else a2 = alpha2;
    }

    if (fabs(a2-alpha2) < epsilon*(a2+alpha2+epsilon))
        return 0;

    a1 = alpha1 - s * (a2 - alpha2);
    if (a1 < 0)
    {
        a2 += s * a1;
        a1 = 0;
    }
    else if (a1 > C)
    {
        double t = a1-C;
        a2 += s * t;
        a1 = C;
    }

    double b1, b2, newThreshold;

    if (a1 > 0 && a1 < C)
        newThreshold = threshold + E1 + y1 * (a1 - alpha1) * k11 + y2 * (a2 - alpha2) * k12;
    else
    {
        if (a2 > 0 && a2 < C)
            newThreshold = threshold + E2 + y1 * (a1 - alpha1) * k12 + y2 * (a2 - alpha2) * k22;
        else 
        {
            b1 = threshold + E1 + y1 * (a1 - alpha1) * k11 + y2 * (a2 - alpha2) * k12;
            b2 = threshold + E2 + y1 * (a1 - alpha1) * k12 + y2 * (a2 - alpha2) * k22;
            newThreshold = (b1 + b2) / 2;
        }
    }
    // Change in threshold value
    dT = newThreshold - threshold;
    // Update threshold to reflect changes in lagrange multipliers.
    threshold = newThreshold;

    double t1 = y1 * (a1-alpha1);
    double t2 = y2 * (a2-alpha2);

    // For linear kernel update weights to reflect changes in a1 and a2.
    for (unsigned int i=0; i < points[0].size(); i++)
        w[i] += points[i1][i] * t1 + points[i2][i] * t2;
    // Update error cache using new lagrange's multipliers.
    for (unsigned int i=0; i<points.size(); i++)
        if (0 < alpha[i] && alpha[i] < C)
            errorCache[i] +=  t1 * kernel(points[i1],points[i]) + t2 * kernel(points[i2],points[i]) - dT;

    errorCache[i1] = 0.0;
    errorCache[i2] = 0.0;

    // Update alpha with a1 and a2
    alpha[i1] = a1;
    alpha[i2] = a2;

    return 1;
}
