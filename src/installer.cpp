#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <windowsx.h>

#include <string>
#include <vector>
#include <cwchar>

#include "i18n.h"
#include "resource.h"

// ---------------------------------------------------------------------------
// IDs de recursos embutidos e de controles
// ---------------------------------------------------------------------------
#define IDR_APP_EXE 101
#define IDR_README 102
#define IDR_DOC_EN 200
#define IDR_DOC_ES 201
#define IDR_DOC_FR 202
#define IDR_DOC_ZH 203
#define IDR_DOC_HI 204

#define IDC_LANG 1001
#define IDC_DEST 1002
#define IDC_BROWSE 1003
#define IDC_STARTMENU 1004
#define IDC_DESKTOP 1005
#define IDC_INSTALL 1006
#define IDC_CANCEL 1007
#define IDC_STATUS 1008
#define IDC_APPICON_STATIC 1009
#define IDC_TITLE 1010
#define IDC_SUBTITLE 1011

// ---------------------------------------------------------------------------
// Strings específicas do instalador (6 idiomas)
// ---------------------------------------------------------------------------
enum InstStrId {
    INS_TITLE = 0,
    INS_SUBTITLE,
    INS_LANG_LABEL,
    INS_DEST_LABEL,
    INS_BROWSE,
    INS_STARTMENU,
    INS_DESKTOP,
    INS_INSTALL,
    INS_CANCEL,
    INS_INSTALLING,
    INS_DONE_TITLE,
    INS_DONE_TEXT,
    INS_RUN,
    INS_FAILED,
    INS_UNINSTALL_TITLE,
    INS_UNINSTALL_CONFIRM,
    INS_UNINSTALLING,
    INS_UNINSTALL_DONE,
    INS_COUNT
};

