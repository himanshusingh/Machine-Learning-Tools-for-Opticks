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
#include "LayerList.h"
#include "Location.h"
#include "ModelServices.h"
#include "ObjectResource.h"
#include "PlugInArg.h"
#include "PlugInArgList.h"
#include "PlugInManagerServices.h"
#include "PlugInRegistration.h"
#include "PlugInResource.h"
#include "ProgressTracker.h"
#include "PseudocolorLayer.h"
#include "RasterDataDescriptor.h"
#include "RasterUtilities.h"
#include "Signature.h"
#include "SignatureSet.h"
#include "SignatureSelector.h"
#include "SpatialDataView.h"
#include "SpectralUtilities.h"
#include "SpectralGsocVersion.h"
#include "ISODATA.h"
#include "ISODATADlg.h"

#include <QtCore/QTime>
#include <QtCore/QString>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>

#include <limits>
#include <string>
#include <vector>

REGISTER_PLUGIN_BASIC(SpectralISODATA, ISODATA);

ISODATA::ISODATA()
{
    setName("ISODATA");
    setDescription("ISODATA Spectral Clustering Algorithm");
    setDescriptorId("{0E3E9D75-57C4-4FAE-BDBD-33C79C7FCB97}");
    setCopyright(SPECTRAL_GSOC_COPYRIGHT);
    setVersion(SPECTRAL_GSOC_VERSION_NUMBER);
    setProductionStatus(SPECTRAL_GSOC_IS_PRODUCTION_RELEASE);
    setAbortSupported(true);
    setMenuLocation("[SpectralGsoc]/ISODATA");
}
ISODATA::~ISODATA()
{}


bool ISODATA::getInputSpecification(PlugInArgList*& pInArgList)
{

    VERIFY(pInArgList = Service<PlugInManagerServices>()->getPlugInArgList());
    VERIFY(pInArgList->addArg<Progress>(ProgressArg(), NULL));
    VERIFY(pInArgList->addArg<SpatialDataView>(ViewArg()));
    VERIFY(pInArgList->addArg<double>("SAMThreshold", static_cast<double>(85.0),
        "SAM Threshold. Default is 85.0."));
    VERIFY(pInArgList->addArg<double>("Maximum STDV", static_cast<double>(0.0),
        "Maximum Standard Deviation of points from their cluster centers along each axis."));
    VERIFY(pInArgList->addArg<double>("Minimum Centre Distance", static_cast<double>(0.0),
        "Minimum required distance between two cluster centers."));
    VERIFY(pInArgList->addArg<unsigned int>("Maximum Iterations", static_cast<unsigned int>(10),
        "Maximum number of iterations for which the algorithm will run."));
    VERIFY(pInArgList->addArg<unsigned int>("Initial Clusters", static_cast<unsigned int>(2),   
        "The number of intial Clusters that will be used to run the algorithm."));
    VERIFY(pInArgList->addArg<unsigned int>("Minimum Cluster Points", static_cast<unsigned int>(10),
        "Minimum number of points that can form a Cluster."));
    VERIFY(pInArgList->addArg<std::string>("Results Name", "ISODATA Results",
        "Determines the name for the results of the clustering."));

    return true;
}
bool ISODATA::getOutputSpecification(PlugInArgList*& pOutArgList)
{
    return true;
}
bool ISODATA::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
    if (pInArgList == NULL)
        return false;

    //Extract Input Arguments

    ProgressTracker progress(pInArgList->getPlugInArgValue<Progress>(ProgressArg()),
        "Executing ISODATA", "spectral", "{2925A495-54FD-4E3B-A92A-3D5891A0D277}");

    SpatialDataView* pView = pInArgList->getPlugInArgValue<SpatialDataView>(ViewArg());
    if (pView == NULL)
    {
        progress.report("Invalid view.", 0, ERRORS, true);
        return false;
    }

    LayerList* pLayerList = pView->getLayerList();
    VERIFY(pLayerList != NULL);

    RasterElement* pRasterElement = pLayerList->getPrimaryRasterElement();
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

    double SAMThreshold;
    VERIFY(pInArgList->getPlugInArgValue("SAMThreshold", SAMThreshold) == true);
    if (SAMThreshold <= 0.0)
    {
        progress.report("Invalid SAM threshold.", 0, ERRORS, true);
        return false;
    }

    unsigned int MaxIterations;
    VERIFY(pInArgList->getPlugInArgValue("Maximum Iterations", MaxIterations) == true);

    unsigned int NumClus;
    VERIFY(pInArgList->getPlugInArgValue("Initial Clusters", NumClus) == true);

    double MaxSTDV;
    VERIFY(pInArgList->getPlugInArgValue("Maximum STDV", MaxSTDV) == true);
    if (MaxSTDV < 0.0)
    {
        progress.report("Invalid Maximum STDV.", 0, ERRORS, true);
        return false;
    }
    double Lump;
    VERIFY(pInArgList->getPlugInArgValue("Minimum Centre Distance", Lump) == true);
    if (Lump < 0.0)
    {
        progress.report("Invalid Minimum Center Distance.", 0, ERRORS, true);
        return false;
    }
    unsigned int SamPrm;
    VERIFY(pInArgList->getPlugInArgValue("Minimum Cluster Points", SamPrm) == true);

    std::string resultsName;
    VERIFY(pInArgList->getPlugInArgValue("Results Name", resultsName) == true);

    // Show interavtive dialog if  the application is not running in Batch Mode.
    if (isBatch() == false)
    {
        ISODATADlg ISODATADlg(SAMThreshold, MaxIterations, NumClus,
            Lump, MaxSTDV, SamPrm, Service<DesktopServices>()->getMainWidget());

        if (ISODATADlg.exec() != QDialog::Accepted)
        {
            progress.report("Unable to obtain input parameters.", 0, ABORT, true);
            return false;
        }
        SAMThreshold = ISODATADlg.getSAMThreshold();
        MaxIterations = ISODATADlg.getMaxIterations();
        NumClus = ISODATADlg.getNumClus();
        Lump = ISODATADlg.getLump();
        MaxSTDV = ISODATADlg.getMaxSTDV();
        SamPrm = ISODATADlg.getSamPrm();
    }

    return true;
}