/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include <QtGui/QComboBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFileDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSpinBox>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QTableWidget>
#include <QtGui/QCheckBox>

#include "svm.h"
#include "svmDlg.h"
#include "AppVerify.h"
#include "FileBrowser.h"

#include<string>
using std::string;
using std::vector;

svmDlg::svmDlg(QWidget* pParent) : QDialog(pParent)
{
    setModal(true);
    setWindowTitle("Support Vector Machine Classification");

    // Predict Layout begin
    QLabel* pModelFileLabel = new QLabel("Select SVM Model file", this);
    pModelFileLabel->setToolTip("Select the file model generated by SVM.");
    mpModelFile = new FileBrowser;
    mpModelFile->setBrowseCaption("Locate SVM model file");
    mpModelFile->setBrowseFileFilters("SVM model file (*.model)");

    QHBoxLayout* pPredictLayout = new QHBoxLayout;
    pPredictLayout->addWidget(pModelFileLabel);
    pPredictLayout->addWidget(mpModelFile);

    QGroupBox* pPredictGroup = new QGroupBox;
    pPredictGroup->setLayout(pPredictLayout);
    pPredictGroup->setEnabled(false);
    // Predict Layout end

    // Train Layout begin
    QLabel* pInputFileLabel = new QLabel("Select Input data file:", this);
    pInputFileLabel->setToolTip("Select the input file generated using classificationData plugin."); 
    mpInputFile = new FileBrowser;
    mpInputFile->setBrowseCaption("Locate supervised classification data file.");
    mpInputFile->setBrowseFileFilters("Supervised Classification Data (*.scd)");

    QLabel* pOutputFileLabel = new QLabel("Select Output Model file:", this);
    pOutputFileLabel->setToolTip("Select the file on which the model learned by SVM will be written.");
    mpOuputModelFile = new FileBrowser;
    mpOuputModelFile->setBrowseCaption("SVM model file.");
    mpOuputModelFile->setBrowseFileFilters("SVM model file (*.model)");
    mpOuputModelFile->setBrowseExistingFile(false);

    QLabel* pKernelTypeLabel = new QLabel("Select Kernel", this);
    pKernelTypeLabel->setToolTip("Select the Kernel from the list which will be used to train SVM.");
    mpKernelType = new QComboBox(this);
    mpKernelType->setToolTip(pKernelTypeLabel->toolTip());
    mpKernelType->addItem("Linear");
    mpKernelType->addItem("RBF");

    QLabel* pCLabel = new QLabel("C", this);
    pCLabel->setToolTip("C is the regularisation perameter.");
    mpC = new QDoubleSpinBox(this);
    mpC->setToolTip(pCLabel->toolTip());
    mpC->setValue(0.1);
    mpC->setDecimals(4);
    mpC->setMinimum(0.0);
    mpC->setMaximum(std::numeric_limits<double>::max());

    QLabel* pEpsilonLabel = new QLabel("Epsilon", this);
    mpEpsilon = new QDoubleSpinBox(this);
    mpEpsilon->setDecimals(6);
    mpEpsilon->setValue(0.001);
    mpEpsilon->setMinimum(0.0);
    mpEpsilon->setMaximum(std::numeric_limits<double>::max());

    QLabel* pToleranceLabel = new QLabel("Tolerance", this);
    pToleranceLabel->setToolTip("Tolerance value for the SMO algorithm.");
    mpTolerance = new QDoubleSpinBox(this);
    mpTolerance->setToolTip(pToleranceLabel->toolTip());
    mpTolerance->setDecimals(6);
    mpTolerance->setValue(0.001);
    mpTolerance->setMinimum(0.0);
    mpTolerance->setMaximum(std::numeric_limits<double>::max());

    QLabel* pSigmaLabel = new QLabel("Sigma(RBF only)", this);
    pSigmaLabel->setToolTip("This is used in the RBF kernel function.");
    mpSigma = new QDoubleSpinBox(this);
    mpSigma->setToolTip(pSigmaLabel->toolTip());
    mpSigma->setValue(1.0);
    mpSigma->setDecimals(4);
    mpSigma->setMinimum(0.0);
    mpSigma->setMaximum(std::numeric_limits<double>::max());

	QLabel* pCrossValidateAndTestLabel = new QLabel("Cross validate and Test:", this);
	pCrossValidateAndTestLabel->setToolTip("If checked then cross validation and test errors are computed using input data.");
	mpCrossValidateAndTest = new QCheckBox(this);
	mpCrossValidateAndTest->setChecked(true);
	mpCrossValidateAndTest->setToolTip(pCrossValidateAndTestLabel->toolTip());

    QGridLayout* pTrainLayout = new QGridLayout;
    pTrainLayout->addWidget(pKernelTypeLabel, 0, 0);
    pTrainLayout->addWidget(mpKernelType, 0, 1);
    pTrainLayout->addWidget(pCLabel, 1, 0);
    pTrainLayout->addWidget(mpC, 1, 1);
    pTrainLayout->addWidget(pEpsilonLabel, 2, 0);
    pTrainLayout->addWidget(mpEpsilon, 2, 1);
    pTrainLayout->addWidget(pToleranceLabel, 3, 0);
    pTrainLayout->addWidget(mpTolerance, 3, 1);
    pTrainLayout->addWidget(pSigmaLabel, 4, 0);
    pTrainLayout->addWidget(mpSigma, 4, 1);
    pTrainLayout->addWidget(pInputFileLabel, 5, 0);
    pTrainLayout->addWidget(mpInputFile, 5, 1);
    pTrainLayout->addWidget(pOutputFileLabel, 6, 0);
    pTrainLayout->addWidget(mpOuputModelFile, 6, 1);
	pTrainLayout->addWidget(pCrossValidateAndTestLabel, 7, 0);
	pTrainLayout->addWidget(mpCrossValidateAndTest, 7, 1);
    pTrainLayout->setMargin(10);
    pTrainLayout->setSpacing(5);

    QGroupBox* pTrainGroup = new QGroupBox;
    pTrainGroup->setLayout(pTrainLayout);
    pTrainGroup->setEnabled(false);
    // Train Layout end

    // Button Box Begin
    QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);
    // Button Box End

    mpPredictRadio = new QRadioButton("Predict");
    mpTrainRadio = new QRadioButton("Train");

    // Overall Layout begin
    QVBoxLayout* pOverallLayout = new QVBoxLayout(this);
    pOverallLayout->setMargin(10);
    pOverallLayout->setSpacing(5);
    pOverallLayout->addWidget(mpPredictRadio);
    pOverallLayout->addWidget(pPredictGroup);
    pOverallLayout->addWidget(mpTrainRadio);
    pOverallLayout->addWidget(pTrainGroup);
    pOverallLayout->addWidget(pButtonBox);
    // Overall Layout end

    // Make GUI connections
    VERIFYNRV(connect(mpPredictRadio, SIGNAL(toggled(bool)), pPredictGroup, SLOT(setEnabled(bool))));
    VERIFYNRV(connect(mpTrainRadio, SIGNAL(toggled(bool)), pTrainGroup, SLOT(setEnabled(bool))));
    VERIFYNRV(connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
    VERIFYNRV(connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject())));

    mpPredictRadio->setChecked(true);
    mpTrainRadio->setChecked(false);
}

