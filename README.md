# DelAfterReboot
A simple python script that forcibly deletes folders recursively before the system boots.

# Building

Download del.py and icon.ico from repository

Install Python: https://www.python.org/downloads/

Install pyinstaller `pip install pyinstaller`

Run `pyinstaller --onefile --console --icon=icon.ico del.py`

Done! Now you have del.exe in dist\del.exe

Move it where you want

# Adding DelAfterReboot to context menu

Download, edit and run install.reg from repository

Or do it manually (in cmd with administrator rights):

```
reg add "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /ve /t REG_SZ /d "DelAfterReboot" /f
reg add "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /v HasLUAShield /t REG_SZ /d "" /f
reg add "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot\command" /ve /t REG_SZ /d "cmd /c \"(PATH TO DEL.EXE)\" \"%%1\"" /f
```

# Uninstalling

Run in cmd with administrator rights:

`reg delete "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /f`

# Risks of using DelAfterReboot
> [!CAUTION]
> Do not use this to delete system folders or folders with a large number of files.
> 
> Files are deleted sequentially. If there are a large number of them, deletion will take a long time.
>
> The creator of the repository is not responsible for your rash or stupid actions. Use at your own risk.
