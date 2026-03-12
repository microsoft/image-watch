using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;


namespace Microsoft.ImageWatch.Interface
{
    public class ExpressionInfo
    {
        public ExpressionInfo(string type)
        {
            Type = type;
        }

        public List<object> Args = new List<object>();
        public readonly string Type;

        public override bool Equals(object other)
        {
            var b = other as ExpressionInfo;
            if (b == null)
                return false;

            if (Type != b.Type)
                return false;

            if (Args.Count != b.Args.Count)
                return false;

            return Args.Zip(b.Args, (x,y) => x.Equals(y)).All(v => v);
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
    }

    public class ExpressionHelper
    {
        // The CppEE layer creates a display name string for the type that includes the module name whenever visualizing
        // a type from a separate module. This can be seen in the implementation of CppEE::CValueNode::FormatType in this file:
        // https://devdiv.visualstudio.com/DevDiv/_git/VS?path=%2Fsrc%2Fvc%2FCppEE%2Fsrc%2FNodeImpl%2FValueNode.cpp&_a=contents&version=GBmain
        //
        // For example, "typeName" turns into "typeName {Assembly.dll!typeName}". Create a regex that allows us to resolve
        // to the actual type name, needed for debug visualization of natvis files when a type is defined in a different module.
        private static readonly Regex ModuleTypeNameRegex = new Regex(@"(\s*{.+!.+})", RegexOptions.Singleline | RegexOptions.Compiled);

        public static string RemoveModuleDisplayNameFromExpression(string expression)
        {
            if (expression == null)
                return null;

            return ModuleTypeNameRegex.Replace(expression, string.Empty);
        }

        private static void DebugLog(string title, string message)
        {
#if DEBUG
            System.Diagnostics.Debug.WriteLine(" ::EXH: {0}: {1}",
                new object[] { title, message });
#endif
        }

        public static ExpressionInfo ParseImage(string expression)
        {
            if (!IsOperator(expression))
            {
                var type = GetExpressionType(expression);
                if (WatchedImageTypeMap.GetImageType(type) == null)
                    return null;

                var res = new ExpressionInfo(type);

                res.Args.Add(GetPointerType(type));
                res.Args.Add(GetObjectAddress(expression, type));

                return res;
            }
            else
            {               
                string type = null;
                string[] argStrings = null;
                string error = null;

                if (!ParseOperator(expression, ref type, ref argStrings, 
                    ref error))
                {
                    DebugLog("parsing operator", error);
                    return null;
                }

                if (type == null || argStrings == null)
                {
                    System.Diagnostics.Debug.Assert(false, "parse error");
                    return null;
                }

                Type[] argTypes = null;
                using (var opType = WatchedImageTypeMap.CreateInstance(type, out string _) as WatchedImageOperator)
                {
                    if (opType != null)
                        argTypes = opType.GetArgumentTypes();
                }

                if (argTypes == null || argTypes.Length != argStrings.Length)
                    return null;

                List<ExpressionInfo> args = new List<ExpressionInfo>();                
                for (int i = 0; i < argStrings.Length; ++i)
                {
                    if (argTypes[i] == typeof(WatchedImage))
                        args.Add(ParseImage(argStrings[i]));
                    else if (argTypes[i] == typeof(UInt32))
                        args.Add(ParseUInt32(argStrings[i]));
                    else if (argTypes[i] == typeof(UInt64))
                        args.Add(ParseUInt64(argStrings[i]));
                    else if (argTypes[i] == typeof(float))
                        args.Add(ParseFloat(argStrings[i]));
                    else if (argTypes[i] == typeof(string))
                        args.Add(ParseString(argStrings[i]));
                    else if (argTypes[i] == typeof(ElFormatStringArg))
                        args.Add(ParseElFormatString(argStrings[i]));
                    else
                        args.Add(null);
                }

                // TODO: dont return null (= "invalid expression"), but more 
                // specific parse errors
                if (args.Any(a => a == null))
                    return null;

                var res = new ExpressionInfo(type);
                res.Args.AddRange(args);

                return res;
            }
        }

