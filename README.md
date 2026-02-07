# DelAfterReboot

A simple program that forcibly deletes folders recursively before the system boots.

---

## Quick install

DelAfterReboot is available in WinGet:

```cmd
winget install delafterreboot
```

Use the same command to update.

## Verify installer provenance (attestations)

To verify attestation produced by the CI:

```bash
gh attestation verify (path to msi) --repo Artyomka628/DelAfterReboot --verbose
```

Only installers from version 3.0.0 and later are supported.

---

## Local build

These steps describe how to build the executable and MSI locally on Windows. The build process is the same for all architectures (x86, x64, arm64).

### Prerequisites

You will need:
- Git (to clone the repository and get commit hash)
- MSYS2 with appropriate mingw packages (for x86/x64) OR llvm-mingw (for arm64)
- WiX Toolset (to create MSI packages)

### Step 1: Clone repository

```powershell
git clone https://github.com/Artyomka628/DelAfterReboot.git
cd DelAfterReboot
```

### Step 2: Generate version header

```powershell
$version = "unknown"
$commit = "unknown"
$date = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")

if (Test-Path "release.json") {
  try {
    $json = Get-Content release.json -Raw | ConvertFrom-Json
    if ($json.version) { $version = $json.version }
  } catch {}
}

try { $commit = git rev-parse --short HEAD } catch {}

$content = @()
$content += '#pragma once'
$content += "#define APP_VERSION `"$version`""
$content += "#define GIT_COMMIT_HASH `"$commit`""
$content += "#define BUILD_DATE `"$date`""
$content -join "`n" | Out-File -FilePath version.h -Encoding utf8
```

### Step 3: Build executable

#### For x86 (32-bit)

1. Install MSYS2 from https://www.msys2.org/
2. Install mingw-w64 for i686:
```powershell
# Run in PowerShell or Command Prompt
msys2 -c "pacman -Syuu --noconfirm"
msys2 -c "pacman -S mingw-w64-i686-gcc --noconfirm"
```
3. Build in MINGW32 shell:
```bash
rm -rf dist
mkdir -p dist
g++ -std=c++17 -O2 -s -static -static-libgcc -static-libstdc++ -municode delafterreboot.cpp -o dist/delafterreboot.exe -ladvapi32 -lshell32 -luser32
```

#### For x64 (64-bit)

1. Install MSYS2 from https://www.msys2.org/
2. Install mingw-w64 for x86_64:
```powershell
# Run in PowerShell or Command Prompt
msys2 -c "pacman -Syuu --noconfirm"
msys2 -c "pacman -S mingw-w64-x86_64-gcc --noconfirm"
```
3. Build in MINGW64 shell:
```bash
rm -rf dist
mkdir -p dist
g++ -std=c++17 -O2 -s -static -static-libgcc -static-libstdc++ -municode delafterreboot.cpp -o dist/delafterreboot.exe -ladvapi32 -lshell32 -luser32
```

#### For ARM64 (64-bit ARM)

1. Download llvm-mingw for aarch64 from [here](https://github.com/mstorsjo/llvm-mingw/releases)
2. Extract the archive and add the `bin` folder to your PATH
3. Build:
```powershell
rm -Force -Recurse dist -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path dist -Force | Out-Null
aarch64-w64-mingw32-clang++ -std=c++17 -O2 -s -static -static-libgcc -static-libstdc++ -municode delafterreboot.cpp -o dist/delafterreboot.exe -ladvapi32 -lshell32 -luser32
```

After building, you should have `dist\delafterreboot.exe`.

---

# Build MSI installer

MSI creation requires WiX Toolset and the provided `.wxs` files.

#### Install WiX Toolset

Via Chocolatey (recommended):
```powershell
choco install wixtoolset -y
```

Or download from https://wixtoolset.org/

#### Prepare installer directory

```powershell
New-Item -ItemType Directory -Path installer -Force | Out-Null
Move-Item -Path "dist\delafterreboot.exe" -Destination "installer\delafterreboot.exe" -Force
```

#### Build MSI

```powershell
$wixPath = "C:\Program Files (x86)\WiX Toolset v3.14\bin"
if (-not (Test-Path $wixPath)) {
  $wixPath = "C:\Program Files (x86)\WiX Toolset v3.11\bin"
}

$arch = "x64"  # Use "x86", "x64", or "arm64" depending on what you built

& "$wixPath\candle.exe" "installer\Product-$arch.wxs" -ext WixUIExtension -dDelExePath="installer\delafterreboot.exe" -out "installer\$arch.wixobj"
& "$wixPath\light.exe" "installer\$arch.wixobj" -ext WixUIExtension -o "installer\DelAfterReboot-$arch.msi" -b installer
```

You should now have `installer\DelAfterReboot-<arch>.msi`.

---

## release.json format

To include version information in the build, create a `release.json` file in the repository root:

```json
{
  "version": "1.2.3",
  "notes": [
    "Unless you are making releases to your fork using build.yml, you can ignore the notes value."
  ]
}
```

---

## Context menu install (Windows)

Download and edit `install.reg` from the repository or run (Admin CMD):

```cmd
reg add "HKCR\Directory\shell\DelAfterReboot" /ve /d "DelAfterReboot" /f
reg add "HKCR\Directory\shell\DelAfterReboot" /v "Icon" /d "%ProgramFiles%\DelAfterReboot\delafterreboot.exe,0" /f
reg add "HKCR\Directory\shell\DelAfterReboot\command" /ve /d "\"%ProgramFiles%\DelAfterReboot\delafterreboot.exe\" \"%1\"" /f
```

---

## Add install folder to PATH (Admin CMD)

```cmd
setx /M PATH "%PATH%;%ProgramFiles%\DelAfterReboot"
```

---

## Uninstall context menu entry

```cmd
reg delete "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /f
```

---

## Compatibility

Thanks to the move to C++, the minimum operating system for DelAfterReboot is now Windows Vista

[More information](https://github.com/Artyomka628/DelAfterReboot/wiki/Moving-to-Cpp)

---

## Risks

> [!CAUTION]
> Do not use this to delete system folders or folders with a very large number of files.
> Deletion happens sequentially and may take significant time. The author is not responsible for misuse. Use at your own risk.

---