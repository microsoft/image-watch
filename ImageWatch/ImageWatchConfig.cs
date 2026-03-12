using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;

namespace Microsoft.ImageWatch
{
    public class ImageWatchConfig
    {
        public ImageWatchConfig()
        {
            // default (external) values
            HelpPageURL = DefaultHelpPageURL;
            EnableMicrosoftInternalTypes = false;
            MaxPixelMemoryUsageInMegabytes = 1024;

            // check if config exists
            try
            {
                var path = Path.Combine(ImageWatchPackage.UserPath,
                    "ImageWatch.config");

                if (!File.Exists(path))
                    return;

                var doc = XDocument.Load(path);
        
                var hp = GetConfigValue(doc, "HelpPageURL");
                if (hp != null)
                    HelpPageURL = hp;

                var umit = GetConfigValue(doc, "EnableMicrosoftInternalTypes");
                bool umitVal;
                if (umit != null && bool.TryParse(umit, out umitVal))
                    EnableMicrosoftInternalTypes = umitVal;

                var mpm = GetConfigValue(doc, "MaxPixelMemoryUsageInMegabytes");
                UInt32 mpmVal;
                if (mpm != null && UInt32.TryParse(mpm, out mpmVal))
                    MaxPixelMemoryUsageInMegabytes = mpmVal;
            }
            catch (Exception)
            {
            }
        }

        string GetConfigValue(XDocument doc, string key)
        {
            try
            {
                var e = doc.Root.Descendants(key);
                if (e.Count() > 0)
                    return e.First().Value;
                
            }
            catch (Exception)
            {
            }

            return null;
        }

        public string HelpPageURL { get; private set; }
        public bool EnableMicrosoftInternalTypes { get; private set; }
        public UInt32 MaxPixelMemoryUsageInMegabytes { get; private set; }
        public bool EnableD3DTypes { get; private set; }

        public string DefaultHelpPageURL { get { return "http://go.microsoft.com/fwlink/?LinkId=285461"; } }
    }
}
