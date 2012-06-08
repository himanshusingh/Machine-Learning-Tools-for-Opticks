/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

#include "AoiElement.h"
#include "AoiLayer.h"
#include "AppVerify.h"
#include "BitMaskIterator.h"
#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataElementGroup.h"
#include "DataRequest.h"
#include "DesktopServices.h"
#include "Location.h"
#include "ModelServices.h"
#include "ObjectResource.h"
#include "PlugInArg.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "PlugInResource.h"
#include "ProgressTracker.h"
#include "RasterDataDescriptor.h"
#include "RasterUtilities.h"
#include "Signature.h"
#include "SignatureSet.h"
#include "SignatureSelector.h"
#include "SpatialDataView.h"
#include "SpatialDataWindow.h"
#include "SpectralUtilities.h"
#include "SpectralGsocVersion.h"
#include "ClassificationDataDlg.h"
#include "ClassificationData.h"
#include "FileResource.h"
#include <QtCore/QString>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>

#include <limits>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

REGISTER_PLUGIN_BASIC(SpectralClassificationData, ClassificationData);

namespace
{
    template<typename T>
    void copyReflectance(T* pixel, std::vector<double>& data)
    {
        for (std::vector<double>::size_type band = 0; band < data.size() - 1; ++band)
        {
            data[band] = pixel[band];
        }
    }
};

ClassificationData::ClassificationData()
{
    setName("ClassificationData");
    setDescription("Generate Data from Raster to Train Supervised learning algorithms");
    setDescriptorId("{B135B809-8DED-4282-A481-F9355078D1A3}");
    setCopyright(SPECTRAL_GSOC_COPYRIGHT);
    setVersion(SPECTRAL_GSOC_VERSION_NUMBER);
    setProductionStatus(SPECTRAL_GSOC_IS_PRODUCTION_RELEASE);
    setAbortSupported(true);
    setMenuLocation("[SpectralGsoc]/ClassificationData");
}
ClassificationData::~ClassificationData()
{}

bool ClassificationData::getInputSpecification(PlugInArgList*& pInArgList)
{
    VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList());
    VERIFY(pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, Executable::ProgressArgDescription()));
    VERIFY(pInArgList->addArg<RasterElement>(Executable::DataElementArg(), NULL, "Raster element from which data is taken."));
    return true;
}

bool ClassificationData::getOutputSpecification(PlugInArgList*& pOutArgList)
{
    VERIFY(pOutArgList = Service<PlugInManagerServices>()->getPlugInArgList());
    return true;
}

