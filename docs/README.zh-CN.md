# Windows Lite Restore（Windows Lite 恢复工具）

一款适用于 Windows 10/11（x64）的原生工具，用于恢复“精简版”（Lite）Windows
镜像通常会禁用或移除的服务、注册表策略、计划任务和可选功能——所有操作都经过
用户确认，并采用 Microsoft 的参考默认值。

除恢复功能外，该应用还提供**优化配置**（游戏、AI、办公），仅禁用或改为“手动”
每种场景下不必要的服务。无论是启用**还是**禁用，每次更改都会经过审查、模拟、
报告和可逆备份。

---

## 功能特性

- 原生图形界面，无任何外部依赖（仅 Win32）。
- 标签页：**功能**、**服务**、**策略**、**任务**、**诊断**和**日志**。
- 以醒目的按钮提供配置选择器（**Microsoft 默认**、**游戏**、**AI**、**办公**）。
- 单项复选框选择，支持搜索、按类别筛选以及快捷操作（“推荐”、“全部待处理”、“清除”）。
- **模拟（dry-run）**，可在应用前查看将要发生的具体更改。
- 应用前创建**系统还原点**和**可逆备份**（服务、策略、任务）。
- **撤销上次运行**（诊断菜单）。
- 文本报告和详细日志。
- 通过 UAC（`runas`）自动提权，并保留当前选择。

### 优化配置

| 配置 | 作用 |
|------|------|
| **Microsoft 默认** | 将服务、策略、任务和功能恢复为完整安装的默认值。 |
| **游戏** | 禁用遥测/报告和后台传递优化；将 SysMain、Windows Search、打印、通知等改为“手动”。 |
| **人工智能** | 与“游戏”相同，并额外禁用 Xbox 服务；为 AI 工作负载释放 CPU/内存/磁盘资源。 |
| **办公** | 仅禁用 Xbox 和遥测；保留打印、搜索和更新。 |

优化配置**绝不会更改系统关键服务**（启动、登录、安全和基础网络）。这些服务会
显示为 **“关键（受保护）”**，无法被禁用，因此任何优化都不会导致 Windows 无法
启动或无法登录。未列入配置的服务会显示为“无更改”。

### 恢复的内容

| 类别 | 示例 |
|------|------|
| 服务 | Windows Update、BITS、Store/AppX、音频、主题、安全、网络 |
| 注册表策略 | UAC、被策略禁用的遥测、服务封锁 |
| 计划任务 | 维护、Windows Update、诊断 |
| 可选功能 | .NET Framework 3.5、媒体/语言功能、工具 |

完整目录位于 `src/catalog.cpp`（包含描述、类别、参考启动类型，以及是否为推荐/
关键/可选项目）。

---

## 系统要求

- Windows 10 或 Windows 11，**64 位**。
- 无需任何运行时、框架或额外库：可执行文件采用静态链接（MinGW-w64），无需安装即可运行。

> 若要**安装已被移除的可选功能**，可能需要相同版本/架构的 Windows 安装介质/ISO
> （源文件夹可在 *设置* 中配置）。

---

## 使用方法

1. 运行 `WindowsLiteRestore.exe`。
2. 如果 Windows 请求提权（UAC），请接受——许多更改都需要管理员权限。未提权时，
   应用以**只读模式**运行。
3. 系统会自动开始分析。通过标签页下方的按钮选择所需的**配置**，并检查已勾选的项目。
4. 可选地勾选**模拟**以在不做任何更改的情况下验证计划。
5. 点击**应用所选项目**（已勾选项）或**应用配置** / **恢复 Microsoft 默认值**
   （配置推荐项）。
6. 确认计划，并选择是否创建还原点和备份。
7. 若要撤销服务/策略/任务，请使用 *诊断 → 撤销上次运行*。

### 命令行选项

