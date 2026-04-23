# Image Watch for Visual Studio

This repository contains sample source code used by the Image Watch Visual Studio extension, a watch window for visualizing in-memory images (bitmaps) when debugging native C++ code.

> **Deprecation notice:** Image Watch was deprecated as of April 1, 2026, and will be removed. The following Visual Studio Marketplace listings are no longer maintained and will be removed:
>
> - [Image Watch](https://marketplace.visualstudio.com/items?itemName=VisualCPPTeam.ImageWatch)
> - [Image Watch for Visual Studio 2017](https://marketplace.visualstudio.com/items?itemName=VisualCPPTeam.ImageWatch2017)
> - [Image Watch for Visual Studio 2019](https://marketplace.visualstudio.com/items?itemName=VisualCPPTeam.ImageWatch2019)
> - [Image Watch for Visual Studio 2022](https://marketplace.visualstudio.com/items?itemName=VisualCPPTeam.ImageWatchForVisualStudio2022)

# Required Workloads and Components

Workloads
- .NET Desktop Development
- Desktop Development with C++
- Visual Studio Extension Development

Components
- C++ Spectre Mitigated libraries for x64/86 (Latest MSVC) 
- Windows 10 SDK
- .NET Framework 4.7.2 targeting pack

# Getting started

1) Open ImageWatch.sln in Visual Studio 2022 or later.
2) Choose Debug or Release configuration.
3) Select "Build" > "Build Solution".
4) To run tests:
    * Open "Test" > "Test Explorer".
    * Select "Run All ..." or "Run".
        * Note: Tests can be grouped by Traits; Categories "Perf" and "Slow" might be better suited for Release configuration.
5) To run the extension:
    * Open context menu for ImageWatch.Packaging project in Solution Explorer.
    * Select "Set as Startup Project".
    * Select "Debug" > "Start Debugging"

# Additional notes

Project Test/ConsoleApplication1 is provided as a sample OpenCV application to manually test Image Watch functionality on.
Once its required dependencies are acquired (e.g., through vcpkg), it can be added to the solution build via "Build" > "Configuration Manager".
