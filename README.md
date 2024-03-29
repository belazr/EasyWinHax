# EasyWinHax
EasyWinHax is a C++ library designed to provide basic and low abstraction functionallity for windows process hacking and more specifically game hacking.
This project was created to learn and help others to learn the ins and outs of windows, the portable executable format and low level concepts of code execution by example. Therefore the code is kept very c-ish (even though it uses some c++ concepts) in the hope of not hiding the essentials behind abstraction. This is not an attempt to write a by-the-books modern C++ library. Most importantly it is about having fun with computers!

## Requirements
- Windows 10/11 64bit
- Visual Studio
- C++ 14 compliant compiler
- Windows SDK

Build tested with:
- Windows 11 64bit
- Visual Studio 17
- MSVC v143
- Windows 11 SDK (10.0.22621)

Usage tested with:
- Windows 11 22H2 64bit

## Build
Open the solution file (EasyWinHax.sln) with Visual Studio and run the desired builds from there.
By default a static library with static runtime library linkage (/MT and /MTd) is built.
This way programs linking to this library can be completely portable with no DLL dependencies.
## Usage with Visual Studio 17
To create a program linking to this library, create an empty project in visual studio.
Then go to the project property page and set C/C++->Code Generation->Runtime Libraray to "Multi-threaded (/MT)" and "Multi-threaded Debug (/MTd)" for Release and Debug configurations respectively.
Under Linker->General->Additional Library Directories add the path of the EasyWinHax.lib file for each build configuration and platform.
Under Linker->Input->Additional Dependencies add "EasyWinHax.lib".
Finally include the headers "hax.h" and use the provided functions and classes as documented at their declaration in the header files.

## Functionallity
Even though EasyWinHax was written to provide helpers for game hacking it can be helpful in interacting and manipulating any windows process.
### Process information
The library provides functions to retrieve information about a windows process including reimplementations of some Win32 API functions with added advantages. Most function are defined to interact with the caller process as well as an external target process. The external functions are implemented so that the x64 builds of these functions are able to retrieve information about an x86 as well as an x64 external target process. Possible process information is eg. process id, process environment block, loader data, import and export adresses of functions. For example proc::ex::getProcAddress is able to get the address of an exported function (like the Win32 version) but on external processes and independent of the target architechture. See the "proc.h" header for further documentation.
### Memory interaction
The library provides functions to interact with the virtual memory of a process. Again most functions are defined to interact with the caller process as well as an external target process. The external functions are again implemented so that the x64 compilations of these functions are able to interact with the virtual memory of an x64 as well as an x86 target process. Possible memory interactions are eg. low level hooking, patching and memory pattern scanning. See the "mem.h" header for further documentation.
### Launching code
The library provides functions to launch and execute code in an external target process. It supports launching via CreateRemoteThread, thread hijacking, SetWindowsHookEx, hooking NtUserBeginPaint and QueueUserAPC including retriving the return value of the executed code. See the "launch.h" header for further documentation.
### Vector math
The library provides basic vector types and functions, as well as world to screen functions for column- and row-major projection matricies. See the "vecmath.h" header for further documentation.
### Function hooking
#### Trampoline hook
The library provides classes to install a trampoline hook in the beginning of a function.
The internal TrampHook class hooks functions within the same process it is instantiated in. The function redirected to is usually defined in a DLL injected into the process.
In the context of game hacking the class provides an easy way to hook graphics api functions (eg. EndScene or wglSwapBuffer) and draw to screen via dll injection.
The external TrampHook class hooks functions in an external target process. The function redirected to is shell code the object injects into the process.
Due to the limitations of shell code the use cases are limited.
Though some fun can be had with it like hooking SystemQueryProcessInformation in a TaskManger.exe instance and hiding a process from it or hooking NtUserBeginPaint to launch shell code like the JackieBlue DLL injector does.
The x64 compilation of the external class is able to hook functions of x64 as well as x86 target processes.
See the "hooks\TrampHook.h" header for further documentation.
#### Import address table hook
The library further provides classes to install an import address table hook.
The internal IatHook class hooks the IAT of a module of the caller process, again execution usually redirected to a function within an injected DLL.
The external IatHook class hooks the IAT of a module of an external target process, again execution redirected to shell code.
The same restrictions as for the external trampoline hook apply.
The x64 compilation of the external class is able to hook IATs of modules of x64 as well as x86 target processes.
See the "hooks\IatHook.h" header for further documentation.
### Benchmarking
The library provides a simple benchmarking class to benchmark code execution. It is useful for measuring the average execution time of code in a function hook. See the "Bench.h" header for further documentation.
### Loading files
The library provides a simple file loader class to load files from disk into memory. See the "FileLoader.h" header for further documentation.
### Undocumented windows structures and function types
The library provides a collection of structures and function types used by the windows operating system that are not or just partially declared in the "Windows.h" header. See the "undocWinTypes.h" header.
### Drawing from hooks
The library provides an Engine class that can be used to draw geometric shapes within a hook independent of graphics API via the IDraw interface.
Currently there are implementations of the IDraw interface for DirectX 11 to draw from a Present hook, DirectX 9 to draw from an EndScene hook and for OpenGL 2 to draw from an wglSwapBuffers hook.
See the "engine\Engine.h" and "engine\IDraw.h" headers for further documentation.
#### DirectX 11 implementation
The library provides a DirectX 11 implementation of the IDraw interface to draw triangle strips and text. The Engine class uses these implementations to draw more advanced shapes.
It provides a font class that can be used for text drawing. See the headers in the "engine\dx11" and "engine\dx" folders for further documentation.
#### DirectX 9 implementation
The library provides a DirectX 9 implementation of the IDraw interface to draw triangle strips and text. The Engine class uses these implementations to draw more advanced shapes.
It provides a font class that can be used for text drawing without the need to include the deprecated DirectX SDK. See the headers in the "engine\dx9" and "engine\dx" folders for further documentation.
#### OpenGL 2 implementation
The library provides an OpenGL 2 implementation of the IDraw interface to draw triangle strips and text. The Engine class uses these implementations to draw more advanced shapes.
It also provides a font class that can be used for text drawing. See the headers in the "engine\ogl2" folder for further documentation.

## TODO
- Better error handling for launch functions
- Add further implementations of the IDraw interface (DirectX 12, Vulkan)

## References
- https://guidedhacking.com/
- https://learn.microsoft.com/en-us/windows/win32/api/
- https://www.vergiliusproject.com/
- https://github.com/x64dbg/x64dbg
- https://github.com/khalladay/hooking-by-example
- https://www.codeproject.com/tips/139349/getting-the-address-of-a-function-in-a-dll-loaded