        public static void InitializeImage(WatchedImage image, ExpressionInfo info, List<string> matchedNames)
        {
            if (image == null || info == null)
                return;

            if (WatchedImageTypeMap.GetImageType(info.Type) != image.GetType())
            {
                System.Diagnostics.Debug.Assert(false);
                return;
            }
            
            if (image is WatchedImageImage)
            {
                if (info.Args == null || info.Args.Count != 2
                    || !(info.Args[0] is string)
                    || !(info.Args[1] is UInt64))
                {
                    System.Diagnostics.Debug.Assert(false);
                    return;
                }
                
                (image as WatchedImageImage).Initialize(
                    info.Args[0] as string, (UInt64)info.Args[1],
                    MyToolWindow.TheController.InspectionContext,
                    MyToolWindow.TheController.Frame,
                    MyToolWindow.TheController.Process);

                if (image is WatchedImageUserImage)
                {
                    (image as WatchedImageUserImage).SetErrorLogAction(
                        msg => ImageWatchPackage.Log(msg));
                }
            }
            else if (image is WatchedImageOperator)
            {
                if (!info.Args.All(a => a is ExpressionInfo))
                {
                    System.Diagnostics.Debug.Assert(false);
                    return;
                } 
                
                var wio = image as WatchedImageOperator;
                var at = wio.GetArgumentTypes();
                if (info.Args == null || info.Args.Count != at.Length)
                {
                    System.Diagnostics.Debug.Assert(false);
                    return;
                }

                List<object> args = new List<object>();
                for (int i = 0; i < at.Length; ++i)
                {
                    var ei = info.Args[i] as ExpressionInfo;

                    if (at[i] == typeof(WatchedImage))
                    {
                        var img = WatchedImageTypeMap.CreateInstance(ei.Type, out string matchedName);
                        matchedNames.Add(matchedName);

                        // recursion
                        InitializeImage(img, ei, matchedNames);

                        args.Add(img);
                    }
                    else if (at[i] == typeof(UInt32))
                    {
                        if (!(ei.Args[0] is UInt32))
                        {
                            System.Diagnostics.Debug.Assert(false);
                            return;
                        }

                        args.Add((UInt32)ei.Args[0]);
                    }
                    else if (at[i] == typeof(UInt64))
                    {
                        if (!(ei.Args[0] is UInt64))
                        {
                            System.Diagnostics.Debug.Assert(false);
                            return;
                        }

                        args.Add((UInt64)ei.Args[0]);
                    }
                    else if (at[i] == typeof(float))
                    {
                        if (!(ei.Args[0] is float))
                        {
                            System.Diagnostics.Debug.Assert(false);
                            return;
                        }

                        args.Add((float)ei.Args[0]);
                    }
                    else if (at[i] == typeof(string))
                    {
                        if (!(ei.Args[0] is string))
                        {
                            System.Diagnostics.Debug.Assert(false);
                            return;
                        }

                        args.Add((string)ei.Args[0]);
                    }
                    else if (at[i] == typeof(ElFormatStringArg))
                    {
                        if (!(ei.Args[0] is ElFormatStringArg))
                        {
                            System.Diagnostics.Debug.Assert(false);
                            return;
                        }

                        args.Add((ElFormatStringArg)ei.Args[0]);
                    }
                    else
                    {
                        System.Diagnostics.Debug.Assert(false);
                        return;
                    }
                }
                
                (image as WatchedImageOperator).Initialize(args.ToArray(),
                    MyToolWindow.TheController.Process);
            }
            else
            {
                System.Diagnostics.Debug.Assert(false);
            }
        }

        public static bool IsOperator(string expression)
        {
            string foo = null;
            return ParseOperatorName(expression, ref foo, ref foo);
        }

        private static bool ParseOperatorName(string expression, 
            ref string name, ref string rest)
        {
            var m = Regex.Match(expression, @"(^|\s+)(@\w+).*");

            if (m == null || m.Groups.Count < 3 || m.Groups[2] == null ||
                !m.Groups[2].Success)
                return false;

            name = m.Groups[2].Value;
            rest = expression.Substring(m.Groups[2].Index + 
                m.Groups[2].Length);

            return true;
        }

