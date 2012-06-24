/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include <QtCore/QString>
#include <QtGui/QCheckBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QSpinBox>

#include "AppVerify.h"
#include "ISODATADlg.h"

#include <limits>
ISODATADlg::ISODATADlg(double SAMThreshold, unsigned int MaxIterations, unsigned int NumClus, double Lump, 
    double MaxSTDV, int SamPrm, unsigned int MaxPair, QWidget* pParent) : QDialog(pParent)
{
    setModal(true);
    setWindowTitle("ISODATA");

    QLabel* pSAMThresholdLabel = new QLabel("SAM Threshold", this);
    pSAMThresholdLabel->setToolTip("This threshold will be used for each run of the SAM algorithm.");
    mpSAMThreshold = new QDoubleSpinBox(this);
    mpSAMThreshold->setValue(SAMThreshold);
    mpSAMThreshold->setDecimals(5);
    mpSAMThreshold->setMinimum(0.0);
    mpSAMThreshold->setMaximum(180.0);
    mpSAMThreshold->setToolTip(pSAMThresholdLabel->toolTip());

    QLabel* pMaxIterationsLabel = new QLabel("Maximum Iterations", this);
    pMaxIterationsLabel->setToolTip("Maximumn number of iterations for which the algorithm will run.");
    mpMaxIterations = new QSpinBox(this);
    mpMaxIterations->setValue(MaxIterations);
    mpMaxIterations->setMinimum(0);
    mpMaxIterations->setMaximum(std::numeric_limits<int>::max());
    mpMaxIterations->setToolTip(pMaxIterationsLabel->toolTip());

    QLabel* pNumClusLabel = new QLabel("Inital Number of Clusters", this);
    pNumClusLabel->setToolTip("The number of intial Clusters that will be used to run the algorithm.");
    mpNumClus = new QSpinBox(this);
    mpNumClus->setValue(NumClus);
    mpNumClus->setMinimum(2);
    mpNumClus->setMaximum(std::numeric_limits<int>::max());
    mpNumClus->setToolTip(pNumClusLabel->toolTip());

    QLabel* pLumpLabel = new QLabel("Minimum Center distance", this);
    pLumpLabel->setToolTip("Minimum required distance between two cluster centers.");
    mpLump = new QDoubleSpinBox(this);
    mpLump->setValue(Lump);
    mpLump->setDecimals(5);
    mpLump->setMinimum(0.0);
    mpLump->setMaximum(std::numeric_limits<double>::max());
    mpLump->setToolTip(pLumpLabel->toolTip());

    QLabel* pMaxSTDVLabel = new QLabel("Maximum Standard Deviation", this);
    pMaxSTDVLabel->setToolTip("Maximum Standard Deviation of points from their cluster centers along each axis.");
    mpMaxSTDV = new QDoubleSpinBox(this);
    mpMaxSTDV->setValue(MaxSTDV);
    mpMaxSTDV->setDecimals(5);
    mpMaxSTDV->setMinimum(0.0);
    mpMaxSTDV->setMaximum(std::numeric_limits<double>::max());
    mpMaxSTDV->setToolTip(pMaxSTDVLabel->toolTip());

    QLabel* pSamPrmLabel = new QLabel("Minimum Points in a Cluster", this);
    pSamPrmLabel->setToolTip("Minimum number of points that can form a Cluster");
    mpSamPrm = new QSpinBox(this);
    mpSamPrm->setValue(SamPrm);
    mpSamPrm->setMinimum(0);
    mpSamPrm->setMaximum(std::numeric_limits<int>::max());
    mpSamPrm->setToolTip(pSamPrmLabel->toolTip());

    QLabel* pMaxPairLabel = new QLabel("Maximum Merge pairs", this);
    pMaxPairLabel->setToolTip("Maximum number of cluster pairs that can be merged per iteration.");
    mpMaxPair = new QSpinBox(this);
    mpMaxPair->setValue(MaxPair);
    mpMaxPair->setMinimum(0);
    mpMaxPair->setMaximum(std::numeric_limits<int>::max());
    mpMaxPair->setToolTip(pMaxPairLabel->toolTip());

    QFrame* pLine = new QFrame(this);
    pLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    QDialogButtonBox* pButtonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    // Layout Begin
    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->addWidget(pSAMThresholdLabel, 0, 0);
    pLayout->addWidget(mpSAMThreshold, 0, 1);
    pLayout->addWidget(pMaxIterationsLabel, 1, 0);
    pLayout->addWidget(mpMaxIterations, 1, 1);
    pLayout->addWidget(pNumClusLabel, 2, 0);
    pLayout->addWidget(mpNumClus, 2, 1);
    pLayout->addWidget(pLumpLabel, 3, 0);
    pLayout->addWidget(mpLump, 3, 1);
    pLayout->addWidget(pMaxSTDVLabel, 4, 0);
    pLayout->addWidget(mpMaxSTDV, 4, 1);
    pLayout->addWidget(pSamPrmLabel, 5, 0);
    pLayout->addWidget(mpSamPrm, 5, 1);
    pLayout->addWidget(pMaxPairLabel, 6, 0);
    pLayout->addWidget(mpMaxPair, 6, 1);
    pLayout->addWidget(pLine, 7, 0, 1, 2);
    pLayout->addWidget(pButtonBox, 8, 0, 1, 2);
    pLayout->setRowStretch(8, 10);
    pLayout->setColumnStretch(2, 10);
    pLayout->setMargin(10);
    pLayout->setSpacing(5);
    setLayout(pLayout);
    // Layout End

    // Make GUI connections
    VERIFYNRV(connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
    VERIFYNRV(connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject())));
}

ISODATADlg::~ISODATADlg()
{}

double ISODATADlg::getSAMThreshold() const
{
    return mpSAMThreshold->value();
}

unsigned int ISODATADlg::getMaxIterations() const
{
    return mpMaxIterations->value();
}

unsigned int ISODATADlg::getNumClus() const
{
    return mpNumClus->value();
}

double ISODATADlg::getLump() const
{
    return mpLump->value();
}

double ISODATADlg::getMaxSTDV() const
{
    return mpMaxSTDV->value();
}

int ISODATADlg::getSamPrm() const
{
    return mpSamPrm->value();
}

unsigned int ISODATADlg::getMaxPair() const
{
    return mpMaxPair->value();
}
