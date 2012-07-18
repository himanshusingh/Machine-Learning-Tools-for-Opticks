/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef ISODATA_H
#define ISODATA_H

#include "AlgorithmShell.h"
#include <string>
#include <vector>
class ProgressTracker;
class PseudocolorLayer;
class ExecutableResource;
class SignatureSet;

class ISODATA : public AlgorithmShell
{
public:
    ISODATA();
    virtual ~ISODATA();
    virtual bool getInputSpecification(PlugInArgList*& pInArgList);
    virtual bool getOutputSpecification(PlugInArgList*& pOutArgList);
    virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
private:
	bool getInputArguments(PlugInArgList* pInArgList);
	PseudocolorLayer* runSamOnCentroids(ExecutableResource& pSam, SignatureSet* target, int iterationNumber, RasterElement* pRasterElement);
	void performLumping();

	std::vector <Signature*> centroids;
	ProgressTracker progress;
	SpatialDataView* pView;
	std::string resultsName;
	double SAMThreshold;
	unsigned int MaxIterations;
	unsigned int NumClus;
	unsigned int MaxPair;
	double MaxSTDV;
	double Lump;
	int SamPrm;
	unsigned int clusters;
};

#endif
