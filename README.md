# EasyWinHax
EasyWinHax is a C++ library designed to provide basic and low abstraction functionallity for windows process hacking and more specifically game hacking.
It is written to learn and help others to learn the ins and outs of windows, the portable executable format and low level concepts of code execution by example. Therefore the code is kept very c-ish (even though it uses some c++ concepts) in the hope of not hiding the essentials behind abstraction. This is not an attempt to write a by-the-books modern C++ library. Most importantly it is about having fun with computers!

## Requirements
- Visual Studio
- C++ 14 compliant compiler
- Windows SDK

Build tested with:
- Visual Studio 17
- MSVC v143
- Windows 11 SDK (10.0.22621)

Usage tested with:
- Windows 10
- Windows 11

## Build
Open the solution file (EasyWinHax.sln) with visual Studio and run the desired builds from there.
By default a static library with static runtime library linkage (/MT and /MTd) is built.
This way programs linking to this library can be completely portable with no dll dependencies.
## Usage with Visual Studio 17
To create a program linking to this library, create an empty project in visual studio.
Then go to the project property page and set C/C++->Code Generation->Runtime Libraray to "Multi-threaded (/MT)" and "Multi-threaded Debug (/MTd)" for Release and Debug configurations respectively.
Under Linker->General->Additional Library Directories add the path of the EasyWinHax.lib file for each build configuration and platform.
Under Linker->Input->Additional Dependencies add "EasyWinHax.lib".
Finally include the headers "hax.h" for general functionallity, "dx9.h" for drawing inside a DirectX 9 hook and/or "ogl.h" for drawing inside an OpenGL hook and use the provided functions and classes as documented at their declaration in the header files.

## Functionallity
Even though EasyWinHax was written to provide helpers for game hacking it can be helpful in interacting and manipulating any windows process.
### Process information
The library provides functions to retrieve information about a windows process including reimplementations of some Win32 API functions with added advantages. Most function are defined to interact with the caller process as well as an external target process. The external functions are implemented so that the x64 builds of these functions are able to retrieve information about an x86 as well as an x64 external target process. Possible process information is eg. process id, process environment block, loader data, import and export adresses of functions. For example proc::ex::getProcAddress is able to get the address of an exported function (like the Win32 version) but on external processes and independent of the target architechture. See the "proc.h" header for further documentation.
### Memory interaction
The library provides functions to interact with the virtual memory of a process. Again most furnctions are defined to interact with the caller process as well as an external target process. The external functions are again implemented so that the x64 builds of these functions are able to interact with the virtual memory of an x86 as well as an x64 external target process. Possible memory interactions are eg. low level hooking, patching and memory pattern scanning. See the "mem.h" header for further documentation.
### Vector math
The library provides basic vector types and functions, as well as world to screen functions for column- and row-major projection matricies. See the "vecmath.h" header for further documentation.
### Function hooking
The library provides classes to install a trampoline hook in the beginning of a function.
The InternalHook class hooks functions within the same process it is instantiated in. The function redirected to is usually defined in a dll injected into the process. In the context of game hacking the class provides an easy way to hook graphics api functions (eg. EndScene or wglSwapBuffer) and draw to screen via dll injection. See the "hooks\InternalHook.h" header for further documentation.
The ExternalHook class hooks functions in an external target process. The function redirected to is shell code the object injects into the process. Due to the limitations of shell code the use cases are limited. Though some fun can be had with it like hooking SystemQueryProcessInformation in a TaskManger process and hiding a process from it. See the "hooks\ExternalHook.h" header for further documentation.
### Benchmarking
The library provides a simple benchmarking class to benchmark code execution. It is useful for measuring the average execution time of code in a function hook. See the "Bench.h" header for further documentation.
### Loading files
The library provides a simple file loader class to load files from disk into memory. See the "FileLoader.h" header for further documentation.
### DirectX 9 drawing
The library provides a way to setup an EndScene hook and some functions to draw basic shapes within the hook. It also provides a font class that can be used to draw text to the screen without including the deprecated DirectX SDK. See the headers in the "dx9" folder for further documentation.
### OpenGL drawing
The library provides a way to setup an wglSwapBuffers hook and some functions to draw basic shapes within the hook. It also provides a font class that can be used to draw text to the screen. See the headers in the "ogl" folder for further documentation.
### Undocumented windows structures and function types
The library provides a collection of structures and function types used by the windows operating system that are not or just partially declared in the "Windows.h" header. See the "undocWinTypes.h" header.

## TODOs
- Import by ordinal support for proc::in::getProcAddress and proc::ex::getProcAddress
- Import address table hook class

## References
https://guidedhacking.com/
https://learn.microsoft.com/en-us/windows/win32/api/
https://www.vergiliusproject.com/
https://github.com/khalladay/hooking-by-example
https://www.codeproject.com/tips/139349/getting-the-address-of-a-function-in-a-dll-loaded