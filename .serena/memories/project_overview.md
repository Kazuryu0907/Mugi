# Mugi - Project Overview

## Purpose
Mugi is a BakkesMod plugin for Rocket League that provides real-time game data streaming via UDP sockets. The plugin tracks various game events, player statistics, and match information, sending this data to external applications for analysis or overlay purposes.

## Tech Stack
- **Language**: C++ (C++20 standard)
- **Framework**: BakkesMod Plugin SDK
- **Build System**: Visual Studio MSBuild (.vcxproj)
- **Platform**: Windows (x64)
- **Dependencies**:
  - BakkesMod SDK (pluginsdk.lib)
  - nlohmann/json library for JSON serialization
  - Windows Sockets (ws2_32.lib) for UDP communication
  - ImGui (for potential GUI features)

## Architecture
- **Main Class**: `Mugi` - inherits from `BakkesMod::Plugin::BakkesModPlugin`
- **GUI Base**: Includes optional `SettingsWindowBase` and `PluginWindowBase` for UI
- **Networking**: UDP socket communication on ports 12345 and 12344
- **Event System**: Hooks into Rocket League's event system to capture game data

## Key Features
- Real-time game event tracking (goals, saves, demolitions)
- Player statistics monitoring (scores, assists, boost levels)
- Team and match information broadcasting
- Bot and human player differentiation
- Replay analysis support
- JSON-based data serialization for external consumption