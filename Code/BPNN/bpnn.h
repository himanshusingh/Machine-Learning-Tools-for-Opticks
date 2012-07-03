/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef BPNN_H
#define BPNN_H

#include "AlgorithmShell.h"
#include "ProgressTracker.h"
#include <vector>
using std::vector;

class BPNN : public AlgorithmShell
{
public:
    BPNN();
    virtual ~BPNN();
    virtual bool getInputSpecification(PlugInArgList*& pInArgList);
    virtual bool getOutputSpecification(PlugInArgList*& pOutArgList);
    virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);

    ProgressTracker progress;
};

#endif
