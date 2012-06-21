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
#include <QtGui/QTableWidget>
#include "FileBrowser.h"

#include<string>
using std::string;

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
    string getkernelType() const;
    double getC() const;
    double getEpsilon() const;
    double getTolerance() const;
    double getSigma() const;
    string getModelFileName() const;
    string getOutputModelFileName() const;
    string getInputFileName() const;

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

class predictionResultDlg : public QDialog
{
    Q_OBJECT
public:
    predictionResultDlg(std::vector<string>& names, std::vector<string>& classes, QWidget* pParent = NULL);
private:
    QTableWidget* pResultTable;
};

#endif