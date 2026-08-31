# Building from Source

Want to compile OntyTask yourself? It is straightforward and builds with Visual Studio tools into a single lightweight binary (<512 KB).

---

## 1. What You Need

- **Operating System:** Windows 10 or Windows 11 (Windows 7 support is planned).
- **Compiler:** Visual Studio (2022 / 2026) with the **Desktop development with C++** workload (C++20).
- **Project Toolset:** The project is configured with MSVC toolset (`v145`) and Windows 10 SDK. If your Visual Studio setup uses a different SDK or toolset version, simply right-click the project in Visual Studio and select **Retarget solution** or adjust the platform toolset to match your installed version.

---

## 2. Compiling with MSBuild

Open **Developer Command Prompt** or **Developer PowerShell for Visual Studio**, navigate to the OntyTask directory, and run:

```powershell
# Build 64-bit Release version
msbuild OntyTask.vcxproj /p:Configuration=Release /p:Platform=x64
```

Your compiled executable will be placed in:

- `bin/x64/OntyTask.exe`

Just run `OntyTask.exe` - that's all there is to it!
