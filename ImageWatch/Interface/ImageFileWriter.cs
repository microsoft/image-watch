using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

using Microsoft.Research.NativeImage;

namespace Microsoft.ImageWatch.Interface
{
    static class ImageFileWriter
    {
        private static string[] GetSaveFileDialogFilter(NativeImageBase img,
            out string filter, out int defaultFilterIndex)
        {
            List<string> formats = new List<string>();

            if (img.Rectangle.Width <= 0xffff &&
                img.Rectangle.Height <= 0xffff &&
                NativeImageHelpers.CanConvertTo24BitRGB(img))
            {
                formats.Add("PNG Image (24bit RGB, lossless)|.png");
                formats.Add("JPEG Image (24bit RGB, lossy)|.jpg");
            }

            if (img.Rectangle.Width <= 0xffff &&
                img.Rectangle.Height <= 0xffff &&
                NativeImageHelpers.CanSaveAsJXR(img))
            {
                formats.Add("JPEG XR Image (original format, lossless)|.jxr");
            }

            string defaultExt = "";
            if (ImageWatchPackage.Config.EnableMicrosoftInternalTypes)
            {
                defaultExt = ".vti";
                formats.Add("VisionTools Image (original format, lossless)|.vti");
            }
            else
            {
                defaultExt = ".bin";
                formats.Add("ImageWatch Image (original format, lossless)|.bin");
            }
            
            System.Diagnostics.Debug.Assert(!string.IsNullOrWhiteSpace(
                defaultExt));

            var flist = formats
                .ToDictionary(s => s.Split('|')[0], s => s.Split('|')[1])
                .OrderBy(kv => kv.Key);

            // savefiledialog filterindex is 1-based
            defaultFilterIndex = 1;
            foreach (var kv in flist)
            {
                if (kv.Value == defaultExt)
                    break;

                ++defaultFilterIndex;
            }

            filter = string.Join("|", flist
                .Select(kv => string.Format("{0}|*{1}", kv.Key, kv.Value)));

            return flist.Select(kv => kv.Value).ToArray();
        }

        static string lastExtension_ = "";
        public static void SaveFileDialog(NativeImageBase image)
        {
            if (image == null || image.IsDisposed)
                return;

            Microsoft.Win32.SaveFileDialog dlg =
                new Microsoft.Win32.SaveFileDialog();

            dlg.AddExtension = false;

            string filter;
            int filterIndex;
            string[] extList = GetSaveFileDialogFilter(image, out filter, 
                out filterIndex);
            dlg.Filter = filter;            
            
            int lastExtensionIdx = Array.IndexOf(extList, lastExtension_);
            dlg.FilterIndex = lastExtensionIdx >= 0 ? lastExtensionIdx + 1 : filterIndex;

            var result = dlg.ShowDialog();

            string fname = null;

            if (result.HasValue && result.Value && extList != null)
            {
                fname = dlg.FileName;
                string ext = extList[dlg.FilterIndex - 1];
                if (!fname.ToLower().EndsWith(ext))
                    fname = fname + ext;

                lastExtension_ = ext;                
            }

            if (fname != null)
            {
                try
                {
                    if (fname.ToLower().EndsWith(".vti") ||
                        fname.ToLower().EndsWith(".bin"))
                        NativeImageHelpers.SaveAsVTI(image, fname);
                    else if (fname.ToLower().EndsWith(".jxr"))
                        NativeImageHelpers.SaveAsJXR(image, fname);
                    else
                        NativeImageHelpers.SaveAs24BitRGB(image, fname);
                }
                catch (ApplicationException e)
                {
                    MessageBox.Show(string.Format("Error saving {0}\n\n{1}", 
                        fname, e.Message), "Error");
                    return;
                }
            }
        }
    }
}