namespace {

const wchar_t* const g_inst[kLanguageCount][INS_COUNT] = {
    // Português
    {L"Windows Lite Restore — Instalador",
     L"Instala o aplicativo no seu computador.",
     L"Idioma de instalação:",
     L"Pasta de destino:",
     L"Procurar...",
     L"Criar atalho no menu Iniciar",
     L"Criar atalho na área de trabalho",
     L"Instalar",
     L"Cancelar",
     L"Instalando...",
     L"Instalação concluída",
     L"O Windows Lite Restore foi instalado com sucesso.",
     L"Executar o Windows Lite Restore agora?",
     L"Falha na instalação.",
     L"Desinstalar Windows Lite Restore",
     L"Remover o Windows Lite Restore deste computador?",
     L"Removendo...",
     L"Remoção concluída."},
    // English
    {L"Windows Lite Restore — Installer",
     L"Installs the application on your computer.",
     L"Installation language:",
     L"Destination folder:",
     L"Browse...",
     L"Create Start menu shortcut",
     L"Create desktop shortcut",
     L"Install",
     L"Cancel",
     L"Installing...",
     L"Installation complete",
     L"Windows Lite Restore has been installed successfully.",
     L"Run Windows Lite Restore now?",
     L"Installation failed.",
     L"Uninstall Windows Lite Restore",
     L"Remove Windows Lite Restore from this computer?",
     L"Removing...",
     L"Removal complete."},
    // Español
    {L"Windows Lite Restore — Instalador",
     L"Instala la aplicación en su equipo.",
     L"Idioma de instalación:",
     L"Carpeta de destino:",
     L"Examinar...",
     L"Crear acceso directo en el menú Inicio",
     L"Crear acceso directo en el escritorio",
     L"Instalar",
     L"Cancelar",
     L"Instalando...",
     L"Instalación completada",
     L"Windows Lite Restore se instaló correctamente.",
     L"¿Ejecutar Windows Lite Restore ahora?",
     L"Falló la instalación.",
     L"Desinstalar Windows Lite Restore",
     L"¿Eliminar Windows Lite Restore de este equipo?",
     L"Eliminando...",
     L"Eliminación completada."},
    // Français
    {L"Windows Lite Restore — Programme d'installation",
     L"Installe l'application sur votre ordinateur.",
     L"Langue d'installation :",
     L"Dossier de destination :",
     L"Parcourir...",
     L"Créer un raccourci dans le menu Démarrer",
     L"Créer un raccourci sur le bureau",
     L"Installer",
     L"Annuler",
     L"Installation...",
     L"Installation terminée",
     L"Windows Lite Restore a été installé avec succès.",
     L"Exécuter Windows Lite Restore maintenant ?",
     L"Échec de l'installation.",
     L"Désinstaller Windows Lite Restore",
     L"Supprimer Windows Lite Restore de cet ordinateur ?",
     L"Suppression...",
     L"Suppression terminée."},
    // 简体中文
    {L"Windows Lite Restore — 安装程序",
     L"在您的计算机上安装该应用程序。",
     L"安装语言：",
     L"目标文件夹：",
     L"浏览...",
     L"在开始菜单创建快捷方式",
     L"在桌面创建快捷方式",
     L"安装",
     L"取消",
     L"正在安装...",
     L"安装完成",
     L"Windows Lite Restore 已成功安装。",
     L"是否立即运行 Windows Lite Restore？",
     L"安装失败。",
     L"卸载 Windows Lite Restore",
     L"是否从此计算机移除 Windows Lite Restore？",
     L"正在移除...",
     L"移除完成。"},
    // हिन्दी
    {L"Windows Lite Restore — इंस्टॉलर",
     L"आपके कंप्यूटर पर ऐप इंस्टॉल करता है।",
     L"इंस्टॉलेशन भाषा:",
     L"गंतव्य फ़ोल्डर:",
     L"ब्राउज़...",
     L"स्टार्ट मेनू में शॉर्टकट बनाएँ",
     L"डेस्कटॉप पर शॉर्टकट बनाएँ",
     L"इंस्टॉल करें",
     L"रद्द करें",
     L"इंस्टॉल हो रहा है...",
     L"इंस्टॉलेशन पूर्ण",
     L"Windows Lite Restore सफलतापूर्वक इंस्टॉल हो गया।",
     L"अभी Windows Lite Restore चलाएँ?",
     L"इंस्टॉलेशन विफल।",
     L"Windows Lite Restore अनइंस्टॉल करें",
     L"इस कंप्यूटर से Windows Lite Restore हटाएँ?",
     L"हटाया जा रहा है...",
     L"निष्कासन पूर्ण।"},
};

const wchar_t* InstStr(int id) {
    int lang = static_cast<int>(GetUiLanguage());
    if (lang < 0 || lang >= kLanguageCount) lang = 0;
    if (id < 0 || id >= INS_COUNT) return L"";
    return g_inst[lang][id];
}

HINSTANCE g_instance = nullptr;
HFONT g_fontUi = nullptr;
HFONT g_fontTitle = nullptr;
HBRUSH g_brushWhite = nullptr;
HWND g_window = nullptr;
HWND g_langCombo = nullptr;
HWND g_destEdit = nullptr;
HWND g_startMenu = nullptr;
HWND g_desktop = nullptr;
HWND g_status = nullptr;
std::wstring g_destDir;
int g_dpi = 96;

int S(int value) { return MulDiv(value, g_dpi, 96); }

// ---------------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------------

std::wstring GetFolderPath(int csidl) {
    wchar_t path[MAX_PATH] = {};
    if (SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, path) == S_OK) {
        return path;
    }
    return std::wstring();
}

bool EnsureDirectory(const std::wstring& path) {
    if (path.empty()) return false;
    std::wstring current;
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start <= path.size()) {
        size_t pos = path.find(L'\\', start);
        std::wstring part = path.substr(start, pos == std::wstring::npos ? std::wstring::npos : pos - start);
        if (!part.empty()) parts.push_back(part);
        if (pos == std::wstring::npos) break;
        start = pos + 1;
    }
    for (const std::wstring& part : parts) {
        if (!current.empty()) current += L'\\';
        if (current.empty() && part.size() == 2 && part[1] == L':') {
            current = part + L'\\';
            continue;
        }
        current += part;
        CreateDirectoryW(current.c_str(), nullptr);
    }
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool ExtractResource(int id, const std::wstring& destPath) {
    HRSRC res = FindResourceW(g_instance, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!res) return false;
    HGLOBAL handle = LoadResource(g_instance, res);
    if (!handle) return false;
    void* data = LockResource(handle);
    DWORD size = SizeofResource(g_instance, res);
    if (!data || !size) return false;

    HANDLE file = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(file, data, size, &written, nullptr);
    CloseHandle(file);
    return ok && written == size;
}

