using System;
using System.Text.RegularExpressions;
using System.Diagnostics;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Debugger.Interop;

namespace Microsoft.ImageWatch
{
    class ImageWatchVisualizerService :
        IVsCppDebugUIVisualizer, IImageWatchVisualizerService
    {
        public int DisplayValue(uint ownerHwnd, uint visualizerId, IDebugProperty3 debugProperty)
        {
            int hr = VSConstants.S_OK;

            DEBUG_PROPERTY_INFO[] propertyInfo = new DEBUG_PROPERTY_INFO[1];
            hr = debugProperty.GetPropertyInfo(
                enum_DEBUGPROP_INFO_FLAGS.DEBUGPROP_INFO_ALL,
                10 /* Radix */,
                10000 /* Eval Timeout */,
                new IDebugReference2[] { },
                0,
                propertyInfo);

            Debug.Assert(hr == VSConstants.S_OK, "IDebugProperty3.GetPropertyInfo failed");

            if (propertyInfo == null || propertyInfo.Length < 1)
                return VSConstants.E_FAIL;

            try
            {
                IImageWatchAddWatchService addSvc = (IImageWatchAddWatchService)
                    Package.GetGlobalService(typeof(IImageWatchAddWatchService));

                var ex = propertyInfo[0].bstrFullName.Trim();
                
                // strip format specifiers added by VS watch window when
                // clicking on magnifying glass on a nested/derived object
                ex = Regex.Replace(ex, @",[!\w\d\[\]\(\)]+$", "");
                
                addSvc.AddWatch(ex);
            }
            catch (Exception)
            {
                return VSConstants.E_FAIL;
            }
            
            return VSConstants.S_OK;
        }
    }
}
