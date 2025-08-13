# Task Completion Checklist

## When a Development Task is Completed

### 1. Code Quality Checks
- **Build Verification**: Run `msbuild Mugi.sln /p:Configuration=Release /p:Platform=x64` to ensure clean build
- **No Build Errors**: Check for compilation errors and warnings
- **Code Review**: Ensure code follows established conventions and patterns

### 2. Testing Requirements
- **Manual Testing**: Load plugin in BakkesMod and test functionality in Rocket League
- **Event Verification**: Check that game events are properly captured and transmitted
- **Socket Communication**: Verify UDP messages are sent correctly on ports 12345/12344
- **Log Output**: Review BakkesMod console for proper logging output

### 3. Version Management
- **Version Update**: Version is automatically incremented by PowerShell script during build
- **Version Verification**: Check that version.h reflects correct build number

### 4. Documentation
- **Code Comments**: Add appropriate comments for complex logic
- **Change Documentation**: Update relevant comments for modified functionality

### 5. File Management
- **Clean Build**: Ensure no unnecessary files are left in build directories
- **Plugin Output**: Verify DLL is correctly placed in plugins/ directory

## Notes
- No formal unit tests are available for this BakkesMod plugin project
- Testing must be done manually within the Rocket League/BakkesMod environment
- The build process includes automatic version incrementing via PowerShell script
- Primary validation is through runtime testing and socket communication verification