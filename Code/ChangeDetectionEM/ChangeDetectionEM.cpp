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
    VERIFY(pInArgList->addArg<RasterElement>("Change Image", NULL, "This changed image that will be used to detect changes."));
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
    return true;
}