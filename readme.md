# Image Watch for Visual Studio 2022

Visual Studio extension providing a watch window for visualizing in-memory images (bitmaps) when debugging native C++ code.

# Getting started

1) Open ImageWatch.sln in Visual Studio 2022.
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
