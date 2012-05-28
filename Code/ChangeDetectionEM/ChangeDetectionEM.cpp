/*
* The information in this file is
* Copyright(c) 2012, Himanshu Singh <91.himanshu@gmail.com>
* and is subject to the terms and conditions of the
* GNU Lesser General Public License Version 2.1
* The license text is available from   
* http://www.gnu.org/licenses/lgpl.html
*/

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
#include "ChangeDetectionEM.h"

#include <QtCore/QString>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>

#include <limits>
#include <string>
#include <vector>

REGISTER_PLUGIN_BASIC(SpectralChangeDetectionEM, ChangeDetectionEM);

namespace
{
    template<typename T>
    void CVA(T* diff, void* _orig, void* _changed, unsigned int bands)
    {
        T* orig = reinterpret_cast<T*>(_orig);
        T* changed = reinterpret_cast<T*>(_changed);
        double result = 0.0;
        *diff = static_cast<T>(0.0);
        for (unsigned int band = 0; band < bands; ++band)
        {
            result += (changed[band] - orig[band])*(changed[band] - orig[band]);
        }
        result = sqrt(result);
        *diff = static_cast<T>(result);

    }
};

ChangeDetectionEM::ChangeDetectionEM()
{
    setName("ChangeDetectionEM");
    setDescription("Change Detection using an EM based approach");
    setDescriptorId("{20BA5F46-B7C1-40E6-B6D2-DAA5DFB7C4A5}");
    setCopyright(SPECTRAL_GSOC_COPYRIGHT);
    setVersion(SPECTRAL_GSOC_VERSION_NUMBER);
    setProductionStatus(SPECTRAL_GSOC_IS_PRODUCTION_RELEASE);
    setAbortSupported(true);
    setMenuLocation("[SpectralGsoc]/ChangeDetectionEM");
}
ChangeDetectionEM::~ChangeDetectionEM()
{}

bool ChangeDetectionEM::getInputSpecification(PlugInArgList*& pInArgList)
{
    VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList());
    VERIFY(pInArgList->addArg<Progress>(Executable::ProgressArg(), NULL, Executable::ProgressArgDescription()));
    VERIFY(pInArgList->addArg<RasterElement>(Executable::DataElementArg(), NULL, "Raster element of the original image."));
    VERIFY(pInArgList->addArg<RasterElement>("Changed Image", NULL, "This changed image that will be used to detect changes."));
    return true;
}

bool ChangeDetectionEM::getOutputSpecification(PlugInArgList*& pOutArgList)
{
    VERIFY(pOutArgList = Service<PlugInManagerServices>()->getPlugInArgList());
    VERIFY(pOutArgList->addArg<DataElementGroup>("Change Detection Result", NULL,
        "Data element group containing all results from the Change Detection."));
    VERIFY(pOutArgList->addArg<RasterElement>("Change Detection Results Element", NULL,
        "This raster element will display the changed areas."));
    return true;
}

