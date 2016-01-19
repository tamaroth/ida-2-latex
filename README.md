# IDA2LaTeX

To build this package with visual studio, you'll need:

- **Visual Studio** (I'm using Visual Studio 2015)
- **IDA SDK** (most likely version 6 and above, this code has been compiled with IDASDK 6.9)

**To properlly build and install the plugin, you must**:

* Go to Project Properties -> General
  * Change **Output Directory** to your IDA plugin directory
* Go to Project PRoperties -> C++ -> General
  * Change **Additional Include Directories** to IDA SDK include directory
* Go to Project Properties -> Linker -> Input
  * Change **Additional Dependencies** to %IDASDK%\lib\x86_win_vc_32\ida.lib where %IDASDK% is the directory where you have your IDA SDK.