| 选项 | 说明 |
|------|------|
| `--scan-report <文件>` | 生成分析报告并退出（无界面模式）。 |
| `--profile <名称>` | 初始配置：`default`/`padrao`、`gaming`/`jogos`、`ai`/`ia`、`office`/`escritorio`。 |
| `--no-elevate` / `--read-only` | 启动时不请求提权（只读）。 |
| `--elevated` | 表示进程已提权（内部使用）。 |
| `--resume-selection <文件>` | 恢复已保存的选择（内部使用）。 |

示例：

```powershell
WindowsLiteRestore.exe --scan-report report.txt --profile gaming
```

---

## 编译

唯一的编译要求是 **MinGW-w64** 编译器（推荐使用
[w64devkit](https://github.com/skeeto/w64devkit)，其中包含 `g++` 和 `windres`）。

```powershell
# 方式 A：设置工具链路径后编译
$env:W64DEVKIT = "C:\path\to\w64devkit"
.\build.ps1

# 方式 B：自动下载工具链（需要网络）
.\build.ps1 -DownloadToolchain
```

`build.ps1` 脚本会：

1. 查找 `g++.exe`/`windres.exe`（通过 `$env:W64DEVKIT`、`tools\w64devkit` 或 `PATH`）。
2. 编译资源（`resources.rc`、清单、图标、版本信息）。
3. 编译 `main.cpp`、`ui.cpp`、`core.cpp` 和 `catalog.cpp`。
4. 静态链接并生成 `dist\WindowsLiteRestore.exe`、`README.md` 和 `SHA256.txt`。

图标可使用 `tools\make-icon.ps1` 重新生成（使用 System.Drawing，无外部依赖）。

### 项目结构

```
src/
  app.h            类型、模型、App 类、模块间的约定
  catalog.cpp      服务、功能、策略和任务的目录
  core.cpp         扫描、应用、备份、撤销、报告、提权、DISM
  ui.cpp           图形界面（原生 Win32）
  main.cpp         入口点（wWinMain）
  resources.rc     对话框、图标、清单、版本信息
  resource.h       资源/命令 ID
  app.manifest     asInvoker、DPI 感知、公共控件 v6
dist/              最终可执行文件 + README + SHA256
tools/             make-icon.ps1
```

---

## 安全性与局限性

- **参考默认值**对应完整的 Windows 10/11 安装。版本、构建和硬件各有差异，请在
  应用前进行审查。
- **优化配置会禁用服务**（遥测、Xbox 等），并将其他服务改为“手动”。它们**不会**
  更改关键或安全服务，并且所有内容都可通过备份/撤销来还原。
- **关键服务受到保护**，永远不会被禁用。
- **缺失的服务不会被“安装”**——精简版镜像可能已移除源组件。这些项目会显示为
  不可用。
- **已被移除的功能**可能需要兼容的介质/ISO（常见 DISM 错误：`0x800F081F`、
  `0x800F0954`、`0x800F0906`——请提供源文件夹）。
- **可选功能不会被“撤销”**，以避免移除可能使系统不稳定的组件。
- **某些服务受到保护**（例如 Windows Update Medic、Defender）。如果更改失败，请
  检查篡改防护设置。
- 本应用为独立工具，**与 Microsoft 无任何关联**。请自担风险使用，最好配合还原点
  和备份。

### 数据存放位置

| 数据 | 位置 |
|------|------|
| 设置 | `%LOCALAPPDATA%\WindowsLiteRestore\settings.ini` |
| 日志（只读） | `%LOCALAPPDATA%\WindowsLiteRestore\logs` |
| 日志（已提权） | `%PROGRAMDATA%\WindowsLiteRestore\logs` |
| 备份 | `%PROGRAMDATA%\WindowsLiteRestore\Backups` |
| 报告 | `%PROGRAMDATA%\WindowsLiteRestore\reports` |

---

## 免责声明

本软件会修改系统设置并安装 Windows 组件。在应用前请务必创建还原点和可逆备份，
并先运行一次模拟以审查计划。
