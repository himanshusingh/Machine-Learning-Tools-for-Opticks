/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include "AppAssert.h"
#include "AppVerify.h"
#include "ClassificationDataDlg.h"
#include "FileBrowser.h"

#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>

ClassificationFileDlg::ClassificationFileDlg(QWidget* pParent) : QDialog(pParent)
{
    setModal(true);
    setWindowTitle("Classification Data Generator");

    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setMargin(10);
    pLayout->setSpacing(5);

    QLabel* mpDataFileLabel = new QLabel("Select Output File(This file will contain the data that can be used to train SVM.)");
    mpDataFile = new FileBrowser(this);
    mpDataFile->setBrowseCaption("Locate Classification Data file");
    mpDataFile->setBrowseFileFilters("Supervised Classification Data (*.scd)");
    mpDataFile->setBrowseExistingFile(false);

    pLayout->addWidget(mpDataFileLabel, 0, 0); 
    pLayout->addWidget(mpDataFile, 1, 0);
    pLayout->setRowStretch(1, 10);

    QHBoxLayout* pRespLayout = new QHBoxLayout;
    pLayout->addLayout(pRespLayout, 2, 0, 1, 3);

    QPushButton* pAccept = new QPushButton("OK", this);
    pRespLayout->addStretch();
    pRespLayout->addWidget(pAccept);

    QPushButton* pReject = new QPushButton("Cancel", this);
    pRespLayout->addWidget(pReject);

    VERIFYNRV(connect(pAccept, SIGNAL(clicked()), this, SLOT(accept())));
    VERIFYNRV(connect(pReject, SIGNAL(clicked()), this, SLOT(reject())));
}

ClassificationFileDlg::~ClassificationFileDlg()
{}

std::string ClassificationFileDlg::getDataFileName() const
{
    return mpDataFile->getFilename().toStdString();
}