bool CreateShortcut(const std::wstring& lnkPath, const std::wstring& target,
                    const std::wstring& args, const std::wstring& description) {
    IShellLinkW* link = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IShellLinkW, reinterpret_cast<void**>(&link));
    if (FAILED(hr) || !link) return false;
    link->SetPath(target.c_str());
    if (!args.empty()) link->SetArguments(args.c_str());
    if (!description.empty()) link->SetDescription(description.c_str());
    IPersistFile* file = nullptr;
    hr = link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file));
    if (SUCCEEDED(hr) && file) {
        hr = file->Save(lnkPath.c_str(), TRUE);
        file->Release();
    }
    link->Release();
    return SUCCEEDED(hr);
}

bool DeleteFileIfExists(const std::wstring& path) {
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return true;
    return DeleteFileW(path.c_str()) != FALSE;
}

void SetStatusText(const wchar_t* text) {
    if (g_status) SetWindowTextW(g_status, text);
}

// ---------------------------------------------------------------------------
// Instalação
// ---------------------------------------------------------------------------

bool WriteUninstallRegistry(const std::wstring& installDir, const std::wstring& uninstaller) {
    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WindowsLiteRestore", 0,
        nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
    if (result != ERROR_SUCCESS) return false;
    std::wstring uninstallString = L"\"" + uninstaller + L"\" /uninstall";
    std::wstring appPath = installDir + L"\\WindowsLiteRestore.exe";
    RegSetValueExW(key, L"DisplayName", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(L"Windows Lite Restore"),
                   static_cast<DWORD>((wcslen(L"Windows Lite Restore") + 1) * sizeof(wchar_t)));
    RegSetValueExW(key, L"DisplayVersion", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(L"1.0.0"),
                   static_cast<DWORD>((wcslen(L"1.0.0") + 1) * sizeof(wchar_t)));
    RegSetValueExW(key, L"Publisher", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(L"Projeto independente"),
                   static_cast<DWORD>((wcslen(L"Projeto independente") + 1) * sizeof(wchar_t)));
    RegSetValueExW(key, L"InstallLocation", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(installDir.c_str()),
                   static_cast<DWORD>((installDir.size() + 1) * sizeof(wchar_t)));
    RegSetValueExW(key, L"UninstallString", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(uninstallString.c_str()),
                   static_cast<DWORD>((uninstallString.size() + 1) * sizeof(wchar_t)));
    RegSetValueExW(key, L"DisplayIcon", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(appPath.c_str()),
                   static_cast<DWORD>((appPath.size() + 1) * sizeof(wchar_t)));
    DWORD one = 1;
    RegSetValueExW(key, L"NoModify", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&one), sizeof(one));
    RegSetValueExW(key, L"NoRepair", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&one), sizeof(one));
    RegCloseKey(key);
    return true;
}

