/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef BPNNDLG_H
#define BPNNDLG_H

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

class bpnnDlg : public QDialog
{
    Q_OBJECT

public:
    bpnnDlg(QWidget* pParent = NULL);
    virtual ~bpnnDlg();
    bool getIsPredict() const;
    double getLearningRate() const;
    double getMomentum() const;
    int getIterations() const;
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
    QDoubleSpinBox* mpLearningRate;
    QDoubleSpinBox* mpMomentum;
    QSpinBox* mpIterations;
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