bool ClassificationData::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
    if (pInArgList == NULL)
    {
        return false;
    }

    // Extract input arguments.
    ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
        "Executing Change Detection", "spectral", "{C37DDD94-D558-48A2-BFA8-737860A9AB52}");

    RasterElement* pRasterElement = pInArgList->getPlugInArgValue<RasterElement>(Executable::DataElementArg());
    if (pRasterElement == NULL)
    {
        progress.report("Invalid raster element.", 0, ERRORS, true);
        return false;
    }

    RasterDataDescriptor* pDescriptor = dynamic_cast<RasterDataDescriptor*>(pRasterElement->getDataDescriptor());
    if (pDescriptor == NULL)
    {
        progress.report("Invalid raster data descriptor.", 0, ERRORS, true);
        return false;
    }

    ClassificationFileDlg ClassificationFileDlg(Service<DesktopServices>()->getMainWidget());

    if (ClassificationFileDlg.exec() != QDialog::Accepted)
    {
        progress.report("Unable to obtain input parameters.", 0, ABORT, true);
        return false;
    }


    std::string dataFileName = ClassificationFileDlg.getDataFileName();


    std::ofstream dataFile(dataFileName.c_str());

    if (dataFile.good() == false)
    {
        progress.report("Unable to open the Data File", 0, ERRORS, true);
    }


    // Generate the data file
    // Format:
    // BEGIN
    // <Number of train set N> <TAB> <Number of Attributes in a single example d>
    // <Number of classes CN>
    // CN lines each containing <class name> <TAB> <classId>
    // N lines each containing <attrib1> <TAB> <attrib2> <TAB>........ <attribd> <TAB> <classId> 
    // END
    std::vector< std::vector<double> > trainSet;
    // Number of train set
    int N = 0;
    // Number of attributes
    int attributes = pDescriptor->getBandCount();

    // The classes that are selected from the class file.
    std::set<std::string> selectedClasses;
    // Get all the AOIs for this raster element.
    std::vector<DataElement*> pAois = Service<ModelServices>()->getElements(pRasterElement, TypeConverter::toString<AoiElement>());

    // Get the class names and map them to id
    std::map<std::string, int> classToId;

    QStringList classNames;

    int classes = 0;

    // Iterate over all selected AOIs
    for (std::vector<DataElement*>::iterator it = pAois.begin(); it != pAois.end(); ++it)
    {
        std::string msg = "Select Class for " + (*it)->getName();
        QString className = QInputDialog::getItem(Service<DesktopServices>()->getMainWidget(),
            QString::fromStdString(msg), "Select existing classes or enter a new one.", classNames);
        std::string strClassName = className.toStdString();

        // If a new class is entered
        if (classToId.find(strClassName) == classToId.end())
        {
            classes++;
            classNames.push_back(className);
            classToId[strClassName] = classes;
            selectedClasses.insert(strClassName);
        }

        AoiElement* pAoi = static_cast<AoiElement*>(*it);
        // Obtain iterator over the AOI.
        BitMaskIterator iterator(pAoi->getSelectedPoints(), pRasterElement);

        int startRow = iterator.getBoundingBoxStartRow();
        int endRow = iterator.getBoundingBoxEndRow();
        int startCol = iterator.getBoundingBoxStartColumn();
        int endCol = iterator.getBoundingBoxEndColumn();

        FactoryResource<DataRequest> request;
        request->setInterleaveFormat(BIP);
        request->setRows(pDescriptor->getActiveRow(startRow),
            pDescriptor->getActiveRow(endRow));
        request->setColumns(pDescriptor->getActiveColumn(startCol),
            pDescriptor->getActiveColumn(endCol));
        DataAccessor accessor = pRasterElement->getDataAccessor(request.release());
        VERIFY(accessor.isValid());

        N += iterator.getCount();

        for (int row = startRow; row <= endRow; row++) 
        {
            if (isAborted() == true)
            {
                progress.report("User Aborted.", 0, ABORT, true);
                return false;
            }

            for (int col = startCol; col <= endCol; col++) 
            {
                //If the pixel is present in cluster
                if (iterator.getPixel(col, row)) 
                {
                    // +1 for class id
                    std::vector<double> data(attributes + 1);

                    accessor->toPixel(row, col);
                    VERIFY(accessor.isValid());

                    switchOnEncoding(pDescriptor->getDataType(), copyReflectance, accessor->getColumn(), data);

                    data[attributes] = classToId[strClassName];

                    trainSet.push_back(data);

                }
            }
        }

    }

    // Write the data to dataFile
    dataFile<<"BEGIN\n";
    dataFile<<trainSet.size()<<"\t"<<attributes<<"\n";
    dataFile<<selectedClasses.size()<<"\n";
    for (std::set<std::string>::iterator it = selectedClasses.begin(); it != selectedClasses.end(); it++)
    {
        std::string className = *it;
        int id = classToId[className];
        dataFile<<className<<"\t"<<id<<"\n";
    }

    for (unsigned int n = 0; n < trainSet.size(); n++)
    {
        for (unsigned int d = 0; d < attributes; d++)
        {
            dataFile<<trainSet[n][d]<<"\t";
        }
        // class
        dataFile<<trainSet[n][attributes]<<"\n";
    }
    dataFile<<"END\n";

    dataFile.close();

    progress.report("Classification Data Generated", 100, NORMAL);
    progress.upALevel();
    return true;
}