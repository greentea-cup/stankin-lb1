# Лабораторная СТАНКИН
Программирование 2 семестр Лабораторная 2-3
### Сборка
#### Linux
---
```bash
./regen_cmake.sh
./release.sh
```

#### Windows (если работает)
---
###### Visual Studio
Открыть папку как решение -> выбрать конфигурацию `x64 Release` -> запустить.
###### Вручную
! Команды сборки необходимо запускать из `Developer Command Prompt` или `Developer PowerShell`
64 бит
```cmd
.\regen_cmake.bat
.\build.bat
.\Build\x64-release\app.exe <...>
```
32 бит
```cmd
.\regen_cmake32.bat
.\build32.bat
.\Build\x86-release\app.exe <...>
```
