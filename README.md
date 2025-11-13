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
- Visual Studio 18
- MSVC v145
- Windows 11 SDK (10.0.26200)

Usage tested with:
- Windows 11 25H2 64bit

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
In the context of game hacking the class provides an easy way to hook graphics api functions and draw to screen via dll injection. See the examples for a detailed showcase.
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
### Vector
The library provides a simple and low-overhead vector class inspired by the STL implementation. See the "Vector.h" header for further documentation.
### Loading files
The library provides a simple file loader class to load files from disk into memory. See the "FileLoader.h" header for further documentation.
### Undocumented windows structures and function types
The library provides a collection of structures and function types used by the windows operating system that are not or just partially declared in the "Windows.h" header. See the "undocWinTypes.h" header.
### Drawing from hooks
The library provides an Engine class that can be used to draw geometric shapes and text within a graphics API hook via the IBackend implementations.
Currently there are implementations of the IBackend interface for DirectX 9, DirectX 10, DirectX 11 and DirectX 12 to draw from a Present hook, for OpenGL 2 to draw from a wglSwapBuffers hook and for Vulkan to draw from a vkQueuePresentKHR hook.
Text rendering is done via a font atlas texture.
See the headers in the "draw" folder for further documentation.
#### Drawing ImGui overlays
The Engine class also supports drawing of ImGui draw data via the Engine::drawImGuiDrawData function.
The funtion only compiles if the ImGui header imgui.h is included before the Engine.h header. This is best achieved by including it before the hax.h in the source file that calls the function.
The ImGui source files:
imconfig.h, imgui.cpp, imgui.h, imgui_demo.cpp, imgui_draw.cpp, imgui_internal.h, imgui_tables.cpp, imgui_widgets.cpp
also have to be included in the project to use ImGui.
First ImGui has to be set up properly in the hook and then it can be used as in a normal application loop (ImGui::NewFrame() -> ImGui windows and widgets -> ImGui::EndFrame() -> ImGui::Render()).
See the examples\demo.h header for a full example of drawing the ImGui demo window with the Engine class.
### Examples
There are example projects that showcase the drawing engine and the tramp hook class. They can be found in the examples folder.
Build the following projects:

- DirectX10Hook
- DirectX11Hook
- DirectX12Hook
- DirectX9Hook
- OpenGL2Hook
- VulkanHook

and inject the built DLLs into a process that uses the respective graphics API.
You can find a basic DLL injector built with this library here: https://github.com/belazr/JackieBlue.
On process attach they start a thread that hooks the API and the hook then draws a demo to the screen.
Press "END" to unhook and eject the DLL.
There might be some issues if another overlay (e.g. Steam, RivaTuner, Discord) has already hooked the API.
In that case, inspect the beginning of the hooked function and adjust the parameters for the TrampHook in the dllmain.cpp of the example.
![ewh_demo](https://github.com/user-attachments/assets/22807273-ed45-43bb-b9d4-6f8d12818eaa)

## TODO
- Nothing at the moment. Issues or requests are more than welcome!

## References
- https://guidedhacking.com/
- https://learn.microsoft.com/en-us/windows/win32/api/
- https://github.com/ocornut/imgui
- https://github.com/bruhmoment21/UniversalHookX
- https://www.vergiliusproject.com/
- https://github.com/x64dbg/x64dbg
- https://github.com/khalladay/hooking-by-example
- https://www.codeproject.com/tips/139349/getting-the-address-of-a-function-in-a-dll-loaded

## Resources
- https://github.com/ocornut/imgui (usage of types in the Engine class to draw ImGui overlays)
- https://assault.cubers.net/ (game on screenshot)
- https://pixabay.com/illustrations/wallpaper-black-design-background-967837/ (demo texture)