bool DoInstall(const std::wstring& installDir, bool startMenu, bool desktop) {
    SetStatusText(InstStr(INS_INSTALLING));

    wchar_t installerPath[MAX_PATH] = {};
    GetModuleFileNameW(g_instance, installerPath, MAX_PATH);
    std::wstring uninstaller = installDir + L"\\uninstall.exe";

    if (!EnsureDirectory(installDir)) return false;
    std::wstring docsDir = installDir + L"\\docs";
    EnsureDirectory(docsDir);

    if (!ExtractResource(IDR_APP_EXE, installDir + L"\\WindowsLiteRestore.exe")) return false;
    ExtractResource(IDR_README, installDir + L"\\README.md");
    ExtractResource(IDR_DOC_EN, docsDir + L"\\README.en.md");
    ExtractResource(IDR_DOC_ES, docsDir + L"\\README.es.md");
    ExtractResource(IDR_DOC_FR, docsDir + L"\\README.fr.md");
    ExtractResource(IDR_DOC_ZH, docsDir + L"\\README.zh-CN.md");
    ExtractResource(IDR_DOC_HI, docsDir + L"\\README.hi.md");

    // Copia o próprio instalador como desinstalador.
    CopyFileW(installerPath, uninstaller.c_str(), FALSE);

    std::wstring appExe = installDir + L"\\WindowsLiteRestore.exe";
    std::wstring programs = GetFolderPath(CSIDL_COMMON_PROGRAMS);

    if (startMenu && !programs.empty()) {
        CreateShortcut(programs + L"\\Windows Lite Restore.lnk", appExe, L"",
                       L"Restaurador Windows Lite");
        CreateShortcut(programs + L"\\Desinstalar Windows Lite Restore.lnk", uninstaller,
                       L"/uninstall", L"Desinstalar Windows Lite Restore");
    }
    if (desktop) {
        std::wstring desk = GetFolderPath(CSIDL_COMMON_DESKTOPDIRECTORY);
        if (!desk.empty()) {
            CreateShortcut(desk + L"\\Windows Lite Restore.lnk", appExe, L"",
                           L"Restaurador Windows Lite");
        }
    }

    if (!WriteUninstallRegistry(installDir, uninstaller)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Desinstalação
// ---------------------------------------------------------------------------

void DoUninstall() {
    SetStatusText(InstStr(INS_UNINSTALLING));

    wchar_t installerPath[MAX_PATH] = {};
    GetModuleFileNameW(g_instance, installerPath, MAX_PATH);
    std::wstring self = installerPath;

    // Obtém a pasta de instalação a partir do caminho do desinstalador.
    std::wstring installDir = self;
    size_t pos = installDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) installDir.resize(pos);

    std::wstring programs = GetFolderPath(CSIDL_COMMON_PROGRAMS);
    if (!programs.empty()) {
        DeleteFileIfExists(programs + L"\\Windows Lite Restore.lnk");
        DeleteFileIfExists(programs + L"\\Desinstalar Windows Lite Restore.lnk");
    }
    std::wstring desk = GetFolderPath(CSIDL_COMMON_DESKTOPDIRECTORY);
    if (!desk.empty()) {
        DeleteFileIfExists(desk + L"\\Windows Lite Restore.lnk");
    }

    // Remove os arquivos instalados (menos o próprio desinstalador em execução).
    DeleteFileIfExists(installDir + L"\\WindowsLiteRestore.exe");
    DeleteFileIfExists(installDir + L"\\README.md");
    std::wstring docsDir = installDir + L"\\docs";
    DeleteFileIfExists(docsDir + L"\\README.en.md");
    DeleteFileIfExists(docsDir + L"\\README.es.md");
    DeleteFileIfExists(docsDir + L"\\README.fr.md");
    DeleteFileIfExists(docsDir + L"\\README.zh-CN.md");
    DeleteFileIfExists(docsDir + L"\\README.hi.md");
    RemoveDirectoryW(docsDir.c_str());

    RegDeleteTreeW(HKEY_LOCAL_MACHINE,
                   L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WindowsLiteRestore");

    // Agenda a remoção do próprio desinstalador e da pasta na próxima reinicialização.
    MoveFileExW(self.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    RemoveDirectoryW(installDir.c_str());
}

// ---------------------------------------------------------------------------
// Janela do instalador
// ---------------------------------------------------------------------------

void ApplyLanguageToControls() {
    SetWindowTextW(g_window, InstStr(INS_TITLE));
    SetWindowTextW(GetDlgItem(g_window, IDC_TITLE), InstStr(INS_TITLE));
    SetWindowTextW(GetDlgItem(g_window, IDC_SUBTITLE), InstStr(INS_SUBTITLE));
    SetWindowTextW(GetDlgItem(g_window, IDC_LANG), InstStr(INS_LANG_LABEL));
    SetWindowTextW(GetDlgItem(g_window, IDC_DEST), InstStr(INS_DEST_LABEL));
    SetWindowTextW(GetDlgItem(g_window, IDC_BROWSE), InstStr(INS_BROWSE));
    SetWindowTextW(GetDlgItem(g_window, IDC_STARTMENU), InstStr(INS_STARTMENU));
    SetWindowTextW(GetDlgItem(g_window, IDC_DESKTOP), InstStr(INS_DESKTOP));
    SetWindowTextW(GetDlgItem(g_window, IDC_INSTALL), InstStr(INS_INSTALL));
    SetWindowTextW(GetDlgItem(g_window, IDC_CANCEL), InstStr(INS_CANCEL));
    ComboBox_SetCurSel(g_langCombo, static_cast<int>(GetUiLanguage()));
}

LRESULT CALLBACK InstallerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LANG:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int index = ComboBox_GetCurSel(g_langCombo);
                        if (index >= 0 && index < kLanguageCount) {
                            SetUiLanguage(static_cast<Language>(index));
                            ApplyLanguageToControls();
                        }
                    }
                    return 0;
                case IDC_BROWSE: {
                    std::wstring chosen;
                    BROWSEINFOW browse = {};
                    browse.hwndOwner = hwnd;
                    browse.lpszTitle = InstStr(INS_DEST_LABEL);
                    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
                    LPITEMIDLIST idList = SHBrowseForFolderW(&browse);
                    if (idList) {
                        wchar_t buf[MAX_PATH] = {};
                        if (SHGetPathFromIDListW(idList, buf)) {
                            SetWindowTextW(g_destEdit, buf);
                        }
                        CoTaskMemFree(idList);
                    }
                    return 0;
                }
                case IDC_CANCEL:
                    DestroyWindow(hwnd);
                    return 0;
                case IDC_INSTALL: {
                    wchar_t dest[MAX_PATH] = {};
                    GetWindowTextW(g_destEdit, dest, MAX_PATH);
                    g_destDir = dest;
                    if (g_destDir.empty()) {
                        MessageBoxW(hwnd, InstStr(INS_FAILED), InstStr(INS_TITLE),
                                    MB_ICONERROR | MB_OK);
                        return 0;
                    }
                    EnableWindow(g_langCombo, FALSE);
                    EnableWindow(g_destEdit, FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
                    EnableWindow(g_startMenu, FALSE);
                    EnableWindow(g_desktop, FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_INSTALL), FALSE);
                    bool startMenu = SendMessageW(g_startMenu, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    bool desktop = SendMessageW(g_desktop, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    bool ok = DoInstall(g_destDir, startMenu, desktop);
                    if (ok) {
                        int answer = MessageBoxW(
                            hwnd, InstStr(INS_RUN), InstStr(INS_DONE_TITLE),
                            MB_ICONINFORMATION | MB_YESNO);
                        if (answer == IDYES) {
                            ShellExecuteW(nullptr, L"open",
                                          (g_destDir + L"\\WindowsLiteRestore.exe").c_str(),
                                          nullptr, nullptr, SW_SHOWNORMAL);
                        }
                    } else {
                        MessageBoxW(hwnd, InstStr(INS_FAILED), InstStr(INS_TITLE),
                                    MB_ICONERROR | MB_OK);
                    }
                    DestroyWindow(hwnd);
                    return 0;
                }
            }
            return 0;
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkMode(dc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(g_brushWhite);
        }
        case WM_CTLCOLORBTN:
            return reinterpret_cast<LRESULT>(g_brushWhite);
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(g_brushWhite);
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int RunInstaller() {
    g_dpi = 96;
    HDC screen = GetDC(nullptr);
    if (screen) {
        g_dpi = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(nullptr, screen);
    }
    if (g_dpi <= 0) g_dpi = 96;

    g_fontUi = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_fontTitle = CreateFontW(-MulDiv(14, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_brushWhite = CreateSolidBrush(RGB(255, 255, 255));

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = InstallerWndProc;
    wc.hInstance = g_instance;
    wc.hIcon = LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = g_brushWhite;
    wc.lpszClassName = L"WLRInstallerWindow";
    RegisterClassExW(&wc);

    g_window = CreateWindowExW(0, L"WLRInstallerWindow", InstStr(INS_TITLE),
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
                               CW_USEDEFAULT, CW_USEDEFAULT, S(640), S(400), nullptr, nullptr,
                               g_instance, nullptr);
    if (!g_window) return 1;

    // Centraliza.
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    RECT r = {};
    GetWindowRect(g_window, &r);
    int w = r.right - r.left;
    int h = r.bottom - r.top;
    SetWindowPos(g_window, nullptr, work.left + (work.right - work.left - w) / 2,
                 work.top + (work.bottom - work.top - h) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    HWND icon = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ICON, S(20), S(16),
                                S(32), S(32), g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_APPICON_STATIC)),
                                g_instance, nullptr);
    SendMessageW(icon, STM_SETICON,
                 reinterpret_cast<WPARAM>(LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_APPICON))), 0);

    HWND title = CreateWindowExW(0, L"STATIC", InstStr(INS_TITLE),
                                 WS_CHILD | WS_VISIBLE | SS_LEFT, S(64), S(14), S(540), S(26),
                                 g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TITLE)),
                                 g_instance, nullptr);
    SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontTitle), TRUE);

    HWND subtitle = CreateWindowExW(0, L"STATIC", InstStr(INS_SUBTITLE),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT, S(64), S(42), S(540), S(20),
                                    g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SUBTITLE)),
                                    g_instance, nullptr);
    SendMessageW(subtitle, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    HWND langLabel = CreateWindowExW(0, L"STATIC", InstStr(INS_LANG_LABEL),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT, S(20), S(92), S(160), S(20),
                                     g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LANG)),
                                     g_instance, nullptr);
    SendMessageW(langLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    g_langCombo = CreateWindowExW(0, WC_COMBOBOXW, nullptr,
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                  S(190), S(88), S(240), S(300), g_window,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LANG)),
                                  g_instance, nullptr);
    SendMessageW(g_langCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
    for (int i = 0; i < kLanguageCount; ++i) {
        SendMessageW(g_langCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(LanguageName(static_cast<Language>(i))));
    }
    ComboBox_SetCurSel(g_langCombo, static_cast<int>(GetUiLanguage()));

    HWND destLabel = CreateWindowExW(0, L"STATIC", InstStr(INS_DEST_LABEL),
                                     WS_CHILD | WS_VISIBLE | SS_LEFT, S(20), S(128), S(160), S(20),
                                     g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEST)),
                                     g_instance, nullptr);
    SendMessageW(destLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    g_destEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, S(190), S(124),
                                 S(320), S(22), g_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEST)),
                                 g_instance, nullptr);
    SendMessageW(g_destEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    // Pasta de destino padrão: Program Files.
    std::wstring defaultDest = GetFolderPath(CSIDL_PROGRAM_FILES) + L"\\WindowsLiteRestore";
    SetWindowTextW(g_destEdit, defaultDest.c_str());

    HWND browse = CreateWindowExW(0, L"BUTTON", InstStr(INS_BROWSE),
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, S(520), S(123),
                                  S(100), S(24), g_window,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BROWSE)),
                                  g_instance, nullptr);
    SendMessageW(browse, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    g_startMenu = CreateWindowExW(0, L"BUTTON", InstStr(INS_STARTMENU),
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, S(20), S(170),
                                  S(360), S(22), g_window,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STARTMENU)),
                                  g_instance, nullptr);
    SendMessageW(g_startMenu, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
    SendMessageW(g_startMenu, BM_SETCHECK, BST_CHECKED, 0);

    g_desktop = CreateWindowExW(0, L"BUTTON", InstStr(INS_DESKTOP),
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, S(20), S(198),
                                S(360), S(22), g_window,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DESKTOP)),
                                g_instance, nullptr);
    SendMessageW(g_desktop, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    g_status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, S(20), S(236),
                               S(580), S(40), g_window,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS)),
                               g_instance, nullptr);
    SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    HWND cancel = CreateWindowExW(0, L"BUTTON", InstStr(INS_CANCEL),
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, S(420), S(340),
                                  S(90), S(32), g_window,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CANCEL)),
                                  g_instance, nullptr);
    SendMessageW(cancel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    HWND install = CreateWindowExW(0, L"BUTTON", InstStr(INS_INSTALL),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, S(520),
                                   S(340), S(100), S(32), g_window,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSTALL)),
                                   g_instance, nullptr);
    SendMessageW(install, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    ShowWindow(g_window, SW_SHOWNORMAL);
    UpdateWindow(g_window);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(g_window, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

int RunUninstall() {
    g_fontUi = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_brushWhite = CreateSolidBrush(RGB(255, 255, 255));

    int answer = MessageBoxW(nullptr, InstStr(INS_UNINSTALL_CONFIRM), InstStr(INS_UNINSTALL_TITLE),
                             MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
    if (answer != IDYES) return 0;

    DoUninstall();
    MessageBoxW(nullptr, InstStr(INS_UNINSTALL_DONE), InstStr(INS_UNINSTALL_TITLE),
                MB_ICONINFORMATION | MB_OK);
    return 0;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int) {
    g_instance = instance;
    INITCOMMONCONTROLSEX controls = {};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controls);

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    bool uninstall = false;
    if (commandLine) {
        std::wstring args = commandLine;
        if (args.find(L"/uninstall") != std::wstring::npos ||
            args.find(L"-uninstall") != std::wstring::npos) {
            uninstall = true;
        }
    }

    int result = uninstall ? RunUninstall() : RunInstaller();

    CoUninitialize();
    if (g_brushWhite) DeleteObject(g_brushWhite);
    if (g_fontUi) DeleteObject(g_fontUi);
    if (g_fontTitle) DeleteObject(g_fontTitle);
    return result;
}
