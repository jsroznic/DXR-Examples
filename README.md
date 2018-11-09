# DXR-Examples
Introductory programs to begin learning DirectX 12 Raytracing. 

This repository is derived from the work of Adam Marrs's IntroToDXR repository, found [here.](https://github.com/acmarrs/IntroToDXR)

![Program Preview](https://github.com/jsroznic/DXR-Examples/blob/master/DXR-Scene.PNG "Output Preview")

## Requirements

* Windows 10 v1809, "October 2018 Update" (RS5)
* Windows 10 SDK v1809 (10.0.17763.0). [Download it here.](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive)
* Visual Studio 2017 or VS Code

## Code Organization

Data is passed through the application using structs. These structs are defined in `Structures.h` and are organized into three categories: 

* Global
* Standard D3D12
* DXR

## Command Line Arguments

* `-width [integer]` specifies the width (in pixels) of the rendering window
* `-height [integer]` specifies the height(in pixels of the rendering window

## Licenses and Open Source Software

The code uses two dependencies:
* [TinyObjLoader](https://github.com/syoyo/tinyobjloader-c/blob/master/README.md), provided with an MIT license. 
* [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h), provided with an MIT license.
