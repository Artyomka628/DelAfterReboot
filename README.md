# DelAfterReboot
A simple C script that forcibly deletes folders recursively before the system boots.

# Building

Install MSYS2: https://www.msys2.org/

Open mingw32.exe and run `pacman -S mingw-w64-i686-gcc`

Copy del.c to (Installation path)\home\\%username%\

Run `i686-w64-mingw32-gcc del.c -o del.exe -std=c99 -O2 -march=i686 -mtune=generic -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 -static-libgcc -static-libstdc++ -fno-asynchronous-unwind-tables`

Done! Now you have del.exe in (Installation path)\home\\%username%\

# Adding DelAfterReboot to context menu

Before doing this, create a variable called "DelAfterRebootPath"

Run `setx DelAfterRebootPath (path to del.exe)`

Download and run install.reg from repository

Or do it manually (in cmd with administrator rights):

```
reg add "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /ve /t REG_SZ /d "DelAfterReboot" /f
reg add "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot" /v HasLUAShield /t REG_SZ /d "" /f
reg add "HKEY_CLASSES_ROOT\Directory\shell\DelAfterReboot\command" /ve /t REG_SZ /d "cmd /c \"%%DelAfterRebootPath%%\del.exe\" \"%%1\"" /f
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
