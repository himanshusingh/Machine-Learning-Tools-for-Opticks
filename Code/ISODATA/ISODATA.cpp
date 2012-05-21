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
return true;
}
bool ISODATA::getOutputSpecification(PlugInArgList*& pOutArgList)
{
return true;
}
bool ISODATA::execute(PlugInArgList* pInArgList, PlugInArgList* pOutArgList)
{
return true;
}