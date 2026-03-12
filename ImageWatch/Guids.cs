// Guids.cs
// MUST match guids.h
using System;

namespace Microsoft.ImageWatch
{
    static class GuidList
    {
        public const string guidImageWatchPkgString = "baf7d0ae-c17e-493f-9c42-cd01b6f60b6d";
        public const string guidImageWatchCmdSetString = "7ad22386-e925-45ae-80d6-84bd790a79a2";
        public const string guidToolWindowPersistanceString = "3abea8e9-72f5-4294-b206-20a0eb527cf2";

        public static readonly Guid guidImageWatchCmdSet = new Guid(guidImageWatchCmdSetString);
    };
}