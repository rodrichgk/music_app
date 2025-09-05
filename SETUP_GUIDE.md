# Qt Music App - Setup Guide for Windows

## Prerequisites Check

First, let's check what you have installed on your system.

### 1. Check Current Installations

Run these commands in PowerShell to check what's already installed:

```powershell
# Check if Qt is installed
qmake --version

# Check if CMake is installed
cmake --version

# Check if Visual Studio Build Tools are available
where cl

# Check if Git is installed
git --version
```

## Installation Steps

### Step 1: Install Qt

**Option A: Qt Online Installer (Recommended)**
1. Download Qt Online Installer from: https://www.qt.io/download-qt-installer
2. Run the installer and create a Qt account (free)
3. Select these components:
   - **Qt 6.5.x** (latest LTS version)
   - **Qt Creator** (IDE)
   - **CMake** (if not already installed)
   - **Ninja** (build system)
   - **Qt Multimedia** (required for audio)

**Option B: Qt via Package Manager (Alternative)**
```powershell
# Install using winget (Windows Package Manager)
winget install --id=TheQtCompany.QtCreator
```

### Step 2: Install Visual Studio Build Tools

Qt on Windows requires MSVC compiler:

1. Download "Build Tools for Visual Studio 2022" from Microsoft
2. Install with these workloads:
   - **C++ build tools**
   - **Windows 10/11 SDK**

**Or install full Visual Studio Community (free):**
```powershell
winget install Microsoft.VisualStudio.2022.Community
```

### Step 3: Install FFmpeg Dependencies

Your project uses FFmpeg libraries. Install via vcpkg (recommended):

```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Install FFmpeg libraries
.\vcpkg install ffmpeg:x64-windows
.\vcpkg install pkg-config:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install
```

### Step 4: Set Environment Variables

Add these to your system PATH:
- `C:\Qt\6.5.x\msvc2022_64\bin` (adjust version as needed)
- `C:\vcpkg\installed\x64-windows\bin`

Set these environment variables:
- `Qt6_DIR=C:\Qt\6.5.x\msvc2022_64\lib\cmake\Qt6`
- `PKG_CONFIG_PATH=C:\vcpkg\installed\x64-windows\lib\pkgconfig`

## Building the Project

### Method 1: Using Qt Creator (Easiest)

1. Open Qt Creator
2. File → Open File or Project
3. Navigate to your project and open `CMakeLists.txt`
4. Configure the project:
   - Select **MSVC 2022** kit
   - Choose **Release** or **Debug** build
5. Click **Configure Project**
6. Build → Build All (Ctrl+Shift+B)
7. Run → Run (Ctrl+R)

### Method 2: Command Line Build

```powershell
# Navigate to project directory
cd C:\Users\kibar\Documents\music_app

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake

# Build the project
cmake --build . --config Release

# Run the executable
.\Release\Music_App.exe
```

## Troubleshooting Common Issues

### Issue 1: Qt Not Found
```
CMake Error: Could not find Qt6
```
**Solution:** Set Qt6_DIR environment variable or specify in cmake:
```powershell
cmake .. -DQt6_DIR="C:\Qt\6.5.x\msvc2022_64\lib\cmake\Qt6"
```

### Issue 2: FFmpeg Libraries Not Found
```
Could not find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
```
**Solution:** Install pkg-config via vcpkg and set PKG_CONFIG_PATH

### Issue 3: MSVC Compiler Not Found
```
No CMAKE_CXX_COMPILER could be found
```
**Solution:** Install Visual Studio Build Tools and run from "Developer Command Prompt"

### Issue 4: Missing DLLs at Runtime
**Solution:** Copy required DLLs to output directory or add to PATH:
- Qt6Core.dll
- Qt6Widgets.dll  
- Qt6Multimedia.dll
- FFmpeg DLLs (avcodec, avformat, etc.)

## Quick Start Script

Save this as `setup.ps1` and run in PowerShell as Administrator:

```powershell
# Quick setup script for Qt Music App
Write-Host "Setting up Qt Music App development environment..."

# Check if running as administrator
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    Write-Host "Please run as Administrator" -ForegroundColor Red
    exit 1
}

# Install winget packages
Write-Host "Installing Qt Creator..."
winget install --id=TheQtCompany.QtCreator --silent

Write-Host "Installing Visual Studio Community..."
winget install Microsoft.VisualStudio.2022.Community --silent

Write-Host "Installing Git..."
winget install --id=Git.Git --silent

Write-Host "Setup complete! Please:"
Write-Host "1. Restart your computer"
Write-Host "2. Install Qt SDK from Qt Creator"
Write-Host "3. Install FFmpeg via vcpkg"
Write-Host "4. Open CMakeLists.txt in Qt Creator"
```

## Alternative: Docker Development Environment

If you prefer a containerized approach:

```dockerfile
# Dockerfile for Qt development
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    qt6-base-dev \
    qt6-multimedia-dev \
    cmake \
    build-essential \
    libavformat-dev \
    libavcodec-dev \
    libavutil-dev \
    libswresample-dev \
    pkg-config

WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make
```

## Next Steps After Setup

1. **Test the build** - Make sure the app compiles and runs
2. **Fix the hard-coded path** in `audioitem.cpp` line 22
3. **Add a test audio file** to the project directory
4. **Review the architecture document** for improvement roadmap

## Getting Help

If you encounter issues:
1. Check the Qt documentation: https://doc.qt.io/
2. Verify all environment variables are set correctly
3. Ensure all dependencies are installed
4. Try building a simple Qt "Hello World" project first

---

**Estimated Setup Time:** 30-60 minutes depending on download speeds
**Disk Space Required:** ~5GB for Qt + Visual Studio + dependencies
