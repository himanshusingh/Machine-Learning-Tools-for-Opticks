/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef CLASSIFICATIONDATA_H
#define CLASSIFICATIONDATA_H

#include <QtGui/QDialog>
#include "FileBrowser.h"

#include<vector>
#include<string>

class QComboBox;

class ClassificationFileDlg : public QDialog
{
    Q_OBJECT

public:
    ClassificationFileDlg(QWidget* pParent);
    ~ClassificationFileDlg();
    std::string getDataFileName() const;

private:
    FileBrowser* mpDataFile;

};

#endif