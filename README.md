# DelAfterReboot
A simple python script that forcibly deletes folders recursively before the system boots.

# DelAfterReboot is now in WinGet!
Now you can install DelAfterReboot with one command,
without downloading the msi file and manually building it.

Just run in cmd: `winget install delafterreboot`

To update, use the same command

# Building

Download delafterreboot.py and icon.ico from repository

Install Python: https://www.python.org/downloads/

Install pyinstaller `pip install pyinstaller`

Run `pyinstaller --onefile --console --icon=icon.ico --name delafterreboot delafterreboot.py`

Done! Now you have delafterreboot.exe in dist\delafterreboot.exe

Move it where you want

# Adding DelAfterReboot to context menu

Download, edit and run install.reg from repository

Or do it manually (in cmd with administrator rights):

```
reg add "HKCR\Directory\shell\DelAfterReboot" /ve /d "DelAfterReboot" /f
reg add "HKCR\Directory\shell\DelAfterReboot" /v "Icon" /d "%ProgramFiles%\DelAfterReboot\delafterreboot.exe,0" /f
reg add "HKCR\Directory\shell\DelAfterReboot\command" /ve /d "\"%ProgramFiles%\DelAfterReboot\delafterreboot.exe\" \"%1\"" /f
```

Replace %ProgramFiles%\DelAfterReboot\ to your real path to delafterreboot.exe

# Add DelAfterReboot folder to PATH (cmd)

Run in cmd with administrator rights:

```
setx /M PATH "%PATH%;%ProgramFiles%\DelAfterReboot"
```

# Uninstalling

Run in cmd with administrator rights:

`reg delete "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /f`

# Compatibility
| System | Status |
| :--- | :--- |
| Windows 7 (Python 3.8) | ✅ Compatible, but only x32 version |
| Windows 10+ (Python 3.12+) | ✅ Compatible |
| Windows 10+ (Python < 3.12, 3.8+) | ✅ Compatible, but only x32 version |
| Windows < 7 (or Python < 3.8) | ❌ Not compatible |
# Risks of using DelAfterReboot
> [!CAUTION]
> Do not use this to delete system folders or folders with a large number of files.
> 
> Files are deleted sequentially. If there are a large number of them, deletion will take a long time.
>
> The creator of the repository is not responsible for your rash or stupid actions. Use at your own risk.