svmDlg::~svmDlg()
{}

bool svmDlg::getIsPredict() const
{
    return mpPredictRadio->isChecked();
}

string svmDlg::getkernelType() const
{
    return mpKernelType->currentText().toStdString();
}

double svmDlg::getC() const
{
    return mpC->value();
}

double svmDlg::getEpsilon() const
{
    return mpEpsilon->value();
}

double svmDlg::getTolerance() const
{
    return mpTolerance->value();
}

double svmDlg::getSigma() const
{
    return mpSigma->value();
}

string svmDlg::getInputFileName() const
{
    return mpInputFile->getFilename().toStdString();
}

string svmDlg::getModelFileName() const
{
    return mpModelFile->getFilename().toStdString();
}

string svmDlg::getOutputModelFileName() const
{
    return mpOuputModelFile->getFilename().toStdString();
}

bool svmDlg::getCrossValidate() const
{
	return mpCrossValidateAndTest->isChecked();
}

predictionResultDlg::predictionResultDlg(vector<string>& names, vector<string>& classes, QWidget* pParent)
{
    setWindowTitle("Prediction Results");
    QGridLayout* pBox = new QGridLayout(this);
    pBox->setMargin(10);
    pBox->setSpacing(5);

    pResultTable = new QTableWidget(names.size(), 2, this);
    pResultTable->verticalHeader()->hide();
    pResultTable->verticalHeader()->setDefaultSectionSize(20);
    QStringList horizontalHeaderLabels(QStringList() << "Signature" << "Class");
    pResultTable->setHorizontalHeaderLabels(horizontalHeaderLabels);
    //  pResultTable->horizontalHeader()->setDefaultSectionSize(100);
    for (unsigned int i = 0; i < names.size(); i++)
    {
        QTableWidgetItem *pNameItem, *pClassItem;
        pNameItem = new QTableWidgetItem(QString::fromStdString(names[i]));
        pClassItem = new QTableWidgetItem(QString::fromStdString(classes[i]));

        pResultTable->setItem(i, 0, pNameItem);
        pResultTable->setItem(i, 1, pClassItem);
    }

    pBox->addWidget(pResultTable, 0, 0);

    QFrame* pLine = new QFrame(this);
    pLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    pBox->addWidget(pLine, 1, 0, 1, 1);

    QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, 
        Qt::Horizontal, this);
    pBox->addWidget(pButtonBox, 2, 0, Qt::AlignRight);

    VERIFYNR(connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
    VERIFYNR(connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject())));
}
