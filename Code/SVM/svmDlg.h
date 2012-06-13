/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef SVMDLG_H
#define SVMDLG_H

#include <QtGui/QDialog>
#include "FileBrowser.h"

#include<vector>
#include<string>

class QComboBox;
class QLineEdit;
class QListWidget;
class QRadioButton;
class QSpinBox;
class QDoubleSpinBox;

class svmDlg : public QDialog
{
    Q_OBJECT

public:
    svmDlg(QWidget* pParent = NULL);
    virtual ~svmDlg();
    bool getIsPredict() const;
    std::string getkernelType() const;
    double getC() const;
    double getEpsilon() const;
    double getTolerance() const;
    double getSigma() const;
    std::string getModelFileName() const;
    std::string getOutputModelFileName() const;
    std::string getInputFileName() const;

private:
    // For Prediction
    QRadioButton*mpPredictRadio;
    FileBrowser* mpModelFile;

    // For training
    QRadioButton* mpTrainRadio;
    FileBrowser* mpInputFile;
    FileBrowser* mpOuputModelFile;
    QComboBox* mpKernelType;
    QDoubleSpinBox* mpC;
    QDoubleSpinBox* mpEpsilon;
    QDoubleSpinBox* mpSigma;
    QDoubleSpinBox* mpTolerance;
};

#endif