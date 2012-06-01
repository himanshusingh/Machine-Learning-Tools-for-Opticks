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
#include "GmmEM.h"
#include "ChangeDetectionEMDlg.h"

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

    template<typename T>
    void updadeMaxAndMin(T* pData, double& maxXd, double& minXd)
    {
        minXd = std::min(minXd, static_cast<double>(*pData));
        maxXd = std::max(maxXd, static_cast<double>(*pData));
    }
    template<typename T>
    void getValue(T* pData, double& p)
    {
        p = static_cast<double>(*pData);
    }

    template<typename T>
    void setValue(T* pData, double& v)
    {
        *pData = static_cast<T>(v);
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
    VERIFY(pInArgList->addArg<RasterElement>("Original Image", NULL, "Raster element of the original image."));
    VERIFY(pInArgList->addArg<RasterElement>("Changed Image", NULL, "This changed image that will be used to detect changes."));
    return true;
}

bool ChangeDetectionEM::getOutputSpecification(PlugInArgList*& pOutArgList)
{
    VERIFY(pOutArgList = Service<PlugInManagerServices>()->getPlugInArgList());
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

    RasterElement* pRasterElementOrig =  NULL;
    VERIFY(pInArgList->getPlugInArgValue("Original Image", pRasterElementOrig) == true);
    RasterElement* pRasterElementChanged = NULL;
    VERIFY(pInArgList->getPlugInArgValue("Changed Image", pRasterElementChanged) == true);

    // If not running in batch mode then show interactive dialog to select images
    if (isBatch() == false)
    {
        std::vector<DataElement*> de;
        de = Service<ModelServices>()->getElements(TypeConverter::toString<RasterElement>());
        std::vector<RasterElement*> rasters;
        for (std::vector<DataElement*>::const_iterator it = de.begin(); it != de.end(); it++)
        {
            rasters.push_back(static_cast<RasterElement*>(*it));
        }
        // Get names of raster elements
        std::vector<std::string> rasterNames;
        for (std::vector<RasterElement*>::const_iterator it = rasters.begin(); it != rasters.end(); it++)
        {
            rasterNames.push_back((*it)->getName());
        }
        // Show the GUI dialog
        ChangeDetectionEMDlg ChangeDetectionEMDlg(rasterNames, Service<DesktopServices>()->getMainWidget());

        if (ChangeDetectionEMDlg.exec() != QDialog::Accepted)
        {
            progress.report("Unable to obtain input parameters.", 0, ABORT, true);
            return false;
        }

        std::vector<std::string> selected = ChangeDetectionEMDlg.getSelectedRasters();
        int i = 0;
        for (std::vector<RasterElement*>::const_iterator it = rasters.begin(); it != rasters.end(); it++)
        {
            if (selected[0] == (*it)->getName())
                pRasterElementOrig = (*it);
        }
        for (std::vector<RasterElement*>::const_iterator it = rasters.begin(); it != rasters.end(); it++)
        {
            if (selected[1] == (*it)->getName())
                pRasterElementChanged = (*it);
        }

    }

    if (pRasterElementOrig == NULL)
    {
        progress.report("Invalid raster element for original image.", 0, ERRORS, true);
        return false;
    }

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
            arg(pDescriptorChanged->getRowCount()).arg(pDescriptorChanged->getColumnCount()).arg(pDescriptorChanged->getBandCount()).toStdString(), 0, WARNING, true);
        progress.report("All dimensions of images must be equal", 0, ERRORS, true);
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

    // Max and Min value in the difference image
    // the initial Threshold will be T0 = (maxXd - minXd)/2
    double minXd = std::numeric_limits<double>::max();
    double maxXd = -minXd;

    // Obtain the difference image using CVA technique
    unsigned int rowCount = pDescriptorOrig->getRowCount();
    unsigned int colCount = pDescriptorOrig->getColumnCount();

    for (unsigned int row = 0; row < rowCount; row++)
    {
        progress.report("Prforming Change Vector Analysis to obtain difference image", 100*row/rowCount, NORMAL, true);
        for (int col = 0; col < colCount; col++)
        {
            switchOnEncoding(pDescriptorOrig->getDataType(), CVA, pAccDiff->getColumn(), pAccOrig->getColumn(),pAccChanged->getColumn()
                , pDescriptorOrig->getBandCount());

            switchOnEncoding(pDescriptorOrig->getDataType(), updadeMaxAndMin, pAccDiff->getColumn(), maxXd, minXd);

            pAccOrig->nextColumn();
            pAccChanged->nextColumn();
            pAccDiff->nextColumn();
        }
        pAccOrig->nextRow();
        pAccChanged->nextRow();
        pAccDiff->nextRow();
    }


    // Initial Estimates obtained by setting threshold = (maxXd - minXd)/2
    double initialThreshold = (maxXd - minXd)/2;
    double changeCount = 0.0, notChageCount = 0.0;
    double changeWeight = 0.0, notChangeWeight = 0.0;
    double changeMean = 0.0, notChangeMean = 0.0;
    double changeStdDev = 0.0, notChangeStdDev = 0.0;
    pAccDiff->toPixel(0,0);
    for (int row = 0; row < rowCount; row++)
    {
        progress.report("Obtaining initial estimates", 50*row/rowCount, NORMAL, true);
        for (int col = 0; col < colCount; col++)
        {
            double p;
            switchOnEncoding(pDescriptorOrig->getDataType(), getValue, pAccDiff->getColumn(), p);
            if (p > initialThreshold)
            {
                changeCount++;
                changeMean += p;
            }
            else
            {
                notChageCount++;
                notChangeMean += p;
            }
            pAccDiff->nextColumn();
        }
        pAccDiff->nextRow();
    }

    changeMean = changeMean/changeCount;
    notChangeMean = notChangeMean/notChageCount;

    // The pixel values of difference image are stored in X and passed to EM algorithm
    dataPoints_t X;

    pAccDiff->toPixel(0,0);
    for (int row = 0; row < rowCount; row++)
    {
        progress.report("Obtaining initial estimates", 51 + 50*row/rowCount, NORMAL, true);
        for (int col = 0; col < colCount; col++)
        {
            double p;
            switchOnEncoding(pDescriptorOrig->getDataType(), getValue, pAccDiff->getColumn(), p);
            X.push_back(p);
            if (p > initialThreshold)
            {
                changeStdDev += (p-changeMean)*(p-changeMean);
            }
            else
            {
                notChangeStdDev += (p - notChangeMean)*(p - notChangeMean);
            }
            pAccDiff->nextColumn();
        }
        pAccDiff->nextRow();
    }

    changeStdDev = sqrt(changeStdDev/changeCount);
    notChangeStdDev = sqrt(notChangeStdDev/notChageCount);
    changeWeight = double(changeCount)/(rowCount*colCount);
    notChangeWeight = double(notChageCount)/(rowCount*colCount);


    estimates_t final;
    // Changed
    GMM wc;
    // Unchanged
    GMM wn;
    // Run the EM only when the model conforms to GMM.
    if (notChangeStdDev != 0)
    {
        // The initial estimates for EM
        estimates_t initial;
        initial.push_back(GMM(changeWeight, changeMean, changeStdDev));
        initial.push_back(GMM(notChangeWeight, notChangeMean, notChangeStdDev));

        //Run EM algorithm on intial estimates
        final = EM(initial, X, 20, progress.getCurrentProgress());

        progress.report(QString("Changed class weight = %1, mean = %2, stddev = %3").arg(final[0].weight).arg(final[0].mean).arg(final[0].stdDev).toStdString(), 0, WARNING, true);

        progress.report(QString("Unchanged class weight = %1, mean = %2, stddev = %3").arg(final[1].weight).arg(final[1].mean).arg(final[1].stdDev).toStdString(), 0, WARNING, true);

        // Changed
        wc = final[0];
        // Unchanged
        wn = final[1];
    }


    // White if changed
    double cv = 0.0;
    // Black if not changed
    double nv = 255.0;

    pAccDiff->toPixel(0,0);
    for (int row = 0; row < rowCount; row++)
    {
        progress.report("Generating the difference image", 100*(row+1)/rowCount, NORMAL, true);
        for (int col = 0; col < colCount; col++)
        {
            double p;
            switchOnEncoding(pDescriptorOrig->getDataType(), getValue, pAccDiff->getColumn(), p);
            // Check the class to which the pixel [row, column] belong
            if (notChangeStdDev == 0)
            {
                if (p == notChangeMean)
                {
                    switchOnEncoding(pDescriptorOrig->getDataType(), setValue, pAccDiff->getColumn(), nv);
                }
                else
                {
                    switchOnEncoding(pDescriptorOrig->getDataType(), setValue, pAccDiff->getColumn(), cv);
                }
            }
            else
            {
                if (wc.weight*wc.probabilityFunction(p) > wn.weight*wn.probabilityFunction(p))
                {
                    switchOnEncoding(pDescriptorOrig->getDataType(), setValue, pAccDiff->getColumn(), cv);
                }
                else
                {
                    switchOnEncoding(pDescriptorOrig->getDataType(), setValue, pAccDiff->getColumn(), nv);
                }
            }
            pAccDiff->nextColumn();
        }
        pAccDiff->nextRow();
    }

    // Show results
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