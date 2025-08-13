# Suggested Commands for Mugi Development

## Build Commands
- **Build Solution**: `msbuild Mugi.sln /p:Configuration=Release /p:Platform=x64`
- **Clean Build**: `msbuild Mugi.sln /t:Clean /p:Configuration=Release /p:Platform=x64`
- **Rebuild**: `msbuild Mugi.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64`

## Development Workflow
- **Open in Visual Studio**: `start Mugi.sln`
- **Version Update**: Automatic via PowerShell pre-build event (`update_version.ps1`)
- **Output Location**: Built DLL goes to `plugins/` directory

## Git Commands
- **Status**: `git status`
- **Add Changes**: `git add .`
- **Commit**: `git commit -m "message"`
- **Push**: `git push origin main`
- **View Log**: `git log --oneline`

## Windows System Commands
- **List Files**: `dir` or `ls` (if using Git Bash/WSL)
- **Navigate**: `cd path`
- **Find Files**: `dir /s filename` or `find . -name "filename"`
- **View File**: `type filename` or `cat filename`

## BakkesMod Development
- **Plugin Location**: Plugins are loaded from BakkesMod installation directory
- **Debug**: Use cvarManager->log() outputs visible in BakkesMod console
- **Testing**: Test in Rocket League with BakkesMod loaded

## PowerShell Commands
- **Execute Script**: `powershell.exe -ExecutionPolicy Bypass -File script.ps1`
- **Version Script**: Automatically increments VERSION_BUILD in version.h