bool ChangeDetectionEM::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
    if (pInArgList == NULL)
    {
        return false;
    }

    // Extract input arguments.
    ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
        "Executing Change Detection", "spectral", "{C37DDD94-D558-48A2-BFA8-737860A9AB52}");

    RasterElement* pRasterElementOrig =  pInArgList->getPlugInArgValue<RasterElement>(Executable::DataElementArg());
    if (pRasterElementOrig == NULL)
    {
        progress.report("Invalid raster element for original image.", 0, ERRORS, true);
        return false;
    }
    RasterElement* pRasterElementChanged = pInArgList->getPlugInArgValue<RasterElement>("Changed Image");
    if (pRasterElementChanged == NULL)
    {
        progress.report("Invalid raster element for changed image.", 0, ERRORS, true);
        return false;
    }

    RasterDataDescriptor* pDescriptorOrig = dynamic_cast<RasterDataDescriptor*>(pRasterElementOrig->getDataDescriptor());
    RasterDataDescriptor* pDescriptorChanged = dynamic_cast<RasterDataDescriptor*>(pRasterElementChanged->getDataDescriptor());
    if (pDescriptorOrig == NULL || pDescriptorChanged == NULL)
    {
        progress.report("Invalid raster data descriptor.", 0, ERRORS, true);
        return false;
    }



    if ((pDescriptorOrig->getRowCount() != pDescriptorChanged->getRowCount())
        || (pDescriptorOrig->getColumnCount() != pDescriptorChanged->getColumnCount())
        || (pDescriptorOrig->getBandCount() != pDescriptorChanged->getBandCount()))
    {
        progress.report(QString("Dimensions [%1][%2][%3] and [%4][%5][%6]").arg(pDescriptorOrig->getRowCount()).arg(pDescriptorOrig->getColumnCount()).arg(pDescriptorOrig->getBandCount()).
            arg(pDescriptorChanged->getRowCount()).arg(pDescriptorChanged->getColumnCount()).arg(pDescriptorChanged->getBandCount()).toStdString(), 0, ERRORS, true);
        //progress.report("All dimensions of images must be equal", 0, ERRORS, true);
        return false;
    }

    // Create the RasterElement for Difference image (One band only)
    ModelResource<RasterElement> pRasterElementDiff(dynamic_cast<RasterElement*>(Service<ModelServices>()->getElement(
        "Change Detection Results", TypeConverter::toString<RasterElement>(), NULL)));
    pRasterElementDiff = ModelResource<RasterElement>(reinterpret_cast<RasterElement*>(NULL));
    pRasterElementDiff = ModelResource<RasterElement>(RasterUtilities::createRasterElement("Change Detection Results", pDescriptorOrig->getRowCount(),
        pDescriptorOrig->getColumnCount(),pDescriptorOrig->getDataType()));
    if (pRasterElementDiff.get() == NULL)
    {
        progress.report("Raster Element for Difference image could not be created.", 0, ERRORS, true);
        return false;
    }

    // Obtain accessors
    FactoryResource<DataRequest> requestOrig;
    requestOrig->setInterleaveFormat(BIP);
    DataAccessor pAccOrig = pRasterElementOrig->getDataAccessor(requestOrig.release());
    VERIFY(pAccOrig.isValid());

    FactoryResource<DataRequest> requestChanged;
    requestChanged->setInterleaveFormat(BIP);
    DataAccessor pAccChanged = pRasterElementChanged->getDataAccessor(requestChanged.release());
    VERIFY(pAccChanged.isValid());

    FactoryResource<DataRequest> requestDiff;
    requestDiff->setWritable(true);
    DataAccessor pAccDiff = pRasterElementDiff->getDataAccessor(requestDiff.release());
    VERIFY(pAccDiff.isValid());

    // Obtain the difference image using CVA technique
    unsigned int rowCount = pDescriptorOrig->getRowCount();
    for (unsigned int row = 0; row < rowCount; row++)
    {
        progress.report("Prforming Change Vector Analysis to obtain difference image", 100*row/rowCount, NORMAL, true);
        for (int col = 0; col < pDescriptorOrig->getColumnCount(); col++)
        {
            switchOnEncoding(pDescriptorOrig->getDataType(), CVA, pAccDiff->getColumn(), pAccOrig->getColumn(),pAccChanged->getColumn()
                , pDescriptorOrig->getBandCount());
            pAccOrig->nextColumn();
            pAccChanged->nextColumn();
            pAccDiff->nextColumn();
        }
        pAccOrig->nextRow();
        pAccChanged->nextRow();
        pAccDiff->nextRow();
    }

    if (!isBatch())
    {
        Service<DesktopServices> pDesktop;

        SpatialDataWindow* pWindow = static_cast<SpatialDataWindow*>(pDesktop->createWindow(pRasterElementDiff->getName(),
            SPATIAL_DATA_WINDOW));

        SpatialDataView* pView = (pWindow == NULL) ? NULL : pWindow->getSpatialDataView();
        if (pView == NULL)
        {
            progress.report("Unable to create view", 0, ERRORS, true);
            return false;
        }

        pView->setPrimaryRasterElement(pRasterElementDiff.get());
        pView->createLayer(RASTER, pRasterElementDiff.get());
    }
    pOutArgList->setPlugInArgValue("Change Detection Results Element", dynamic_cast<RasterElement*>(pRasterElementDiff.release()));
    progress.report("Change Detection complete", 100, NORMAL);
    progress.upALevel();
    return true;
}