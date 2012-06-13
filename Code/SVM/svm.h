/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef SVM_H
#define SVM_H

#include "AlgorithmShell.h"

class SVM : public AlgorithmShell
{
public:
    SVM();
    virtual ~SVM();
    virtual bool getInputSpecification(PlugInArgList*& pInArgList);
    virtual bool getOutputSpecification(PlugInArgList*& pOutArgList);
    virtual bool execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList);
};

#endif