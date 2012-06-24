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
#include "ChangeDetectionEMDlg.h"

#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>

using namespace std;

ChangeDetectionEMDlg::ChangeDetectionEMDlg(const vector<string>& rasters, QWidget* pParent) : QDialog(pParent),
    mpComboOrig(NULL), mpComboChange(NULL)
{
    setModal(true);
    setWindowTitle("Change Detection");

    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setMargin(10);
    pLayout->setSpacing(5);

    QLabel* pOrigLabel = new QLabel("Select Original Image: ", this);
    pLayout->addWidget(pOrigLabel, 0, 0);

    mpComboOrig = new QComboBox(this);
    pLayout->addWidget(mpComboOrig, 0, 1, 1, 2);
    pLayout->setColumnStretch(1, 10);

    for (vector<string>::const_iterator it = rasters.begin(); mpComboOrig != NULL && it != rasters.end(); ++it)
    {
        mpComboOrig->addItem(it->c_str());
    }
    mpComboOrig->setEditable(false);

    QLabel* pChangedLabel = new QLabel("Select Changed Image: ", this);
    pLayout->addWidget(pChangedLabel, 1, 0);

    mpComboChange = new QComboBox(this);
    pLayout->addWidget(mpComboChange, 1, 1, 1, 2);
    pLayout->setColumnStretch(1, 10);

    for (vector<string>::const_iterator it = rasters.begin(); mpComboChange != NULL && it != rasters.end(); ++it)
    {
        mpComboChange->addItem(it->c_str());
    }
    mpComboChange->setEditable(false);

    pLayout->setRowStretch(2, 10);

    QHBoxLayout* pRespLayout = new QHBoxLayout;
    pLayout->addLayout(pRespLayout, 3, 0, 1, 3);

    QPushButton* pAccept = new QPushButton("OK", this);
    pRespLayout->addStretch();
    pRespLayout->addWidget(pAccept);

    QPushButton* pReject = new QPushButton("Cancel", this);
    pRespLayout->addWidget(pReject);

    VERIFYNRV(connect(pAccept, SIGNAL(clicked()), this, SLOT(accept())));
    VERIFYNRV(connect(pReject, SIGNAL(clicked()), this, SLOT(reject())));
}

ChangeDetectionEMDlg::~ChangeDetectionEMDlg()
{}

vector<string> ChangeDetectionEMDlg::getSelectedRasters() const
{
    vector<string> rasters;
    VERIFYRV(mpComboOrig != NULL, rasters);
    VERIFYRV(mpComboChange != NULL, rasters);

    QString str = mpComboOrig->currentText();
    if (str.isEmpty() == false)
    {
        rasters.push_back(str.toStdString());
    }
    str = mpComboChange->currentText();
    if (str.isEmpty() == false)
    {
        rasters.push_back(str.toStdString());
    }
    return rasters;
}
