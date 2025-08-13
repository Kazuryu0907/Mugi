# Codebase Structure

## Root Directory Files
- **Mugi.sln**: Visual Studio solution file
- **Mugi.vcxproj**: Main project file with build configuration
- **BakkesMod.props**: MSBuild properties for BakkesMod SDK integration
- **version.h**: Version definitions (auto-updated by PowerShell script)
- **update_version.ps1**: PowerShell script to increment build version

## Core Source Files
- **Mugi.cpp**: Main plugin implementation with event handlers and socket communication
- **Mugi.h**: Header file with class definition and data structures
- **GuiBase.cpp/h**: Base classes for GUI components (settings and plugin windows)
- **pch.cpp/h**: Precompiled header files
- **logging.h**: Logging utilities

## Dependencies
- **IMGUI/**: Dear ImGui library for GUI rendering
  - Core ImGui files (imgui.cpp, imgui.h, etc.)
  - DirectX 11 implementation files
  - Custom controls and additions
- **nlohmann/**: nlohmann/json library
  - **json.hpp**: Single-header JSON library

## Build System
- **Release|x64**: Primary build configuration
- **Output Directory**: `plugins/` (for BakkesMod plugin deployment)
- **Intermediate Directory**: `build/.intermediates/Release/`

## Key Directories (Ignored)
- **plugins/**: Build output (ignored by git)
- **build/**: Build intermediates (ignored by git)
- **.vs/**: Visual Studio files (ignored by git)

## Project Type
- **ConfigurationType**: DynamicLibrary (produces .dll for BakkesMod)
- **Target Platform**: Windows x64
- **Language Standard**: C++20