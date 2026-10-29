# Windows Lite Restore

A native tool for Windows 10/11 (x64) that restores the services, registry
policies, scheduled tasks and optional features that "Lite" Windows images
commonly disable or remove — always with user review and using Microsoft's
reference defaults.

In addition to restoration, the app provides **optimization profiles**
(Gaming, AI, Office) that disable — or set to manual — only the services that are
unnecessary for each scenario. Every change, whether enabling **or** disabling,
goes through review, simulation, report and reversible backup.

---

## Features

- Native GUI with no external dependencies (Win32 only).
- Tabs: **Features**, **Services**, **Policies**, **Tasks**, **Diagnostics** and **Log**.
- Profile selector as prominent buttons (**Microsoft Default**, **Gaming**, **AI**, **Office**).
- Per-item checkbox selection with search, category filter and quick actions
  ("Recommended", "All pending", "Clear").
- **Simulation (dry-run)** to see exactly what will change before applying.
- **System restore point** and **reversible backup** (services, policies, tasks)
  before applying.
- **Undo last run** (Diagnostics menu).
- Text reports and a detailed log.
- Auto-elevation via UAC (`runas`) with the selection preserved.

### Optimization profiles

| Profile | What it does |
|---------|--------------|
| **Microsoft Default** | Restores services, policies, tasks and features to a complete-installation default. |
| **Gaming** | Disables telemetry/reports and background delivery; sets SysMain, Windows Search, printing, notifications, etc. to manual. |
| **Artificial Intelligence** | Same as Gaming, plus disables the Xbox services; frees CPU/RAM/disk for AI workloads. |
| **Office** | Disables only Xbox and telemetry; keeps printing, search and updates. |

Optimization profiles **never change essential system services** (boot, logon,
security and basic networking). Those services appear as **"Essential (protected)"**
and cannot be disabled, so an optimization can never prevent Windows from
booting or logging in. Services not listed in a profile appear as "No change".

### What is restored

| Category | Examples |
|----------|----------|
| Services | Windows Update, BITS, Store/AppX, audio, themes, security, networking |
| Registry policies | UAC, telemetry disabled by policy, service blocks |
| Scheduled tasks | Maintenance, Windows Update, diagnostics |
| Optional features | .NET Framework 3.5, media/language features, tools |

The full catalog lives in `src/catalog.cpp` (with description, category,
reference start type and whether the item is recommended/critical/optional).

---

## Requirements

- Windows 10 or Windows 11, **64-bit**.
- No runtime, framework or additional library: the executable is statically
  linked (MinGW-w64) and runs without installation.

> To **install removed optional features**, a Windows installation media/ISO of
> the same edition/architecture may be required (source folder configurable in
> *Settings*).

---

## Usage

1. Run `WindowsLiteRestore.exe`.
2. If Windows asks for elevation (UAC), accept it — many changes require
   administrator privileges. Without elevation the app runs in **read-only mode**.
3. Analysis runs automatically. Choose the desired **profile** from the buttons
   below the tabs and review the marked items.
4. Optionally tick **Simulate** to validate the plan without changing anything.
5. Click **Apply selected** (checked items) or **Apply profile** /
   **Restore Microsoft defaults** (profile recommendations).
6. Confirm the plan and choose whether to create a restore point and a backup.
7. To revert services/policies/tasks, use *Diagnostics → Undo last run*.

### Command-line options

| Option | Description |
|--------|-------------|
| `--scan-report <file>` | Generates an analysis report and exits (headless). |
| `--profile <name>` | Initial profile: `default`/`padrao`, `gaming`/`jogos`, `ai`/`ia`, `office`/`escritorio`. |
| `--no-elevate` / `--read-only` | Starts without requesting elevation (read-only). |
| `--elevated` | Indicates the process is already elevated (internal use). |
| `--resume-selection <file>` | Restores a saved selection (internal use). |

Example:

```powershell
WindowsLiteRestore.exe --scan-report report.txt --profile gaming
```

---

## Build

The only build requirement is a **MinGW-w64** compiler (recommended
[w64devkit](https://github.com/skeeto/w64devkit), which includes `g++` and
`windres`).

```powershell
# Option A: set the toolchain path and build
$env:W64DEVKIT = "C:\path\to\w64devkit"
.\build.ps1

# Option B: download the toolchain automatically (requires internet)
.\build.ps1 -DownloadToolchain
```

The `build.ps1` script:

1. Locates `g++.exe`/`windres.exe` (via `$env:W64DEVKIT`, `tools\w64devkit` or `PATH`).
2. Compiles the resources (`resources.rc`, manifest, icon, version info).
3. Compiles `main.cpp`, `ui.cpp`, `core.cpp` and `catalog.cpp`.
4. Links statically and writes `dist\WindowsLiteRestore.exe`, the `README.md` and
   the `SHA256.txt`.

The icon can be regenerated with `tools\make-icon.ps1` (uses System.Drawing, no
external dependencies).

### Project structure

```
src/
  app.h            types, model, App class, module contracts
  catalog.cpp      catalog of services, features, policies and tasks
  core.cpp         scan, apply, backup, undo, report, elevation, DISM
  ui.cpp           GUI (native Win32)
  main.cpp         entry point (wWinMain)
  resources.rc     dialogs, icon, manifest, version info
  resource.h       resource/command IDs
  app.manifest     asInvoker, DPI aware, common controls v6
dist/              final executable + README + SHA256
tools/             make-icon.ps1
```

---

## Security and limitations

- **Reference defaults** match a full Windows 10/11 installation. Editions,
  builds and hardware vary; review before applying.
- **Optimization profiles disable services** (telemetry, Xbox, etc.) and set
  others to manual. They **do not** change critical or security services, and
  everything can be reverted via the backup/Undo.
- **Essential services are protected** and can never be disabled.
- **Missing services are not "installed"** by the app — the Lite image may have
  removed the source component. These items appear as unavailable.
- **Removed features** may require compatible media/ISO (common DISM errors:
  `0x800F081F`, `0x800F0954`, `0x800F0906` — provide the source folder).
- **Optional features are not reverted** by "Undo", to avoid removing components
  that could destabilize the system.
- **Some services are protected** (e.g. Windows Update Medic, Defender). If a
  change fails, check tamper protection.
- The app is independent and **not affiliated with Microsoft**. Use at your own
  risk, preferably with a restore point and a backup.

### Data locations

| Data | Location |
|------|----------|
| Settings | `%LOCALAPPDATA%\WindowsLiteRestore\settings.ini` |
| Logs (read-only) | `%LOCALAPPDATA%\WindowsLiteRestore\logs` |
| Logs (elevated) | `%PROGRAMDATA%\WindowsLiteRestore\logs` |
| Backups | `%PROGRAMDATA%\WindowsLiteRestore\Backups` |
| Reports | `%PROGRAMDATA%\WindowsLiteRestore\reports` |

---

## Disclaimer

This software modifies system settings and installs Windows components. Always
create a restore point and a reversible backup before applying, and run a
simulation to review the plan.
