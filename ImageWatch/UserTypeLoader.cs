using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Xml.Linq;
using System.IO;
using System.Reflection;

namespace Microsoft.ImageWatch
{
    static class UserTypeLoader
    {
        static void Log(string msg)
        {
            ImageWatchPackage.Log(msg);
        }

        static bool CheckAttribute(XElement e, string name, string value,
            bool valueCaseInsensitive)
        {
            if (e == null)
                return false;

            var a = e.Attribute(name);

            if (a == null || a.Value == null)
                return false;

            return a.Value.Equals(value,
                StringComparison.InvariantCultureIgnoreCase);
        }

        static bool TypeUsesImageWatchService(XElement e)
        {
            try
            {
                return e.Elements()
                    .Where(x => x.Name.LocalName == "UIVisualizer")
                    .Where(x => CheckAttribute(x, "ServiceId",
                        "{A452AFEA-3DF6-46BB-9177-C0B08F318025}", true))
                    .Where(x => CheckAttribute(x, "Id", "1", true))
                    .Count() > 0;
            }
            catch (Exception)
            {
                return false;
            }
        }

        static bool TypeImplementsInfo(XElement e)
        {
            var infoElements = new string[] 
            {
                "[type]",
                "[channels]",
                "[width]",
                "[height]",
                "[data]",
                "[stride]",
            };

            try
            {
                var ee = e.Elements().Where(x => x.Name.LocalName == "Expand");
                if (ee.Count() == 0)
                {
                    Log(string.Format("<Expand> node not found"));
                    return false;
                }

                var nameLookup = ee.First().Elements()
                    .Where(x => x.Name.LocalName == "Item" ||
                        x.Name.LocalName == "Synthetic")
                    .Select(x => x.Attribute("Name"))
                    .ToLookup(s => s.Value);

                foreach (var s in infoElements)
                {
                    if (!nameLookup.Contains(s))
                    {
                        Log(string.Format(
                            "<Item> with Name=\"{0}\" not found", s));
                        return false;
                    }
                }

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        static string NormalizeWhiteSpace(string s)
        {
            // To match VS debugger type formatting, remove all whitespace 
            // except:
            // 1) leave one space between any two disconnected word 
            // characters (e.g. "const char") 
            // 2) force one space between any two right angle brackets 
            // (templates)

            s = s.Trim();

            for (int i = 0; i < 3; ++i)
            {
                string old = s;
                s = Regex.Replace(s, @"(\w)\s+(\w)", "$1 $2");
                s = Regex.Replace(s, @"(\W)\s+(\W)", "$1$2");
                s = Regex.Replace(s, @"(\w)\s+(\W)", "$1$2");
                s = Regex.Replace(s, @"(\W)\s+(\w)", "$1$2");

                if (old == s)
                    break;
            }

            for (int i = 0; i < 3; ++i)
            {
                string old = s;
                s = Regex.Replace(s, @">>", "> >");

                if (old == s)
                    break;
            }

            return s;
        }

        static string[] ExtractTypeNamesFromNatvis(string filename)
        {
            List<string> res = new List<string>();

            try
            {
                XDocument doc = XDocument.Load(filename);

                var typeNodes = doc.Root.Elements()
                    .Where(x => x.Name.LocalName == "Type")
                    .Where(x => x.Attribute("Name") != null);

                var names = typeNodes.Select(t => t.Attribute("Name").Value)
                    .Select(t => NormalizeWhiteSpace(t))
                    .Distinct();

                foreach (var name in names)
                {
                    // combine all "type" nodes with this name. this is a 
                    // workaround for an issue in VS2012: you cannot put a
                    // UIVisualizer and an Expand node into the same natvis
                    // type _definition_. ImageWatch needs both, so we have
                    // to write TWO type definitions, one with UIVisualizer
                    // and one with Expand
                    var type = new XElement("Type", typeNodes
                        .Where(t => NormalizeWhiteSpace(
                            t.Attribute("Name").Value) == name)
                        .SelectMany(t => t.Elements())
                        .ToArray());

                    if (!TypeUsesImageWatchService(type))
                    {
                        // don't log, too noisy. just ignore..
                        //Log("<UIVisualizer> node not found");
                        continue;
                    }

                    Log(string.Format("Found type {0} ...", name));

                    //if (!TypeImplementsInfo(type))
                    //    continue;

                    res.Add(name);

                    foreach (var at in type.Elements()
                        .Where(x => x.Name.LocalName == "AlternativeType")
                        .Where(x => x.Attribute("Name") != null)
                        .Select(x => NormalizeWhiteSpace(
                            x.Attribute("Name").Value))
                        .Distinct())                            
                    {
                        Log(string.Format(
                            "Found alternative type name: {0} ...", at));

                        res.Add(at);
                    }
                }
            }
            catch (Exception e)
            {
                Log(string.Format("XML parse error: {0}", e.Message));
            }

            return res.Where(n => !string.IsNullOrWhiteSpace(n))
                .Distinct().ToArray();
        }

        static bool IsTypenameValid(string tn)
        {
            if (Regex.IsMatch(tn, @"[^\w<>\*:,\s]"))
                return false;

            return true;
        }

        static string[] EnumerateUserNatvisFiles(string path)
        {
            try
            {
                return Directory.EnumerateFiles(path, "*.natvis")
                    // skip internally implemented (VT) types
                    .Where(fn => Path.GetFileNameWithoutExtension(fn)
                        != "ImageWatch")
                    .ToArray();
            }
            catch (Exception)
            {
                return new string[] { };
            }
        }

        public static string[] LoadUserTypes(string path,
            string[] existingNamesRegex)
        {
            List<string> res = new List<string>();

            Log(string.Format("Looking for natvis image types in {0} ...",
                path));

            foreach (var fn in EnumerateUserNatvisFiles(path))
            {
                Log(string.Format("Parsing {0} ...",
                    Path.GetFileName(fn)));

                string[] typeNames = ExtractTypeNamesFromNatvis(fn);
                if (typeNames == null || typeNames.Length < 1)
                    continue;

                foreach (var tn in typeNames)
                    Log(string.Format("-> Registering {0}", tn));

                bool inUse = false;
                foreach (var n in typeNames)
                {
                    if (res.Contains(n) ||
                        existingNamesRegex.Any(r => Regex.IsMatch(n, r)))
                    {
                        Log(string.Format(
                            "ERROR: Type name {0} already in use", n));
                        inUse = true;
                    }
                }                
                if (inUse)
                    continue;

                var existingNames = res.ToLookup(s => s);
                if (typeNames.Any(tn => existingNames.Contains(tn)))
                {
                    Log(string.Format("ERROR: Type name {0} already in use",
                        typeNames.First(tn => existingNames.Contains(tn))));
                    continue;
                }

                if (typeNames.Any(tn => !IsTypenameValid(tn)))
                {
                    Log(string.Format("ERROR: Invalid type name {0}",
                        typeNames.First(tn => !IsTypenameValid(tn))));
                    continue;
                }

                foreach (var tn in typeNames)
                    res.Add(tn.Replace("*", ".+"));
            }

            return res.ToArray();
        }
    }
}