        private static bool ParseOperator(string expression, 
            ref string operatorName, ref string[] arguments, 
            ref string parseError)
        {
            operatorName = null;
            arguments = null;
            parseError = null;

            if (string.IsNullOrWhiteSpace(expression))
                return false;

            var e = expression.Trim();
            
            string rest = null;
            if (!ParseOperatorName(e, ref operatorName, ref rest))
            {
                parseError = "Invalid operator syntax";
                return false;
            }

            int lastpos = 0;
            var args = ExtractFunctionArguments(rest, ref lastpos);

            if (args != null)
            {
                if (!string.IsNullOrWhiteSpace(rest.Substring(lastpos)))
                {
                    parseError =
                        string.Format("{0}: unexpected symbols: {1}",
                        operatorName, rest.Substring(lastpos).Trim());

                    return false;
                }

                arguments = args;
            }
            else
            {
                parseError = string.Format("{0}: invalid argument syntax",
                    operatorName);

                return false;
            }

            return true;
        }

        static string[] ExtractFunctionArguments(string s, ref int lastpos)
        {
            s = s.Trim();
            if (!s.StartsWith("("))
                return null;

            int c = 0;
            int pos = 0;
            while (pos < s.Length)
            {
                if (s[pos] == '(')
                    ++c;
                else if (s[pos] == ')')
                    --c;

                if (c == 0)
                {
                    lastpos = pos + 1;
                    var args = SplitAtTopLevelCommas(s.Substring(1, pos - 1));

                    if (args == null ||
                        args.Any(a => string.IsNullOrWhiteSpace(a)))
                        return null;

                    return args.Select(a => a.Trim()).ToArray();
                }

                ++pos;
            }

            lastpos = pos;
            return null;
        }

        static string[] SplitAtTopLevelCommas(string s)
        {
            if (string.IsNullOrWhiteSpace(s))
                return null;

            int c = 0;
            int pos = 0;
            List<int> splitPos = new List<int>();
            while (pos < s.Length)
            {
                if (s[pos] == '(')
                    ++c;
                else if (s[pos] == ')')
                    --c;
                if (c == 0 && s[pos] == ',')
                    splitPos.Add(pos);

                ++pos;
            }

            if (splitPos.Count == 0)
                return new string[] { s.Trim() };

            List<string> res = new List<string>();
            int start = 0;
            for (int i = 0; i < splitPos.Count; ++i)
            {
                int len = splitPos[i] - start;
                res.Add(s.Substring(start, len).Trim());
                start += len + 1;
            }
            res.Add(s.Substring(start));

            return res.ToArray();
        }

        private static ExpressionInfo ParseUInt32(string e)
        {
            String valueStr = null;
            String typeStr = null;
            if (!WatchedImageHelpers.EvaluateExpression(
                MyToolWindow.TheController.InspectionContext,
                MyToolWindow.TheController.Frame,
                "(unsigned int)" + e,
                ref valueStr, ref typeStr, "u"))
                return null;

            UInt32 value;
            if (!UInt32.TryParse(valueStr, out value))
                return null;

            var res = new ExpressionInfo("#UInt32");            
            res.Args.Add(value);

            return res;
        }

        private static ExpressionInfo ParseUInt64(string e)
        {
            String valueStr = null;
            String typeStr = null;
            if (!WatchedImageHelpers.EvaluateExpression(
                MyToolWindow.TheController.InspectionContext,
                MyToolWindow.TheController.Frame,
                "(unsigned __int64)" + e,
                ref valueStr, ref typeStr, "u"))
                return null;

            UInt64 value;
            if (!UInt64.TryParse(valueStr, out value))
                return null;

            var res = new ExpressionInfo("#UInt64");
            res.Args.Add(value);

            return res;
        }
        
        private static ExpressionInfo ParseFloat(string e)
        {
            String valueStr = null;
            String typeStr = null;
            if (!WatchedImageHelpers.EvaluateExpression(
                MyToolWindow.TheController.InspectionContext,
                MyToolWindow.TheController.Frame,
                "(float)" + e, 
                ref valueStr, ref typeStr))
                return null;

            float value;
            if (!float.TryParse(valueStr, out value))
                return null;

            var res = new ExpressionInfo("#Float");
            res.Args.Add(value);

            return res;
        }

