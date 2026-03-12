using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Reflection;
using System.Xml.Linq;

namespace Microsoft.ImageWatch
{
    public static class WatchedImageTypeMap
    {
        private struct NamedType
        {
            public string Name { get; set; }
            public Type Type { get; set; }
        }

        static Dictionary<string, NamedType> imageTypes_ = new Dictionary<string, NamedType>();

        public static void Initialize(bool enableInternalTypes, bool enableUserTypes)
        {
            imageTypes_.Clear();

            // internal image types
            if (enableInternalTypes)
            {
                AddImage(@"vt::(CImgInCache(\W|$)|CTypedImgInCache<.+|CCompositeImgInCache<.+)",
                    "VTCImgInCache",
                    typeof(WatchedImageVTCImgInCache));

                AddImage(@"ID3D11Resource(\W|$)|ID3D11Texture2D(\W|$)",
                    "D3D11Resource",
                    typeof(WatchedImageD3D11Resource));
            }

            // add internal user types
            AddUserTypes(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), overrideName: null);

            AddUserTypes(ImageWatchPackage.DefaultVisualizerPath, "DefaultVisualizerPathType");

            if (enableUserTypes)
                AddUserTypes(ImageWatchPackage.UserPath, "UserType");

            // operation types
            AddOperator("band", typeof(WatchedImageBandOp));
            AddOperator("clamp", typeof(WatchedImageClampOp));
            AddOperator("scale", typeof(WatchedImageScaleOp));
            AddOperator("abs", typeof(WatchedImageAbsOp));
            AddOperator("file", typeof(WatchedImageFileOp));
            AddOperator("diff", typeof(WatchedImageDiffOp));
            AddOperator("norm8", typeof(WatchedImageNorm8Op));
            AddOperator("norm16", typeof(WatchedImageNorm16Op));
            AddOperator("thresh", typeof(WatchedImageThreshOp));
            AddOperator("mem", typeof(WatchedImagePtrOp));
            AddOperator("fliph", typeof(WatchedImageFlipHorizOp));
            AddOperator("flipv", typeof(WatchedImageFlipVertOp));
            AddOperator("flipd", typeof(WatchedImageTransposeOp));
            AddOperator("rot90", typeof(WatchedImageRot90Op));
            AddOperator("rot180", typeof(WatchedImageRot180Op));
            AddOperator("rot270", typeof(WatchedImageRot270Op));
        }

        static void AddUserTypes(string path, string overrideName)
        {
            if (string.IsNullOrEmpty(path))
                return;

            var utypes = UserTypeLoader.LoadUserTypes(path, imageTypes_.Keys.ToArray());

            foreach (var t in utypes)
                AddImage(t + @"(\W|$)", overrideName ?? t, typeof(WatchedImageUserImage));
        }

        static void AddImage(string typeRegex, string name, Type type)
        {
            var key = @"^((const\s+)|(\s*))?" + typeRegex;
            if (imageTypes_.ContainsKey(key))
            {
                System.Diagnostics.Debug.Assert(false);
                return;
            }

            imageTypes_.Add(key, new NamedType { Name = name, Type = type });
        }

        static void AddOperator(string nakedName, Type type)
        {
            var key = @"^(\s*)?@" + nakedName + @"(\W|$)";
            if (imageTypes_.ContainsKey(key))
            {
                System.Diagnostics.Debug.Assert(false);
                return;
            }

            imageTypes_.Add(key, new NamedType { Name = nakedName, Type = type });
        }

        public static Type GetImageType(string typeName, out string originalName)
        {
            originalName = string.Empty;
            if (typeName == null)
                return null;

            // ignore module part of type name (it's the natvis author's
            // responsibility to make sure the natvis is properly <Version>'d
            int tidx = typeName.IndexOf('!'); // if not found, tidx = -1
            if (tidx >= 0)
            {
                typeName = Interface.ExpressionHelper.RemoveModuleDisplayNameFromExpression(typeName);
            }

            foreach (var it in imageTypes_)
            {
                if (Regex.IsMatch(typeName, it.Key))
                {
                    originalName = it.Value.Name;
                    return it.Value.Type;
                }
            }

            return null;
        }

        public static Type GetImageType(string typeName)
        {
            return GetImageType(typeName, out string _);
        }

        public static WatchedImage CreateInstance(string typeName, out string matchedName)
        {
            var type = GetImageType(typeName, out matchedName);
            if (type == null)
                return null;

            try
            {
                return Activator.CreateInstance(type)
                    as WatchedImage;
            }
            catch (Exception)
            {
                System.Diagnostics.Debug.Assert(false);
                return null;
            }
        }
    }
}
