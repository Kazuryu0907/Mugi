# Code Style and Conventions

## Naming Conventions
- **Classes**: PascalCase (e.g., `Mugi`, `SettingsWindowBase`)
- **Methods**: camelCase (e.g., `onLoad`, `sendSocket`, `createNameTable`)
- **Variables**: camelCase with descriptive names
- **Private Members**: camelCase (e.g., `isBoostWatching`, `preMatchId`)
- **Structs**: camelCase with s_ prefix for some (e.g., `s_currentSetPoint`)
- **Constants**: UPPERCASE (e.g., `PORT`, `ADDR`)

## Code Structure
- **Headers**: Use `#pragma once` for include guards
- **Includes**: System headers first, then local headers
- **Namespaces**: Using declarations (e.g., `using json = nlohmann::json`)
- **Comments**: Mix of English and Japanese comments
- **Macros**: Use for convenience (e.g., `#define TOS(i) std::to_string(i)`)

## C++ Standards
- **Version**: C++20 standard enabled
- **Features**: Modern C++ with auto, lambdas, range-based for loops
- **Memory Management**: Smart pointers used (std::shared_ptr)
- **STL Containers**: Extensive use of std::unordered_map, std::vector

## BakkesMod Patterns
- **Event Hooks**: Extensive use of gameWrapper->HookEvent patterns
- **Logging**: cvarManager->log() for debug output
- **Wrappers**: Use of BakkesMod wrapper classes (ServerWrapper, CarWrapper, etc.)

## JSON Usage
- nlohmann::json library for all JSON operations
- Consistent structure with "cmd" and "data" fields
- JSON objects used for structured data transmission