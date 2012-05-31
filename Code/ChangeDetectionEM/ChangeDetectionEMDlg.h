/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef CHANGEDETECTIONDLG_H
#define CHANGEDETECTIONDLG_H

#include <QtGui/QDialog>

#include <string>
#include <vector>

class QCheckBox;
class QComboBox;

class ChangeDetectionEMDlg : public QDialog
{
    Q_OBJECT

public:
    ChangeDetectionEMDlg(const std::vector<std::string>& rasters, QWidget* pParent);
    virtual ~ChangeDetectionEMDlg();

    std::vector<std::string> getSelectedRasters() const;
private:
    QComboBox* mpComboOrig;
    QComboBox* mpComboChange;
};

#endif