        private static ExpressionInfo ParseString(string e)
        {
            // double all single backslashes to work around VS's behavior 
            // with "escape characters" in string literals in the watch window 
            // NOTE: the pattern and replacement strings are the same, but
            // that's correct since pattern is a regular expression, 
            // replacement is not
            e = Regex.Replace(e, @"\\", @"\\");

            String valueStr = null;
            String typeStr = null;
            if (!WatchedImageHelpers.EvaluateExpression(
                MyToolWindow.TheController.InspectionContext,
                MyToolWindow.TheController.Frame,
                e, ref valueStr, ref typeStr))
                return null;

            if (typeStr == null)
            {
                // if debugger cannot parse this, it may be a string without 
                // quotes, which we do allow. -> try to parse again with 
                // quotes
                e = "\"" + e + "\"";

                if (!WatchedImageHelpers.EvaluateExpression(
                    MyToolWindow.TheController.InspectionContext,
                    MyToolWindow.TheController.Frame,
                    e, ref valueStr, ref typeStr))
                    return null;
            }
                        
            // is this a wide char string?
            if (typeStr == null)
                return null;
            var format = typeStr.Contains("wchar_t") ? "su" : "s";

            if (!WatchedImageHelpers.EvaluateExpression(
                MyToolWindow.TheController.InspectionContext,
                MyToolWindow.TheController.Frame,
                e, ref valueStr, ref typeStr, format, false))
                return null;

            if (valueStr == null)
                return null;

            var nakedString = "";
            if (format == "su" && valueStr.Length >= 3)
                nakedString = valueStr.Substring(2, valueStr.Length - 3);
            else
                nakedString = valueStr;

            var res = new ExpressionInfo("#String");
            res.Args.Add(nakedString);
            
            return res;
        }

        private static ExpressionInfo ParseElFormatString(string e)
        {
            var arg = ElFormatStringArg.Parse(e);
            if (arg == null)
                return null;

            var res = new ExpressionInfo("#ElFormatString");
            res.Args.Add(arg);

            return res;
        }
        
        private static string GetPointerType(string expressionType)
        {
            if (string.IsNullOrEmpty(expressionType))
                return null;

            // strip constness from pointer/refs, but not from value
            expressionType = Regex.Replace(expressionType.Trim(),
                @"(\b)(const)(\W|$)",
                m =>
                {
                    if (m.Index == 0)
                        return m.ToString();
                    else
                        return m.Groups[1].ToString() + m.Groups[3].ToString();
                }).Trim();

            return expressionType.TrimEnd("^&* ".ToCharArray()) + " *";
        }
        
        private static string GetExpressionType(string expression)
        {
            string value = null;
            string type = null;
            bool res = WatchedImageHelpers.EvaluateExpression(
                MyToolWindow.TheController.InspectionContext,
                MyToolWindow.TheController.Frame, expression,
                ref value, ref type);

            if (res)
            {
                type = RemoveModuleDisplayNameFromExpression(type);
            }

            return res ? type : null;
        }

        private static UInt64 GetObjectAddress(string expression,
            string expressionType)
        {
            if (string.IsNullOrEmpty(expression) ||
                string.IsNullOrEmpty(expressionType))
            {
                return 0;
            }

            try
            {
                string addrExpr = expression;

                // count ptr/refs (but exclude ptr/ref template arguments)
                int lastTemplatePos = expressionType.LastIndexOf('>');
                int nli = expressionType.Skip(lastTemplatePos + 1)
                    .Count(c => c == '*' || c == '^');

                if (nli == 0)
                {
                    addrExpr = string.Format("(void*)(&({0}))", addrExpr);
                }
                else if (nli == 1)
                {
                    addrExpr = string.Format("(void*)({0})", addrExpr);
                }
                else
                {
                    addrExpr = string.Format("(void*)({0}({1}))",
                        new string('*', nli - 1), addrExpr);
                }

                string value = null;
                string type = null;
                if (!WatchedImageHelpers.EvaluateExpression(
                    MyToolWindow.TheController.InspectionContext,
                    MyToolWindow.TheController.Frame, addrExpr,
                    ref value, ref type))
                    return 0;

                return value.StartsWith("0x") ?
                    Convert.ToUInt64(value.Substring(2), 16) :
                    Convert.ToUInt64(value);
            }
            catch (Exception)
            {
                return 0;
            }
        }
    }
}
