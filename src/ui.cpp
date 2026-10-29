#include "app.h"

#include <commdlg.h>
#include <objbase.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

// Compatibilidade com cabeçalhos mais antigos.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef PBS_MARQUEE
#define PBS_MARQUEE 0x08
#endif
#ifndef EM_SETCUEBANNER
#define EM_SETCUEBANNER (ECM_FIRST + 1)
#endif
#ifndef SS_ENDELLIPSIS
#define SS_ENDELLIPSIS 0x00004000L
#endif

namespace {

constexpr int kTabId = 1100;
constexpr int kPageBase = 4100;
constexpr int kSummaryId = 1101;
constexpr int kDetailId = 1102;
constexpr int kStatusId = 1103;
constexpr int kProgressId = 1104;
constexpr int kCancelId = 1105;
constexpr int kProfileLabelId = 1106;

constexpr COLORREF kColorBg = RGB(244, 246, 250);
constexpr COLORREF kColorWhite = RGB(255, 255, 255);
constexpr COLORREF kColorHeader = RGB(17, 48, 82);
constexpr COLORREF kColorAccent = RGB(32, 113, 196);
constexpr COLORREF kColorText = RGB(29, 41, 57);
constexpr COLORREF kColorMuted = RGB(101, 116, 139);
constexpr COLORREF kColorBorder = RGB(218, 225, 234);
constexpr COLORREF kColorGood = RGB(21, 128, 79);
constexpr COLORREF kColorWarn = RGB(173, 91, 12);
constexpr COLORREF kColorBad = RGB(177, 45, 45);
constexpr COLORREF kColorSoftBlue = RGB(233, 242, 253);
constexpr COLORREF kColorSoftGreen = RGB(232, 247, 239);
constexpr COLORREF kColorSoftAmber = RGB(255, 246, 230);

int PageId(ItemKind kind, int offset) {
    return kPageBase + static_cast<int>(kind) * 10 + offset;
}

int PageTabIndex(ItemKind kind) {
    switch (kind) {
        case ItemKind::Feature: return 0;
        case ItemKind::Service: return 1;
        case ItemKind::Policy: return 2;
        case ItemKind::Task: return 3;
    }
    return 0;
}

const wchar_t* ActionName(const Item* item) {
    if (!item) return L"—";
    if (!item->supported) return L"—";
    if (!item->needsChange) return Tr(STR_ACTION_ALREADY);
    switch (item->kind) {
        case ItemKind::Feature: return Tr(STR_ACTION_INSTALL);
        case ItemKind::Service:
            if (item->essential && !item->needsChange) return Tr(STR_ACTION_ESSENTIAL);
            if (item->targetStart == SERVICE_DISABLED) return Tr(STR_ACTION_DISABLE);
            if (item->targetStart == SERVICE_DEMAND_START) return Tr(STR_ACTION_MANUAL);
            return Tr(STR_ACTION_ENABLE);
        case ItemKind::Policy: return Tr(STR_ACTION_RESTORE);
        case ItemKind::Task: return Tr(STR_ACTION_ENABLE_TASK);
    }
    return Tr(STR_ACTION_RESTORE);
}

std::wstring ParentPath(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return std::wstring();
    return path.substr(0, pos);
}

HFONT MakeFont(int points, bool bold, int dpi, const wchar_t* face) {
    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(points, dpi, 72);
    lf.lfWeight = bold ? FW_SEMIBOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(lf.lfFaceName, face);
    return CreateFontIndirectW(&lf);
}

UINT GetDpiForSystemSafe() {
    typedef UINT(WINAPI* GetDpiForSystemFn)();
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto fn = reinterpret_cast<GetDpiForSystemFn>(
            GetProcAddress(user32, "GetDpiForSystem"));
        if (fn) return fn();
    }
    HDC dc = GetDC(nullptr);
    UINT dpi = dc ? static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSX)) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    return dpi ? dpi : 96;
}

UINT GetDpiForWindowSafe(HWND hwnd) {
    typedef UINT(WINAPI* GetDpiForWindowFn)(HWND);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto fn = reinterpret_cast<GetDpiForWindowFn>(
            GetProcAddress(user32, "GetDpiForWindow"));
        if (fn) return fn(hwnd);
    }
    return GetDpiForSystemSafe();
}

void SetControlFont(HWND control, HFONT font) {
    if (control && font) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

bool IsListChecked(HWND list, int index) {
    return ListView_GetCheckState(list, index) != FALSE;
}

void CenterDialog(HWND dialog) {
    RECT r = {};
    GetWindowRect(dialog, &r);
    HWND owner = GetParent(dialog);
    RECT target = {};
    if (owner && IsWindowVisible(owner)) {
        GetWindowRect(owner, &target);
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &target, 0);
    }
    int width = r.right - r.left;
    int height = r.bottom - r.top;
    int x = target.left + (target.right - target.left - width) / 2;
    int y = target.top + (target.bottom - target.top - height) / 2;
    SetWindowPos(dialog, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void SetDialogIcon(HWND dialog) {
    HICON icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APPICON));
    if (!icon) icon = LoadIconW(nullptr, IDI_APPLICATION);
    SendMessageW(dialog, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
    SendMessageW(dialog, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
}

bool BrowseForFolder(HWND owner, std::wstring* path) {
    if (!path) return false;
    wchar_t current[MAX_PATH * 4] = {};
    BROWSEINFOW browse = {};
    browse.hwndOwner = owner;
    browse.lpszTitle = L"Selecione a pasta de origem do Windows (ISO montada ou mídia)";
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    LPITEMIDLIST idList = SHBrowseForFolderW(&browse);
    if (!idList) return false;
    bool ok = SHGetPathFromIDListW(idList, current) != FALSE;
    CoTaskMemFree(idList);
    if (ok && current[0]) {
        *path = current;
        return true;
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------

App& App::Instance() {
    static App app;
    return app;
}

App::App() {
    pages_[0].kind = ItemKind::Feature;
    pages_[1].kind = ItemKind::Service;
    pages_[2].kind = ItemKind::Policy;
    pages_[3].kind = ItemKind::Task;
    trackMouse_.cbSize = sizeof(trackMouse_);
}

App::~App() {
    CloseLogging();
    if (scanThread_) CloseHandle(scanThread_);
    if (applyThread_) CloseHandle(applyThread_);
    if (cancelEvent_) CloseHandle(cancelEvent_);
    FreeFonts();
    if (brushBg_) DeleteObject(brushBg_);
    if (brushWhite_) DeleteObject(brushWhite_);
    if (brushHeader_) DeleteObject(brushHeader_);
}

int App::Run(HINSTANCE instance, const std::wstring& commandLine, int showCmd) {
    instance_ = instance;
    cmdLine_ = commandLine;
    ParseCommandLine(commandLine);
    activeProfile_ = startupProfile_;

    if (!headlessReportPath_.empty()) {
        return RunHeadless(headlessReportPath_);
    }

    admin_ = IsElevated();
    if (!admin_ && noElevateArg_.empty() && !elevatedArg_) {
        if (ElevateSelf(false)) return 0;
    }
    admin_ = IsElevated();
    readOnly_ = !admin_;

    LoadSettings();
    InitLogging();
    Log(L"Windows Lite Restore iniciado.");
    Log(admin_ ? L"Modo: administrador." : L"Modo: somente leitura (sem elevação).");
    Log(L"Sistema: " + GetWindowsBuildString());

    if (!CreateMainWindow(showCmd)) {
        MessageBoxW(nullptr, L"Não foi possível criar a janela principal.",
                    L"Windows Lite Restore", MB_ICONERROR | MB_OK);
        CloseLogging();
        return 1;
    }

    ShowWindow(hwnd_, showCmd);
    UpdateWindow(hwnd_);

    if (!resumeSelectionPath_.empty()) {
        StartScan(false);
    } else if (settings_.autoScan) {
        StartScan(true);
    } else {
        SetStatus(Tr(STR_STATUS_READY));
        UpdateDiagnostics();
    }

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CloseLogging();
    return static_cast<int>(msg.wParam);
}

void App::ParseCommandLine(const std::wstring& commandLine) {
    cmdLine_ = commandLine;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--scan-report" || arg == L"--report") {
            if (i + 1 < argc) headlessReportPath_ = argv[++i];
        } else if (arg.rfind(L"--scan-report=", 0) == 0) {
            headlessReportPath_ = arg.substr(14);
        } else if (arg == L"--no-elevate" || arg == L"--read-only") {
            noElevateArg_ = arg;
        } else if (arg == L"--elevated") {
            elevatedArg_ = true;
        } else if (arg == L"--resume-selection") {
            if (i + 1 < argc) resumeSelectionPath_ = argv[++i];
        } else if (arg.rfind(L"--resume-selection=", 0) == 0) {
            resumeSelectionPath_ = arg.substr(19);
        } else if (arg == L"--profile") {
            if (i + 1 < argc) {
                std::wstring value = Lower(argv[++i]);
                if (value == L"gaming" || value == L"jogos" || value == L"jogo") {
                    startupProfile_ = Profile::Gaming;
                } else if (value == L"ai" || value == L"ia" ||
                           value == L"inteligencia" || value == L"inteligência") {
                    startupProfile_ = Profile::AI;
                } else if (value == L"office" || value == L"escritorio" ||
                           value == L"escritório") {
                    startupProfile_ = Profile::Office;
                } else {
                    startupProfile_ = Profile::MicrosoftDefault;
                }
            }
        } else if (arg.rfind(L"--profile=", 0) == 0) {
            std::wstring value = Lower(arg.substr(10));
            if (value == L"gaming" || value == L"jogos" || value == L"jogo") {
                startupProfile_ = Profile::Gaming;
            } else if (value == L"ai" || value == L"ia" || value == L"inteligencia" ||
                       value == L"inteligência") {
                startupProfile_ = Profile::AI;
            } else if (value == L"office" || value == L"escritorio" ||
                       value == L"escritório") {
                startupProfile_ = Profile::Office;
            } else {
                startupProfile_ = Profile::MicrosoftDefault;
            }
        }
    }
    LocalFree(argv);
}

// ---------------------------------------------------------------------------
// Fontes, tema e DPI
// ---------------------------------------------------------------------------

void App::InitFonts() {
    FreeFonts();
    fontUi_ = MakeFont(9, false, dpi_, L"Segoe UI");
    fontUiBold_ = MakeFont(9, true, dpi_, L"Segoe UI");
    fontTitle_ = MakeFont(17, true, dpi_, L"Segoe UI");
    fontSubtitle_ = MakeFont(9, false, dpi_, L"Segoe UI");
    fontSmall_ = MakeFont(8, false, dpi_, L"Segoe UI");
    fontMono_ = MakeFont(9, false, dpi_, L"Consolas");
}

void App::FreeFonts() {
    if (fontUi_) DeleteObject(fontUi_);
    if (fontUiBold_) DeleteObject(fontUiBold_);
    if (fontTitle_) DeleteObject(fontTitle_);
    if (fontSubtitle_) DeleteObject(fontSubtitle_);
    if (fontSmall_) DeleteObject(fontSmall_);
    if (fontMono_) DeleteObject(fontMono_);
    fontUi_ = fontUiBold_ = fontTitle_ = fontSubtitle_ = fontSmall_ = fontMono_ = nullptr;
}

int App::Scale(int value) const {
    return MulDiv(value, dpi_, 96);
}

void App::ApplyTheme() {
    if (tab_) SetControlFont(tab_, fontUiBold_);
    for (int i = 0; i < 4; ++i) {
        Page& page = pages_[i];
        if (page.list) {
            SetControlFont(page.list, fontUi_);
            ListView_SetBkColor(page.list, kColorWhite);
            ListView_SetTextBkColor(page.list, kColorWhite);
            ListView_SetTextColor(page.list, kColorText);
            SetWindowTheme(page.list, L"Explorer", nullptr);
        }
        if (page.search) SetControlFont(page.search, fontUi_);
        if (page.combo) SetControlFont(page.combo, fontUi_);
        if (page.count) SetControlFont(page.count, fontSmall_);
        if (page.btnRecommended) SetControlFont(page.btnRecommended, fontUi_);
        if (page.btnAll) SetControlFont(page.btnAll, fontUi_);
        if (page.btnClear) SetControlFont(page.btnClear, fontUi_);
    }
    if (detailPanel_) SetControlFont(detailPanel_, fontUi_);
    if (statusText_) SetControlFont(statusText_, fontSmall_);
    if (summaryText_) SetControlFont(summaryText_, fontSmall_);
    if (profileLabel_) SetControlFont(profileLabel_, fontUi_);
    for (HWND button : profileButtons_) SetControlFont(button, fontUi_);
    if (diagEdit_) SetControlFont(diagEdit_, fontMono_);
    if (logEdit_) SetControlFont(logEdit_, fontMono_);
    for (HWND button : diagButtons_) SetControlFont(button, fontUi_);
    for (HWND button : logButtons_) SetControlFont(button, fontUi_);
    if (applyButton_) SetControlFont(applyButton_, fontUiBold_);
    if (defaultButton_) SetControlFont(defaultButton_, fontUi_);
    if (rescanButton_) SetControlFont(rescanButton_, fontUi_);
    if (settingsButton_) SetControlFont(settingsButton_, fontUi_);
    if (aboutButton_) SetControlFont(aboutButton_, fontUi_);
    if (cancelButton_) SetControlFont(cancelButton_, fontUi_);
    if (dryRunCheck_) SetControlFont(dryRunCheck_, fontUi_);
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void App::CenterWindow() {
    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    RECT r = {};
    GetWindowRect(hwnd_, &r);
    int width = r.right - r.left;
    int height = r.bottom - r.top;
    int availW = work.right - work.left - Scale(32);
    int availH = work.bottom - work.top - Scale(32);
    if (width > availW) width = availW;
    if (height > availH) height = availH;
    int x = work.left + (work.right - work.left - width) / 2;
    int y = work.top + (work.bottom - work.top - height) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER);
}

// ---------------------------------------------------------------------------
// Criação da janela e controles
// ---------------------------------------------------------------------------

Page& App::PageFor(ItemKind kind) {
    return pages_[PageTabIndex(kind)];
}

const Page& App::PageFor(ItemKind kind) const {
    return pages_[PageTabIndex(kind)];
}

void App::RegisterPage(Page& page, ItemKind kind) {
    page.kind = kind;
    page.visible.clear();
    page.categories.clear();
}

void App::CreatePageControls(Page& page) {
    const ItemKind kind = page.kind;
    const int id = PageId(kind, 0);
    page.list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT |
                                    LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                                0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                instance_, nullptr);
    ListView_SetExtendedListViewStyle(page.list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
                                                  LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);

    page.search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                  0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageId(kind, 1))),
                                  instance_, nullptr);
    SendMessageW(page.search, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(Tr(STR_SEARCH_PLACEHOLDER)));

    page.combo = CreateWindowExW(0, WC_COMBOBOXW, nullptr,
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST |
                                     WS_VSCROLL,
                                 0, 0, 0, 0, hwnd_,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageId(kind, 2))),
                                 instance_, nullptr);

    page.count = CreateWindowExW(0, L"STATIC", L"",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS | SS_NOPREFIX,
                                 0, 0, 0, 0, hwnd_,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageId(kind, 3))),
                                 instance_, nullptr);

    page.btnRecommended = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_RECOMMENDED),
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                          0, 0, 0, 0, hwnd_,
                                          reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageId(kind, 4))),
                                          instance_, nullptr);
    page.btnAll = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_ALL_PENDING),
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                  0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageId(kind, 5))),
                                  instance_, nullptr);
    page.btnClear = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_CLEAR),
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                    0, 0, 0, 0, hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageId(kind, 6))),
                                    instance_, nullptr);

    LVCOLUMNW column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    const wchar_t* headers[] = {Tr(STR_COL_ITEM), Tr(STR_COL_CATEGORY), Tr(STR_COL_CURRENT),
                                Tr(STR_COL_TARGET_DEFAULT), Tr(STR_COL_ACTION)};
    int widths[] = {330, 150, 190, 200, 120};
    for (int i = 0; i < 5; ++i) {
        column.pszText = const_cast<wchar_t*>(headers[i]);
        column.cx = Scale(widths[i]);
        ListView_InsertColumn(page.list, i, &column);
    }
}

bool App::CreateMainWindow(int showCmd) {
    (void)showCmd;
    dpi_ = static_cast<int>(GetDpiForSystemSafe());
    if (dpi_ <= 0) dpi_ = 96;

    InitFonts();
    if (!brushBg_) brushBg_ = CreateSolidBrush(kColorBg);
    if (!brushWhite_) brushWhite_ = CreateSolidBrush(kColorWhite);
    if (!brushHeader_) brushHeader_ = CreateSolidBrush(kColorHeader);

    iconLarge_ = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APPICON));
    if (!iconLarge_) iconLarge_ = LoadIconW(nullptr, IDI_APPLICATION);
    iconSmall_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APPICON),
                                               IMAGE_ICON, Scale(16), Scale(16), LR_DEFAULTCOLOR));
    if (!iconSmall_) iconSmall_ = iconLarge_;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = App::WndProc;
    wc.hInstance = instance_;
    wc.hIcon = iconLarge_;
    wc.hIconSm = iconSmall_;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = brushBg_;
    wc.lpszClassName = L"WLRestoreMainWindow";
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    WNDCLASSEXW detailClass = {};
    detailClass.cbSize = sizeof(detailClass);
    detailClass.style = CS_HREDRAW | CS_VREDRAW;
    detailClass.lpfnWndProc = App::DetailWndProc;
    detailClass.hInstance = instance_;
    detailClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    detailClass.hbrBackground = brushWhite_;
    detailClass.lpszClassName = L"WLRestoreDetailPanel";
    if (!RegisterClassExW(&detailClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    hwnd_ = CreateWindowExW(0, L"WLRestoreMainWindow", Tr(STR_APP_TITLE),
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                            Scale(1180), Scale(780), nullptr, nullptr, instance_, this);
    if (!hwnd_) return false;

    UINT windowDpi = GetDpiForWindowSafe(hwnd_);
    if (windowDpi && static_cast<int>(windowDpi) != dpi_) {
        dpi_ = static_cast<int>(windowDpi);
        InitFonts();
    }

    // Abas.
    tab_ = CreateWindowExW(0, WC_TABCONTROLW, nullptr,
                           WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_HOTTRACK,
                           0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTabId)),
                           instance_, nullptr);
    const wchar_t* tabLabels[] = {Tr(STR_TAB_FEATURES), Tr(STR_TAB_SERVICES), Tr(STR_TAB_POLICIES),
                                  Tr(STR_TAB_TASKS), Tr(STR_TAB_DIAG), Tr(STR_TAB_LOG)};
    for (int i = 0; i < 6; ++i) {
        TCITEMW item = {};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t*>(tabLabels[i]);
        TabCtrl_InsertItem(tab_, i, &item);
    }

    for (int i = 0; i < 4; ++i) CreatePageControls(pages_[i]);

    detailPanel_ = CreateWindowExW(0, L"WLRestoreDetailPanel", L"",
                                   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0, hwnd_,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDetailId)),
                                   instance_, this);

    // Barra inferior.
    summaryText_ = CreateWindowExW(0, L"STATIC", L"",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS | SS_NOPREFIX,
                                   0, 0, 0, 0, hwnd_,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSummaryId)),
                                   instance_, nullptr);
    profileLabel_ = CreateWindowExW(0, L"STATIC", Tr(STR_PROFILE_LABEL),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 0, 0,
                                    hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kProfileLabelId)),
                                    instance_, nullptr);
    const wchar_t* profileLabels[4] = {Tr(STR_PROFILE_DEFAULT), Tr(STR_PROFILE_GAMING),
                                       Tr(STR_PROFILE_AI), Tr(STR_PROFILE_OFFICE)};
    for (int i = 0; i < 4; ++i) {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | BS_PUSHLIKE;
        if (i == 0) style |= WS_GROUP;
        profileButtons_[i] = CreateWindowExW(
            0, L"BUTTON", profileLabels[i], style, 0, 0, 0, 0, hwnd_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PROFILE + i)), instance_, nullptr);
    }
    Button_SetCheck(profileButtons_[static_cast<int>(activeProfile_)], BST_CHECKED);
    dryRunCheck_ = CreateWindowExW(0, L"BUTTON", Tr(STR_SIMULATE),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 0, 0,
                                   hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_DRYRUN)),
                                   instance_, nullptr);

    settingsButton_ = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_SETTINGS),
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0,
                                      hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SETTINGS)),
                                      instance_, nullptr);
    rescanButton_ = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_RESCAN),
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0,
                                    hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SCAN)),
                                    instance_, nullptr);
    aboutButton_ = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_ABOUT),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0,
                                   hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ABOUT)),
                                   instance_, nullptr);
    defaultButton_ = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_RESTORE_DEFAULT),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0,
                                     hwnd_,
                                     reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RESTORE_DEFAULT)),
                                     instance_, nullptr);
    applyButton_ = CreateWindowExW(0, L"BUTTON", Tr(STR_BTN_APPLY_SELECTED),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0,
                                   hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_APPLY)),
                                   instance_, nullptr);

    statusText_ = CreateWindowExW(0, L"STATIC", L"",
                                  WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS | SS_NOPREFIX,
                                  0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)),
                                  instance_, nullptr);
    progress_ = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
                                WS_CHILD | PBS_SMOOTH | PBS_MARQUEE, 0, 0, 0, 0, hwnd_,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kProgressId)),
                                instance_, nullptr);
    cancelButton_ = CreateWindowExW(0, L"BUTTON", Tr(STR_CANCEL),
                                    WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelId)),
                                    instance_, nullptr);

    // Diagnóstico.
    diagEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY |
                                    WS_VSCROLL | ES_AUTOVSCROLL,
                                0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    SendMessageW(diagEdit_, EM_SETLIMITTEXT, 8 * 1024 * 1024, 0);
    const struct {
        int id;
        const wchar_t* text;
    } diagButtons[] = {
        {IDC_DIAG_REFRESH, Tr(STR_DIAG_BTN_REFRESH)},
        {IDC_DIAG_SAVE, Tr(STR_DIAG_BTN_SAVE)},
        {IDC_DIAG_LOGS, Tr(STR_DIAG_BTN_LOGS)},
        {IDC_DIAG_BACKUPS, Tr(STR_DIAG_BTN_BACKUPS)},
        {IDC_DIAG_UNDO, Tr(STR_DIAG_BTN_UNDO)},
    };
    for (int i = 0; i < 5; ++i) {
        diagButtons_[i] = CreateWindowExW(0, L"BUTTON", diagButtons[i].text,
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0,
                                          0, 0, hwnd_,
                                          reinterpret_cast<HMENU>(static_cast<INT_PTR>(diagButtons[i].id)),
                                          instance_, nullptr);
    }

    // Registro (log).
    logEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY |
                                   WS_VSCROLL | ES_AUTOVSCROLL,
                               0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    SendMessageW(logEdit_, EM_SETLIMITTEXT, 8 * 1024 * 1024, 0);
    const struct {
        int id;
        const wchar_t* text;
    } logButtons[] = {
        {IDC_LOG_SAVE, Tr(STR_LOG_BTN_SAVE)},
        {IDC_LOG_FOLDER, Tr(STR_LOG_BTN_FOLDER)},
        {IDC_LOG_CLEAR, Tr(STR_LOG_BTN_CLEAR)},
    };
    for (int i = 0; i < 3; ++i) {
        logButtons_[i] = CreateWindowExW(0, L"BUTTON", logButtons[i].text,
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0,
                                         0, hwnd_,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(logButtons[i].id)),
                                         instance_, nullptr);
    }

    // Pré-carrega o log acumulado até aqui.
    if (logCsInit_) EnterCriticalSection(&logCs_);
    std::wstring initialLog = logBuffer_;
    if (logCsInit_) LeaveCriticalSection(&logCs_);
    if (initialLog.size() > 512 * 1024) {
        initialLog = initialLog.substr(initialLog.size() - 512 * 1024);
    }
    SetWindowTextW(logEdit_, initialLog.c_str());

    ApplyTheme();
    Layout();
    ShowTab(0);
    CenterWindow();
    SetWindowTextW(hwnd_, admin_ ? Tr(STR_APP_TITLE) : Tr(STR_APP_TITLE_READONLY));
    return true;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void App::Layout() {
    if (!hwnd_) return;
    RECT rc = {};
    GetClientRect(hwnd_, &rc);
    const int m = Scale(16);
    const int headerH = Scale(92);
    const int tabH = Scale(34);
    const int profileH = Scale(40);
    const int gap = Scale(8);
    const int footerH = Scale(78);
    const int statusH = Scale(26);
    const int clientW = rc.right - rc.left;
    const int clientH = rc.bottom - rc.top;

    if (tab_) MoveWindow(tab_, m, headerH, clientW - 2 * m, tabH, TRUE);

    const int profileY = headerH + tabH + gap;
    const int contentY = headerH + tabH + gap + profileH + gap;
    const int footerY = clientH - statusH - footerH;
    const int contentH = footerY - contentY;
    const int detailW = Scale(330);

    // Barra de perfil (abaixo das abas).
    if (profileLabel_) MoveWindow(profileLabel_, m, profileY + Scale(9), Scale(150), Scale(22), TRUE);
    int px = m + Scale(150);
    int buttonWidths[4] = {Scale(150), Scale(90), Scale(80), Scale(110)};
    for (int i = 0; i < 4; ++i) {
        if (profileButtons_[i]) {
            MoveWindow(profileButtons_[i], px, profileY + Scale(4), buttonWidths[i], Scale(30), TRUE);
            px += buttonWidths[i] + gap;
        }
    }

    // Páginas de itens.
    for (int i = 0; i < 4; ++i) {
        Page& page = pages_[i];
        const int listW = clientW - 2 * m - detailW - gap;
        const int rowY = contentY;
        const int listY = contentY + Scale(72);
        const int listH = contentH - Scale(72);
        if (page.search) MoveWindow(page.search, m, rowY + Scale(2), Scale(240), Scale(26), TRUE);
        if (page.combo) MoveWindow(page.combo, m + Scale(248), rowY + Scale(2), Scale(210), Scale(240), TRUE);
        if (page.count) MoveWindow(page.count, m + listW - Scale(220), rowY + Scale(6), Scale(220), Scale(20), TRUE);
        if (page.btnRecommended) MoveWindow(page.btnRecommended, m, rowY + Scale(38), Scale(130), Scale(28), TRUE);
        if (page.btnAll) MoveWindow(page.btnAll, m + Scale(138), rowY + Scale(38), Scale(132), Scale(28), TRUE);
        if (page.btnClear) MoveWindow(page.btnClear, m + Scale(278), rowY + Scale(38), Scale(84), Scale(28), TRUE);
        if (page.list) {
            MoveWindow(page.list, m, listY, listW, listH, TRUE);
            int widths[5];
            widths[0] = listW * 34 / 100;
            widths[1] = listW * 16 / 100;
            widths[2] = listW * 21 / 100;
            widths[3] = listW * 20 / 100;
            widths[4] = listW - widths[0] - widths[1] - widths[2] - widths[3] - 4;
            for (int c = 0; c < 5; ++c) ListView_SetColumnWidth(page.list, c, widths[c]);
        }
    }

    if (detailPanel_) MoveWindow(detailPanel_, m + (clientW - 2 * m - detailW - gap), contentY, detailW, contentH, TRUE);

    // Diagnóstico.
    if (diagEdit_) MoveWindow(diagEdit_, m, contentY, clientW - 2 * m, contentH - Scale(44), TRUE);
    const int diagButtonW = Scale(150);
    for (int i = 0; i < 5; ++i) {
        if (diagButtons_[i]) {
            MoveWindow(diagButtons_[i], m + i * (diagButtonW + gap), contentY + contentH - Scale(36),
                       diagButtonW, Scale(30), TRUE);
        }
    }

    // Registro.
    if (logEdit_) MoveWindow(logEdit_, m, contentY, clientW - 2 * m, contentH - Scale(44), TRUE);
    const int logButtonW = Scale(150);
    for (int i = 0; i < 3; ++i) {
        if (logButtons_[i]) {
            MoveWindow(logButtons_[i], m + i * (logButtonW + gap), contentY + contentH - Scale(36),
                       logButtonW, Scale(30), TRUE);
        }
    }

    // Barra inferior.
    if (summaryText_) MoveWindow(summaryText_, m, footerY + Scale(4), clientW - 2 * m, Scale(22), TRUE);
    if (dryRunCheck_) MoveWindow(dryRunCheck_, m, footerY + Scale(40), Scale(260), Scale(24), TRUE);

    struct FooterButton {
        HWND hwnd;
        int width;
    };
    FooterButton buttons[] = {
        {applyButton_, Scale(170)},
        {defaultButton_, Scale(210)},
        {aboutButton_, Scale(70)},
        {rescanButton_, Scale(140)},
        {settingsButton_, Scale(120)},
    };
    int bx = clientW - m;
    for (const FooterButton& button : buttons) {
        if (!button.hwnd || !IsWindowVisible(button.hwnd)) continue;
        bx -= button.width;
        MoveWindow(button.hwnd, bx, footerY + Scale(36), button.width, Scale(30), TRUE);
        bx -= gap;
    }

    // Barra de status.
    const int progressW = Scale(180);
    const int cancelW = Scale(78);
    int right = clientW - m;
    if (cancelButton_ && IsWindowVisible(cancelButton_)) {
        MoveWindow(cancelButton_, right - cancelW, clientH - statusH + Scale(2), cancelW, Scale(22), TRUE);
        right -= cancelW + gap;
    }
    if (progress_) {
        if (IsWindowVisible(progress_)) {
            MoveWindow(progress_, right - progressW, clientH - statusH + Scale(5), progressW, Scale(16), TRUE);
            right -= progressW + gap;
        }
    }
    if (statusText_) {
        MoveWindow(statusText_, m, clientH - statusH + Scale(4), right - m, Scale(20), TRUE);
    }
}

void App::UpdateTabLabels() {
    if (!tab_) return;
    const wchar_t* base[] = {Tr(STR_TAB_FEATURES), Tr(STR_TAB_SERVICES), Tr(STR_TAB_POLICIES),
                             Tr(STR_TAB_TASKS), Tr(STR_TAB_DIAG), Tr(STR_TAB_LOG)};
    for (int i = 0; i < 4; ++i) {
        int pending = 0;
        for (const Item& item : items_) {
            if (item.kind == pages_[i].kind && item.supported && item.needsChange) ++pending;
        }
        std::wstring label = base[i];
        if (pending > 0) label += L" (" + FormatInt(pending) + L")";
        TCITEMW t = {};
        t.mask = TCIF_TEXT;
        t.pszText = const_cast<wchar_t*>(label.c_str());
        TabCtrl_SetItem(tab_, i, &t);
    }
}

void App::ShowTab(int index) {
    if (index < 0 || index > 5) return;
    if (tab_ && TabCtrl_GetCurSel(tab_) != index) TabCtrl_SetCurSel(tab_, index);
    currentTab_ = index;

    for (int i = 0; i < 4; ++i) {
        Page& page = pages_[i];
        bool show = (i == index);
        int cmd = show ? SW_SHOW : SW_HIDE;
        if (page.list) ShowWindow(page.list, cmd);
        if (page.search) ShowWindow(page.search, cmd);
        if (page.combo) ShowWindow(page.combo, cmd);
        if (page.count) ShowWindow(page.count, cmd);
        if (page.btnRecommended) ShowWindow(page.btnRecommended, cmd);
        if (page.btnAll) ShowWindow(page.btnAll, cmd);
        if (page.btnClear) ShowWindow(page.btnClear, cmd);
    }

    bool itemPage = index < 4;
    ShowWindow(detailPanel_, itemPage ? SW_SHOW : SW_HIDE);
    ShowWindow(dryRunCheck_, itemPage ? SW_SHOW : SW_HIDE);
    ShowWindow(summaryText_, itemPage ? SW_SHOW : SW_HIDE);
    ShowWindow(applyButton_, itemPage ? SW_SHOW : SW_HIDE);
    ShowWindow(defaultButton_, itemPage ? SW_SHOW : SW_HIDE);

    ShowWindow(diagEdit_, index == 4 ? SW_SHOW : SW_HIDE);
    for (HWND button : diagButtons_) ShowWindow(button, index == 4 ? SW_SHOW : SW_HIDE);
    ShowWindow(logEdit_, index == 5 ? SW_SHOW : SW_HIDE);
    for (HWND button : logButtons_) ShowWindow(button, index == 5 ? SW_SHOW : SW_HIDE);

    if (itemPage) {
        Page& page = pages_[index];
        Item* item = nullptr;
        int sel = ListView_GetNextItem(page.list, -1, LVNI_SELECTED);
        if (sel >= 0 && static_cast<size_t>(sel) < page.visible.size()) {
            item = page.visible[sel];
        }
        UpdateDetails(item);
    } else if (index == 4) {
        UpdateDiagnostics();
    }

    UpdateFooterLabels();
    UpdateSelectionUi();
    Layout();
}

void App::RefreshAllCounts() {
    for (int i = 0; i < 4; ++i) UpdatePageCount(pages_[i]);
    UpdateTabLabels();
    UpdateSelectionUi();
}

// ---------------------------------------------------------------------------
// Desenho personalizado
// ---------------------------------------------------------------------------

void App::DrawRoundedRect(HDC dc, const RECT& rc, int radius, COLORREF fill, COLORREF border) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void App::DrawHeaderBadge(HDC dc) {
    RECT client = {};
    GetClientRect(hwnd_, &client);
    const int headerH = Scale(92);
    const wchar_t* text = admin_ ? L"ADMINISTRADOR" : L"ELEVAR AGORA";
    COLORREF fill = admin_ ? kColorGood : (elevateHover_ ? RGB(198, 116, 35) : kColorWarn);

    HGDIOBJ old = SelectObject(dc, fontUiBold_);
    SIZE size = {};
    GetTextExtentPoint32W(dc, text, static_cast<int>(wcslen(text)), &size);
    const int w = size.cx + Scale(26);
    const int h = Scale(26);
    const int right = client.right - Scale(24);
    const int top = (headerH - h) / 2;
    elevateRect_ = {right - w, top, right, top + h};
    DrawRoundedRect(dc, elevateRect_, Scale(13), fill, fill);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT textRect = elevateRect_;
    DrawTextW(dc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);

    SetTextColor(dc, RGB(150, 178, 206));
    SelectObject(dc, fontSmall_);
    RECT version = {right - Scale(90), top + h + Scale(4), right, top + h + Scale(18)};
    DrawTextW(dc, L"v1.0.0", -1, &version, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
}

void App::PaintHeader(HDC dc, const RECT& client) {
    RECT header = client;
    header.bottom = Scale(92);
    FillRect(dc, &header, brushHeader_);

    RECT accent = {header.left, header.bottom - Scale(3), header.right, header.bottom};
    HBRUSH accentBrush = CreateSolidBrush(kColorAccent);
    FillRect(dc, &accent, accentBrush);
    DeleteObject(accentBrush);

    const int x = Scale(24);
    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ old = SelectObject(dc, fontTitle_);
    SetTextColor(dc, RGB(255, 255, 255));
    const wchar_t* headerTitle = Tr(STR_HEADER_TITLE);
    TextOutW(dc, x, Scale(14), headerTitle, static_cast<int>(wcslen(headerTitle)));

    SelectObject(dc, fontSubtitle_);
    SetTextColor(dc, RGB(183, 207, 232));
    const wchar_t* subtitle = admin_ ? Tr(STR_HEADER_SUBTITLE_ADMIN)
                                     : Tr(STR_HEADER_SUBTITLE_READONLY);
    TextOutW(dc, x, Scale(48), subtitle, static_cast<int>(wcslen(subtitle)));
    SelectObject(dc, old);

    DrawHeaderBadge(dc);
}

void App::PaintDetailPanel(HDC dc) {
    RECT rc = {};
    GetClientRect(detailPanel_, &rc);
    FillRect(dc, &rc, brushWhite_);

    RECT accent = {rc.left, rc.top, rc.right, rc.top + Scale(3)};
    HBRUSH accentBrush = CreateSolidBrush(kColorAccent);
    FillRect(dc, &accent, accentBrush);
    DeleteObject(accentBrush);

    const int x = Scale(16);
    int y = Scale(16);
    const int w = rc.right - rc.left - Scale(32);
    SetBkMode(dc, TRANSPARENT);

    if (!detailItem_) {
        HGDIOBJ old = SelectObject(dc, fontUiBold_);
        SetTextColor(dc, kColorText);
        RECT title = {x, y, x + w, y + Scale(40)};
        DrawTextW(dc, Tr(STR_DETAIL_TITLE), -1, &title, DT_SINGLELINE | DT_NOPREFIX);
        y += Scale(40);
        SelectObject(dc, fontUi_);
        SetTextColor(dc, kColorMuted);
        RECT body = {x, y, x + w, y + Scale(120)};
        DrawTextW(dc, Tr(STR_DETAIL_EMPTY), -1, &body, DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(dc, old);
        return;
    }

    const Item* item = detailItem_;

    auto drawLabelValue = [&](const wchar_t* label, const std::wstring& value, HFONT valueFont,
                              COLORREF valueColor) {
        HGDIOBJ oldFont = SelectObject(dc, fontSmall_);
        SetTextColor(dc, kColorMuted);
        RECT lr = {x, y, x + w, y + Scale(14)};
        DrawTextW(dc, label, -1, &lr, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        y += Scale(15);
        SelectObject(dc, valueFont);
        SetTextColor(dc, valueColor);
        RECT vr = {x, y, x + w, y + Scale(22)};
        DrawTextW(dc, value.c_str(), -1, &vr, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        SelectObject(dc, oldFont);
        y += Scale(24);
    };

    // Categoria.
    HGDIOBJ old = SelectObject(dc, fontSmall_);
    SetTextColor(dc, kColorAccent);
    RECT cat = {x, y, x + w, y + Scale(16)};
    DrawTextW(dc, TrCategory(item->category.c_str()), -1, &cat,
              DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    y += Scale(18);
    SelectObject(dc, old);

    // Título.
    SelectObject(dc, fontUiBold_);
    SetTextColor(dc, kColorText);
    RECT calc = {x, y, x + w, y + Scale(80)};
    int titleHeight = DrawTextW(dc, item->name.c_str(), -1, &calc,
                                DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
    RECT title = {x, y, x + w, y + titleHeight};
    DrawTextW(dc, item->name.c_str(), -1, &title, DT_WORDBREAK | DT_NOPREFIX);
    y += titleHeight + Scale(10);
    SelectObject(dc, old);

    // Divisor.
    HBRUSH line = CreateSolidBrush(kColorBorder);
    RECT divider = {x, y, x + w, y + 1};
    FillRect(dc, &divider, line);
    DeleteObject(line);
    y += Scale(10);

    drawLabelValue(Tr(STR_LABEL_CURRENT), item->current, fontUi_, kColorText);
    const wchar_t* targetLabel = activeProfile_ == Profile::MicrosoftDefault
                                     ? Tr(STR_LABEL_TARGET_DEFAULT)
                                     : Tr(STR_LABEL_TARGET_PROFILE);
    drawLabelValue(targetLabel, item->target, fontUi_, kColorText);

    // Divisor.
    line = CreateSolidBrush(kColorBorder);
    divider = {x, y, x + w, y + 1};
    FillRect(dc, &divider, line);
    DeleteObject(line);
    y += Scale(10);

    const int boxH = Scale(62);
    const int boxTop = rc.bottom - Scale(16) - boxH;
    const int descBottom = boxTop - Scale(8);

    if (y < descBottom && !item->description.empty()) {
        SelectObject(dc, fontUi_);
        SetTextColor(dc, kColorText);
        RECT dr = {x, y, x + w, descBottom};
        DrawTextW(dc, item->description.c_str(), -1, &dr,
                  DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
    }

    if (!item->extra.empty()) {
        int extraTop = boxTop - Scale(24);
        if (y < extraTop) {
            SelectObject(dc, fontMono_);
            SetTextColor(dc, kColorMuted);
            RECT er = {x, y, x + w, extraTop};
            DrawTextW(dc, item->extra.c_str(), -1, &er, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        }
    }

    COLORREF fill = kColorSoftBlue;
    COLORREF color = kColorAccent;
    std::wstring status;
    if (item->essential) {
        fill = kColorSoftBlue;
        color = RGB(16, 80, 150);
        status = Tr(STR_DETAIL_ESSENTIAL);
    } else if (!item->supported) {
        fill = kColorSoftAmber;
        color = kColorWarn;
        status = Tr(STR_DETAIL_UNAVAILABLE);
    } else if (!item->needsChange) {
        fill = kColorSoftGreen;
        color = kColorGood;
        status = Tr(STR_DETAIL_OK);
    } else {
        fill = kColorSoftBlue;
        color = kColorAccent;
        status = Tr(STR_DETAIL_PENDING);
    }
    if (item->critical && !item->essential && item->needsChange) status += L" Item crítico do sistema.";
    if (item->optional && item->needsChange) status += L" Item opcional.";

    RECT box = {x, boxTop, x + w, boxTop + boxH};
    DrawRoundedRect(dc, box, Scale(10), fill, kColorBorder);
    SelectObject(dc, fontUi_);
    SetTextColor(dc, color);
    RECT boxText = {x + Scale(12), box.top + Scale(8), x + w - Scale(12), box.bottom - Scale(8)};
    DrawTextW(dc, status.c_str(), -1, &boxText, DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(dc, old);
}

// ---------------------------------------------------------------------------
// Lógica das páginas
// ---------------------------------------------------------------------------

void App::FillPageCategories(Page& page) {
    page.categories.clear();
    if (page.combo) {
        SendMessageW(page.combo, CB_RESETCONTENT, 0, 0);
        SendMessageW(page.combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Todas as categorias"));
    }
    std::vector<std::wstring> cats;
    for (const Item& item : items_) {
        if (item.kind != page.kind) continue;
        if (std::find(cats.begin(), cats.end(), item.category) == cats.end()) {
            cats.push_back(item.category);
        }
    }
    std::sort(cats.begin(), cats.end(), [](const std::wstring& a, const std::wstring& b) {
        return Lower(a) < Lower(b);
    });
    page.categories = cats;
    if (page.combo) {
        for (const std::wstring& category : cats) {
            SendMessageW(page.combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(category.c_str()));
        }
        ComboBox_SetCurSel(page.combo, 0);
    }
}

bool App::PageFilterMatch(const Page& page, const Item* item) const {
    if (!item || item->kind != page.kind) return false;
    int sel = page.combo ? ComboBox_GetCurSel(page.combo) : 0;
    if (sel > 0) {
        int index = sel - 1;
        if (index >= static_cast<int>(page.categories.size()) ||
            item->category != page.categories[index]) {
            return false;
        }
    }
    wchar_t query[512] = {};
    if (page.search) GetWindowTextW(page.search, query, 512);
    if (query[0]) {
        std::wstring q = Lower(Trim(query));
        if (!q.empty()) {
            std::wstring hay = Lower(item->name + L" " + item->category + L" " + item->extra +
                                    L" " + item->description);
            if (hay.find(q) == std::wstring::npos) return false;
        }
    }
    return true;
}

void App::UpdatePageCount(Page& page) {
    int shown = static_cast<int>(page.visible.size());
    int selected = 0;
    for (const Item* item : page.visible) {
        if (item->selected) ++selected;
    }
    std::wstring text = FormatInt(shown) + (shown == 1 ? L" item" : L" itens");
    if (selected > 0) text += L" • " + FormatInt(selected) + L" selecionado(s)";
    if (page.count) SetWindowTextW(page.count, text.c_str());
}

void App::RefreshPage(Page& page) {
    if (!page.list) return;
    Item* keep = detailItem_;
    updatingList_ = true;
    ListView_DeleteAllItems(page.list);
    page.visible.clear();
    for (Item& item : items_) {
        if (!PageFilterMatch(page, &item)) continue;
        page.visible.push_back(&item);
    }
    for (size_t i = 0; i < page.visible.size(); ++i) {
        const Item* item = page.visible[i];
        LVITEMW lv = {};
        lv.mask = LVIF_TEXT | LVIF_PARAM;
        lv.iItem = static_cast<int>(i);
        lv.pszText = const_cast<wchar_t*>(item->name.c_str());
        lv.lParam = reinterpret_cast<LPARAM>(item);
        ListView_InsertItem(page.list, &lv);
        ListView_SetItemText(page.list, static_cast<int>(i), 1,
                             const_cast<wchar_t*>(item->category.c_str()));
        ListView_SetItemText(page.list, static_cast<int>(i), 2,
                             const_cast<wchar_t*>(item->current.c_str()));
        ListView_SetItemText(page.list, static_cast<int>(i), 3,
                             const_cast<wchar_t*>(item->target.c_str()));
        ListView_SetItemText(page.list, static_cast<int>(i), 4,
                             const_cast<wchar_t*>(ActionName(item)));
        if (item->supported && item->needsChange && item->selected) {
            ListView_SetCheckState(page.list, static_cast<int>(i), TRUE);
        }
    }
    updatingList_ = false;

    int keepIndex = -1;
    for (size_t i = 0; i < page.visible.size(); ++i) {
        if (page.visible[i] == keep) {
            keepIndex = static_cast<int>(i);
            break;
        }
    }
    if (keepIndex >= 0) {
        ListView_SetItemState(page.list, keepIndex, LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
        UpdateDetails(keep);
    } else if (currentTab_ == PageTabIndex(page.kind)) {
        detailItem_ = nullptr;
        if (detailPanel_) InvalidateRect(detailPanel_, nullptr, TRUE);
    }
    UpdatePageCount(page);
}

void App::PopulatePages(bool autoSelectRecommended) {
    updatingList_ = true;
    for (int i = 0; i < 4; ++i) {
        Page& page = pages_[i];
        if (page.search) SetWindowTextW(page.search, L"");
        FillPageCategories(page);
    }
    updatingList_ = false;

    for (Item& item : items_) item.selected = false;
    if (autoSelectRecommended) ApplyProfileSelection();

    detailItem_ = nullptr;
    for (int i = 0; i < 4; ++i) RefreshPage(pages_[i]);
    if (detailPanel_) InvalidateRect(detailPanel_, nullptr, TRUE);
    RefreshAllCounts();
    UpdateFooterLabels();
    UpdateDiagnostics();
}

void App::SelectPageRecommended(Page& page) {
    for (Item& item : items_) {
        if (item.kind != page.kind) continue;
        item.selected = item.supported && item.needsChange && item.recommended;
    }
    RefreshPage(page);
    UpdateSelectionUi();
}

void App::SelectPageAll(Page& page) {
    for (Item& item : items_) {
        if (item.kind != page.kind) continue;
        item.selected = item.supported && item.needsChange;
    }
    RefreshPage(page);
    UpdateSelectionUi();
}

void App::ClearPage(Page& page) {
    for (Item& item : items_) {
        if (item.kind != page.kind) continue;
        item.selected = false;
    }
    RefreshPage(page);
    UpdateSelectionUi();
}

void App::RecomputeServiceTargets() {
    for (Item& item : items_) {
        if (item.kind == ItemKind::Service) ResolveServiceTarget(item, activeProfile_);
    }
}

void App::ApplyProfileSelection() {
    for (Item& item : items_) item.selected = false;
    for (Item& item : items_) {
        if (!item.supported || !item.needsChange || !item.recommended) continue;
        if (activeProfile_ == Profile::MicrosoftDefault || item.kind == ItemKind::Service) {
            item.selected = true;
        }
    }
}

void App::UpdateFooterLabels() {
    if (defaultButton_) {
        SetWindowTextW(defaultButton_,
                       activeProfile_ == Profile::MicrosoftDefault
                           ? Tr(STR_BTN_RESTORE_DEFAULT)
                           : Tr(STR_BTN_APPLY_PROFILE));
    }
    const wchar_t* columnTitle = activeProfile_ == Profile::MicrosoftDefault
                                     ? Tr(STR_COL_TARGET_DEFAULT)
                                     : Tr(STR_COL_TARGET_PROFILE);
    for (int i = 0; i < 4; ++i) {
        if (!pages_[i].list) continue;
        LVCOLUMNW column = {};
        column.mask = LVCF_TEXT;
        column.pszText = const_cast<wchar_t*>(columnTitle);
        ListView_SetColumn(pages_[i].list, 3, &column);
    }
}

void App::UpdateProfileButtons() {
    for (int i = 0; i < 4; ++i) {
        if (profileButtons_[i]) {
            Button_SetCheck(profileButtons_[i],
                            i == static_cast<int>(activeProfile_) ? BST_CHECKED : BST_UNCHECKED);
        }
    }
}

void App::ApplyLanguage() {
    if (hwnd_) {
        SetWindowTextW(hwnd_, admin_ ? Tr(STR_APP_TITLE) : Tr(STR_APP_TITLE_READONLY));
    }
    if (profileLabel_) SetWindowTextW(profileLabel_, Tr(STR_PROFILE_LABEL));
    const wchar_t* profileLabels[4] = {Tr(STR_PROFILE_DEFAULT), Tr(STR_PROFILE_GAMING),
                                       Tr(STR_PROFILE_AI), Tr(STR_PROFILE_OFFICE)};
    for (int i = 0; i < 4; ++i) {
        if (profileButtons_[i]) SetWindowTextW(profileButtons_[i], profileLabels[i]);
    }
    if (dryRunCheck_) SetWindowTextW(dryRunCheck_, Tr(STR_SIMULATE));
    if (settingsButton_) SetWindowTextW(settingsButton_, Tr(STR_BTN_SETTINGS));
    if (rescanButton_) SetWindowTextW(rescanButton_, Tr(STR_BTN_RESCAN));
    if (aboutButton_) SetWindowTextW(aboutButton_, Tr(STR_BTN_ABOUT));
    if (applyButton_) SetWindowTextW(applyButton_, Tr(STR_BTN_APPLY_SELECTED));
    if (cancelButton_) SetWindowTextW(cancelButton_, Tr(STR_CANCEL));
    for (int i = 0; i < 4; ++i) {
        Page& page = pages_[i];
        if (page.search) SendMessageW(page.search, EM_SETCUEBANNER, TRUE,
                                      reinterpret_cast<LPARAM>(Tr(STR_SEARCH_PLACEHOLDER)));
        if (page.btnRecommended) SetWindowTextW(page.btnRecommended, Tr(STR_BTN_RECOMMENDED));
        if (page.btnAll) SetWindowTextW(page.btnAll, Tr(STR_BTN_ALL_PENDING));
        if (page.btnClear) SetWindowTextW(page.btnClear, Tr(STR_BTN_CLEAR));
    }
    const wchar_t* diagTexts[5] = {Tr(STR_DIAG_BTN_REFRESH), Tr(STR_DIAG_BTN_SAVE),
                                   Tr(STR_DIAG_BTN_LOGS), Tr(STR_DIAG_BTN_BACKUPS),
                                   Tr(STR_DIAG_BTN_UNDO)};
    for (int i = 0; i < 5; ++i) {
        if (diagButtons_[i]) SetWindowTextW(diagButtons_[i], diagTexts[i]);
    }
    const wchar_t* logTexts[3] = {Tr(STR_LOG_BTN_SAVE), Tr(STR_LOG_BTN_FOLDER),
                                  Tr(STR_LOG_BTN_CLEAR)};
    for (int i = 0; i < 3; ++i) {
        if (logButtons_[i]) SetWindowTextW(logButtons_[i], logTexts[i]);
    }
    UpdateTabLabels();
    UpdateFooterLabels();
    UpdateProfileButtons();
    UpdateDiagnostics();
    UpdateSelectionUi();
    if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void App::UpdateDetails(Item* item) {
    detailItem_ = item;
    if (detailPanel_) InvalidateRect(detailPanel_, nullptr, TRUE);
}

void App::FocusPageFor(ItemKind kind) {
    ShowTab(PageTabIndex(kind));
}

std::wstring App::SelectedSummaryText() const {
    int selected = 0;
    int recPending = 0;
    int feat = 0, serv = 0, pol = 0, task = 0;
    for (const Item& item : items_) {
        if (!item.supported || !item.needsChange) continue;
        if (item.recommended) ++recPending;
        if (!item.selected) continue;
        ++selected;
        switch (item.kind) {
            case ItemKind::Feature: ++feat; break;
            case ItemKind::Service: ++serv; break;
            case ItemKind::Policy: ++pol; break;
            case ItemKind::Task: ++task; break;
        }
    }
    std::wstring text;
    if (selected == 0) {
        text = L"Nenhum item selecionado • " + FormatInt(recPending) + L" pendência(s) recomendada(s)";
    } else {
        text = FormatInt(selected) + L" selecionado(s) • recursos " + FormatInt(feat) +
               L", serviços " + FormatInt(serv) + L", políticas " + FormatInt(pol) +
               L", tarefas " + FormatInt(task);
    }
    if (dryRun_) text += L" • Simulação ativa";
    return text;
}

std::vector<Item*> App::BuildPlan(bool recommendedOnly) const {
    std::vector<Item*> plan;
    for (const Item& item : items_) {
        if (!item.supported || !item.needsChange) continue;
        if (recommendedOnly) {
            if (activeProfile_ != Profile::MicrosoftDefault && item.kind != ItemKind::Service) {
                continue;
            }
            if (!item.recommended) continue;
        } else if (!item.selected) {
            continue;
        }
        plan.push_back(const_cast<Item*>(&item));
    }
    return plan;
}

void App::UpdateSelectionUi() {
    int selected = 0;
    int recPending = 0;
    for (const Item& item : items_) {
        if (!item.supported || !item.needsChange) continue;
        if (item.recommended) ++recPending;
        if (item.selected) ++selected;
    }
    if (summaryText_) SetWindowTextW(summaryText_, SelectedSummaryText().c_str());
    bool idle = !busy_.load();
    if (applyButton_) EnableWindow(applyButton_, idle && selected > 0);
    if (defaultButton_) EnableWindow(defaultButton_, idle && recPending > 0);
    if (dryRunCheck_) {
        EnableWindow(dryRunCheck_, idle);
        Button_SetCheck(dryRunCheck_, dryRun_ ? BST_CHECKED : BST_UNCHECKED);
    }
}

// ---------------------------------------------------------------------------
// Status, log e progresso
// ---------------------------------------------------------------------------

void App::SetStatus(const std::wstring& text) {
    if (statusText_) SetWindowTextW(statusText_, text.c_str());
}

void App::AppendLogLine(const std::wstring& line) {
    if (!logEdit_) return;
    int length = GetWindowTextLengthW(logEdit_);
    if (length > 900000) {
        SetWindowTextW(logEdit_, L"");
        length = 0;
    }
    std::wstring text = line + L"\r\n";
    SendMessageW(logEdit_, EM_SETSEL, length, length);
    SendMessageW(logEdit_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageW(logEdit_, EM_SCROLLCARET, 0, 0);
}

void App::SetProgressMarquee(bool on) {
    if (!progress_) return;
    SendMessageW(progress_, PBM_SETMARQUEE, on ? TRUE : FALSE, on ? 30 : 0);
}

void App::SetProgress(int current, int total) {
    if (!progress_) return;
    if (total > 0) {
        SendMessageW(progress_, PBM_SETRANGE32, 0, total);
        SendMessageW(progress_, PBM_SETPOS, current, 0);
    }
}

void App::SetBusyUi(bool busy, bool applying) {
    busy_.store(busy);
    applying_ = applying;
    if (tab_) EnableWindow(tab_, !busy);
    for (int i = 0; i < 4; ++i) {
        Page& page = pages_[i];
        if (page.list) EnableWindow(page.list, !busy);
        if (page.search) EnableWindow(page.search, !busy);
        if (page.combo) EnableWindow(page.combo, !busy);
        if (page.btnRecommended) EnableWindow(page.btnRecommended, !busy);
        if (page.btnAll) EnableWindow(page.btnAll, !busy);
        if (page.btnClear) EnableWindow(page.btnClear, !busy);
    }
    if (rescanButton_) EnableWindow(rescanButton_, !busy);
    if (settingsButton_) EnableWindow(settingsButton_, !busy);
    if (aboutButton_) EnableWindow(aboutButton_, !busy);
    if (applyButton_) EnableWindow(applyButton_, !busy);
    if (defaultButton_) EnableWindow(defaultButton_, !busy);
    if (dryRunCheck_) EnableWindow(dryRunCheck_, !busy);
    for (HWND button : profileButtons_) EnableWindow(button, !busy);
    for (HWND button : diagButtons_) EnableWindow(button, !busy);
    for (HWND button : logButtons_) EnableWindow(button, !busy);

    if (cancelButton_) {
        ShowWindow(cancelButton_, busy && applying ? SW_SHOW : SW_HIDE);
        EnableWindow(cancelButton_, busy && applying && !cancelRequested_.load());
    }
    if (progress_) ShowWindow(progress_, busy ? SW_SHOW : SW_HIDE);
    if (busy) {
        if (applying) {
            SetProgressMarquee(false);
            SetProgress(0, 100);
        } else {
            SetProgressMarquee(true);
        }
    } else {
        SetProgressMarquee(false);
        cancelRequested_.store(false);
    }
    Layout();
    UpdateSelectionUi();
}

void App::UpdateDiagnostics() {
    if (!diagEdit_) return;
    std::wstring text;
    text += L"Windows Lite Restore — " + std::wstring(Tr(STR_TAB_DIAG)) + L"\r\n";
    text += L"Gerado em: " + TimestampNow(false) + L"\r\n";
    text += L"Modo: " + std::wstring(admin_ ? L"administrador" : L"somente leitura") + L"\r\n";
    text += L"Perfil: " + std::wstring(TrProfileName(static_cast<int>(activeProfile_))) + L"\r\n";
    text += L"Descrição: " + std::wstring(TrProfileDescription(static_cast<int>(activeProfile_))) + L"\r\n";
    text += L"\r\n";
    text += GetSystemSummary();

    text += L"\r\nConfigurações:\r\n";
    text += L"  Fonte Windows Update: " + BoolText(settings_.useWindowsUpdate) + L"\r\n";
    text += L"  Pasta de origem: " +
            (settings_.sourcePath.empty() ? L"(padrão)" : settings_.sourcePath) + L"\r\n";
    text += L"  Ponto de restauração: " + BoolText(settings_.createRestorePoint) + L"\r\n";
    text += L"  Backup reversível: " + BoolText(settings_.createBackup) + L"\r\n";
    text += L"  Análise automática: " + BoolText(settings_.autoScan) + L"\r\n";

    int counts[4] = {};
    int pending[4] = {};
    int recommended = 0;
    for (const Item& item : items_) {
        counts[PageTabIndex(item.kind)]++;
        if (item.supported && item.needsChange) pending[PageTabIndex(item.kind)]++;
        if (item.supported && item.needsChange && item.recommended) ++recommended;
    }
    text += L"\r\nItens analisados:\r\n";
    const wchar_t* names[4] = {Tr(STR_KIND_FEATURE), Tr(STR_KIND_SERVICE), Tr(STR_KIND_POLICY),
                               Tr(STR_KIND_TASK)};
    for (int i = 0; i < 4; ++i) {
        text += L"  " + std::wstring(names[i]) + L": " + FormatInt(counts[i]) + L" (" +
                FormatInt(pending[i]) + L" pendente(s))\r\n";
    }
    text += L"  Pendências recomendadas: " + FormatInt(recommended) + L"\r\n";
    if (!lastScanSummary_.empty()) text += L"\r\n" + lastScanSummary_ + L"\r\n";
    if (!featureScanError_.empty()) text += L"\r\nRecursos: " + featureScanError_ + L"\r\n";

    text += L"\r\nCaminhos:\r\n";
    text += L"  Log: " + (logPath_.empty() ? L"(—)" : logPath_) + L"\r\n";
    text += L"  Backups: " + MakeDataPath(L"Backups") + L"\r\n";
    text += L"  Relatórios: " + MakeDataPath(L"reports") + L"\r\n";
    if (!lastBackupPath_.empty()) text += L"  Último backup: " + lastBackupPath_ + L"\r\n";
    if (!lastReportPath_.empty()) text += L"  Último relatório: " + lastReportPath_ + L"\r\n";

    text += L"\r\nObservações:\r\n";
    text += L"  • O aplicativo nunca desativa serviços ou recursos; apenas restaura o que for marcado.\r\n";
    text += L"  • Recursos removidos da imagem exigem mídia/ISO de origem compatível.\r\n";
    text += L"  • Faça um backup reversível antes de aplicar alterações.\r\n";
    SetWindowTextW(diagEdit_, text.c_str());
}

// ---------------------------------------------------------------------------
// Procedimentos de janela
// ---------------------------------------------------------------------------

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<App*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        if (app) app->hwnd_ = hwnd;
    }
    if (!app) return DefWindowProcW(hwnd, msg, wParam, lParam);
    if (msg == WM_NCDESTROY) {
        app->hwnd_ = nullptr;
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return app->HandleMessage(msg, wParam, lParam);
}

LRESULT CALLBACK App::DetailWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<App*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        return 0;
    }
    if (!app) return DefWindowProcW(hwnd, msg, wParam, lParam);
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            app->PaintDetailPanel(dc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT App::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd_, &ps);
            RECT rc = {};
            GetClientRect(hwnd_, &rc);
            FillRect(dc, &rc, brushBg_);
            PaintHeader(dc, rc);
            int statusH = Scale(26);
            RECT statusRect = {rc.left, rc.bottom - statusH, rc.right, rc.bottom};
            FillRect(dc, &statusRect, brushWhite_);
            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) Layout();
            return 0;
        case WM_DPICHANGED: {
            dpi_ = static_cast<int>(HIWORD(wParam));
            if (dpi_ <= 0) dpi_ = 96;
            InitFonts();
            ApplyTheme();
            Layout();
            RECT* suggested = reinterpret_cast<RECT*>(lParam);
            if (suggested) {
                SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = Scale(1000);
            mmi->ptMinTrackSize.y = Scale(660);
            return 0;
        }
        case WM_COMMAND:
            HandleCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
            return 0;
        case WM_NOTIFY: {
            LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
            if (!hdr) return 0;
            if (hdr->code == NM_CUSTOMDRAW) {
                for (int i = 0; i < 4; ++i) {
                    if (pages_[i].list == hdr->hwndFrom) {
                        LPNMLVCUSTOMDRAW cd = reinterpret_cast<LPNMLVCUSTOMDRAW>(hdr);
                        if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
                        if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                            Item* item = reinterpret_cast<Item*>(cd->nmcd.lItemlParam);
                            if (item) {
                                if (!item->supported) cd->clrText = RGB(150, 160, 175);
                                else if (item->essential) cd->clrText = RGB(16, 80, 150);
                                else if (!item->needsChange) cd->clrText = kColorMuted;
                                else cd->clrText = kColorText;
                            }
                            return CDRF_NEWFONT;
                        }
                        return CDRF_DODEFAULT;
                    }
                }
                return CDRF_DODEFAULT;
            }
            HandleNotify(hdr);
            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, kColorText);
            HWND control = reinterpret_cast<HWND>(lParam);
            if (control == statusText_) return reinterpret_cast<LRESULT>(brushWhite_);
            return reinterpret_cast<LRESULT>(brushBg_);
        }
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkColor(dc, kColorWhite);
            SetTextColor(dc, kColorText);
            return reinterpret_cast<LRESULT>(brushWhite_);
        }
        case WM_CTLCOLORBTN:
            return reinterpret_cast<LRESULT>(brushBg_);
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkColor(dc, kColorWhite);
            return reinterpret_cast<LRESULT>(brushWhite_);
        }
        case WM_APP_LOG: {
            std::wstring* line = reinterpret_cast<std::wstring*>(lParam);
            if (line) {
                AppendLogLine(*line);
                delete line;
            }
            return 0;
        }
        case WM_APP_STATUS: {
            std::wstring* text = reinterpret_cast<std::wstring*>(lParam);
            if (text) {
                SetStatus(*text);
                delete text;
            }
            return 0;
        }
        case WM_APP_SCAN_DONE:
        case WM_APP_SCAN_FAIL: {
            ScanOutcome* outcome = reinterpret_cast<ScanOutcome*>(lParam);
            FinishScan(outcome);
            return 0;
        }
        case WM_APP_APPLY_PROG: {
            SetProgress(static_cast<int>(wParam), static_cast<int>(lParam));
            return 0;
        }
        case WM_APP_DISM_PROG: {
            SetProgress(static_cast<int>(wParam), 100);
            SetStatus(L"Instalando recurso... " + FormatInt(static_cast<int>(wParam)) + L"%");
            return 0;
        }
        case WM_APP_APPLY_DONE: {
            ApplySummary* summary = reinterpret_cast<ApplySummary*>(lParam);
            FinishApply(summary);
            return 0;
        }
        case WM_SETCURSOR: {
            if (busy_.load()) {
                SetCursor(LoadCursorW(nullptr, IDC_WAIT));
                return TRUE;
            }
            if (LOWORD(lParam) == HTCLIENT && !admin_) {
                POINT point;
                GetCursorPos(&point);
                ScreenToClient(hwnd_, &point);
                if (PtInRect(&elevateRect_, point)) {
                    SetCursor(LoadCursorW(nullptr, IDC_HAND));
                    return TRUE;
                }
            }
            break;
        }
        case WM_MOUSEMOVE: {
            if (!admin_) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                POINT point = {x, y};
                bool inside = PtInRect(&elevateRect_, point) != FALSE;
                if (inside != elevateHover_) {
                    elevateHover_ = inside;
                    RECT header = {0, 0, 0, Scale(92)};
                    RECT rc = {};
                    GetClientRect(hwnd_, &rc);
                    header.right = rc.right;
                    InvalidateRect(hwnd_, &header, FALSE);
                    if (inside) TrackMouseEvent(&trackMouse_);
                }
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            if (elevateHover_) {
                elevateHover_ = false;
                RECT header = {0, 0, 0, Scale(92)};
                RECT rc = {};
                GetClientRect(hwnd_, &rc);
                header.right = rc.right;
                InvalidateRect(hwnd_, &header, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (!admin_) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                POINT point = {x, y};
                if (PtInRect(&elevateRect_, point)) HandleCommand(ID_ELEVATE, 0, nullptr);
            }
            return 0;
        }
        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_F5:
                    if (!busy_.load()) StartScan(true);
                    return 0;
                case VK_F1:
                    ShowAboutDialog();
                    return 0;
                case VK_ESCAPE:
                    if (busy_.load() && applying_) HandleCommand(kCancelId, 0, nullptr);
                    return 0;
            }
            if (wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000)) {
                if (!busy_.load()) OnApplyClicked(false);
                return 0;
            }
            break;
        }
        case WM_CLOSE: {
            if (busy_.load()) {
                MessageBoxW(hwnd_, L"Aguarde a conclusão da operação em andamento antes de fechar.",
                            L"Windows Lite Restore", MB_ICONINFORMATION | MB_OK);
                return 0;
            }
            SaveSettings();
            closing_ = true;
            DestroyWindow(hwnd_);
            return 0;
        }
        case WM_DESTROY: {
            closing_ = true;
            PostQuitMessage(0);
            return 0;
        }
        case WM_QUERYENDSESSION:
            return TRUE;
    }
    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Comandos
// ---------------------------------------------------------------------------

namespace {
struct ScanThreadParam {
    App* app;
};
struct ApplyThreadParam {
    App* app;
    std::vector<Item*> plan;
};
}  // namespace

void App::HandleCommand(WORD id, WORD code, HWND control) {
    (void)control;
    if (id >= kPageBase && id < kPageBase + 40) {
        int rel = static_cast<int>(id) - kPageBase;
        int kind = rel / 10;
        int off = rel % 10;
        if (kind < 0 || kind > 3) return;
        Page& page = pages_[kind];
        if (off == 1 && code == EN_CHANGE) {
            if (!updatingList_) RefreshPage(page);
            return;
        }
        if (off == 2 && code == CBN_SELCHANGE) {
            RefreshPage(page);
            return;
        }
        if (code == BN_CLICKED) {
            if (off == 4) SelectPageRecommended(page);
            else if (off == 5) SelectPageAll(page);
            else if (off == 6) ClearPage(page);
        }
        return;
    }

    switch (id) {
        case ID_SCAN:
            if (!busy_.load()) StartScan(true);
            break;
        case ID_APPLY:
            OnApplyClicked(false);
            break;
        case ID_RESTORE_DEFAULT:
            OnApplyClicked(true);
            break;
        case ID_SETTINGS:
            if (!busy_.load()) ShowSettingsDialog();
            break;
        case ID_ABOUT:
            ShowAboutDialog();
            break;
        case ID_EXIT:
            PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            break;
        case ID_DRYRUN:
            dryRun_ = dryRunCheck_ && Button_GetCheck(dryRunCheck_) == BST_CHECKED;
            UpdateSelectionUi();
            break;
        case ID_PROFILE:
        case ID_PROFILE + 1:
        case ID_PROFILE + 2:
        case ID_PROFILE + 3: {
            if (code != BN_CLICKED) break;
            int index = static_cast<int>(id) - ID_PROFILE;
            if (index < 0 || index > 3) break;
            Profile profile = static_cast<Profile>(index);
            if (profile == activeProfile_) break;
            activeProfile_ = profile;
            RecomputeServiceTargets();
            ApplyProfileSelection();
            for (int i = 0; i < 4; ++i) RefreshPage(pages_[i]);
            RefreshAllCounts();
            UpdateFooterLabels();
            UpdateProfileButtons();
            UpdateDiagnostics();
            SetStatus(std::wstring(TrProfileName(static_cast<int>(activeProfile_))) + L" — " +
                      TrProfileDescription(static_cast<int>(activeProfile_)));
            break;
        }
        case ID_ELEVATE: {
            if (admin_) break;
            SetStatus(L"Solicitando elevação...");
            if (ElevateSelf(true)) {
                closing_ = true;
                DestroyWindow(hwnd_);
            } else {
                MessageBoxW(hwnd_, L"Não foi possível iniciar o aplicativo com privilégios de administrador.",
                            L"Windows Lite Restore", MB_ICONWARNING | MB_OK);
            }
            break;
        }
        case ID_OPEN_LOGS:
        case IDC_DIAG_LOGS:
        case IDC_LOG_FOLDER: {
            std::wstring dir = ParentPath(logPath_);
            if (dir.empty()) dir = MakeUserPath(L"logs");
            OpenFolder(dir);
            break;
        }
        case ID_OPEN_BACKUPS:
        case IDC_DIAG_BACKUPS:
            OpenFolder(MakeDataPath(L"Backups"));
            break;
        case ID_UNDO:
        case IDC_DIAG_UNDO:
            OnUndoClicked();
            break;
        case ID_SAVE_REPORT:
        case IDC_DIAG_SAVE:
            WriteDiagnosticReportToFile();
            break;
        case ID_SAVE_LOG:
        case IDC_LOG_SAVE:
            SaveLogAs();
            break;
        case ID_CLEAR_LOG:
        case IDC_LOG_CLEAR:
            SetWindowTextW(logEdit_, L"");
            SetStatus(L"Visualização limpa; o arquivo de log não foi alterado.");
            break;
        case IDC_DIAG_REFRESH:
            UpdateDiagnostics();
            break;
        case kCancelId: {
            if (busy_.load() && applying_) {
                cancelRequested_.store(true);
                if (cancelEvent_) SetEvent(cancelEvent_);
                if (cancelButton_) EnableWindow(cancelButton_, FALSE);
                SetStatus(L"Cancelando... aguarde a conclusão.");
            }
            break;
        }
    }
}

void App::HandleNotify(LPNMHDR header) {
    if (!header) return;
    if (header->idFrom == static_cast<UINT_PTR>(kTabId) && header->code == TCN_SELCHANGE) {
        ShowTab(TabCtrl_GetCurSel(tab_));
        return;
    }
    for (int i = 0; i < 4; ++i) {
        if (pages_[i].list == header->hwndFrom) {
            HandleListNotify(header);
            return;
        }
    }
}

void App::HandleListNotify(LPNMHDR header) {
    if (!header || header->code != LVN_ITEMCHANGED) return;
    for (int i = 0; i < 4; ++i) {
        if (pages_[i].list == header->hwndFrom) {
            HandleListChanged(pages_[i], reinterpret_cast<NMLISTVIEW*>(header));
            return;
        }
    }
}

void App::HandleListChanged(Page& page, LPNMLISTVIEW info) {
    if (updatingList_ || !info) return;
    if (info->iItem < 0 || static_cast<size_t>(info->iItem) >= page.visible.size()) return;
    Item* item = page.visible[info->iItem];
    if (!item) return;

    bool checkChanged = (info->uChanged & LVIF_STATE) &&
                        ((info->uOldState ^ info->uNewState) & LVIS_STATEIMAGEMASK) != 0;
    bool selectionChanged = (info->uChanged & LVIF_STATE) &&
                            ((info->uOldState ^ info->uNewState) & LVIS_SELECTED) != 0;

    if (checkChanged) {
        bool checked = IsListChecked(page.list, info->iItem);
        if (checked && (!item->supported || !item->needsChange)) {
            updatingList_ = true;
            ListView_SetCheckState(page.list, info->iItem, FALSE);
            updatingList_ = false;
            item->selected = false;
        } else {
            item->selected = checked;
        }
        UpdateDetails(item);
        UpdatePageCount(page);
        UpdateSelectionUi();
    } else if (selectionChanged && (info->uNewState & LVIS_SELECTED)) {
        UpdateDetails(item);
    }
}

// ---------------------------------------------------------------------------
// Análise em segundo plano
// ---------------------------------------------------------------------------

void App::StartScan(bool autoSelectRecommended) {
    if (busy_.load()) return;
    scanAutoSelect_ = autoSelectRecommended;
    detailItem_ = nullptr;
    if (detailPanel_) InvalidateRect(detailPanel_, nullptr, TRUE);
    updatingList_ = true;
    for (int i = 0; i < 4; ++i) {
        pages_[i].visible.clear();
        if (pages_[i].list) ListView_DeleteAllItems(pages_[i].list);
    }
    updatingList_ = false;
    SetStatus(Tr(STR_STATUS_ANALYZE));
    SetBusyUi(true, false);
    StartWorkerScan();
}

void App::StartWorkerScan() {
    auto* param = new ScanThreadParam;
    param->app = this;
    scanThread_ = CreateThread(nullptr, 0, App::ScanThreadProc, param, 0, nullptr);
    if (!scanThread_) {
        delete param;
        SetBusyUi(false, false);
        SetStatus(L"Não foi possível iniciar a análise.");
        MessageBoxW(hwnd_, L"Não foi possível iniciar a análise do sistema.",
                    L"Windows Lite Restore", MB_ICONERROR | MB_OK);
    }
}

DWORD WINAPI App::ScanThreadProc(LPVOID param) {
    auto* p = static_cast<ScanThreadParam*>(param);
    App* app = p->app;
    delete p;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ScanOutcome* outcome = new ScanOutcome();
    try {
        outcome->ok = app->ScanAll(&outcome->error);
        outcome->featuresOk = app->featureScanError_.empty();
    } catch (...) {
        outcome->ok = false;
        outcome->error = L"Falha inesperada durante a análise.";
    }
    CoUninitialize();
    if (app->hwnd_) {
        PostMessageW(app->hwnd_, WM_APP_SCAN_DONE, 0, reinterpret_cast<LPARAM>(outcome));
    } else {
        delete outcome;
    }
    return 0;
}

void App::FinishScan(ScanOutcome* outcome) {
    if (scanThread_) {
        CloseHandle(scanThread_);
        scanThread_ = nullptr;
    }
    SetBusyUi(false, false);
    if (!outcome) {
        SetStatus(L"Falha na análise.");
        return;
    }
    bool ok = outcome->ok;
    std::wstring error = outcome->error;
    delete outcome;
    if (!ok) {
        SetStatus(L"Falha na análise: " + error);
        MessageBoxW(hwnd_, (L"A análise falhou:\r\n\r\n" + error).c_str(),
                    L"Windows Lite Restore", MB_ICONERROR | MB_OK);
        UpdateDiagnostics();
        return;
    }
    firstScan_ = false;
    PopulatePages(scanAutoSelect_);
    if (!resumeSelectionPath_.empty()) {
        LoadSelectionFile(resumeSelectionPath_);
        for (int i = 0; i < 4; ++i) RefreshPage(pages_[i]);
        resumeSelectionPath_.clear();
        RefreshAllCounts();
    }
    SetStatus(lastScanSummary_);
}

// ---------------------------------------------------------------------------
// Aplicação em segundo plano
// ---------------------------------------------------------------------------

void App::OnApplyClicked(bool recommendedProfile) {
    if (busy_.load()) return;
    dryRun_ = dryRunCheck_ && Button_GetCheck(dryRunCheck_) == BST_CHECKED;

    std::vector<Item*> plan = BuildPlan(recommendedProfile);
    if (plan.empty()) {
        if (recommendedProfile) {
            MessageBoxW(hwnd_, L"Não há pendências recomendadas para restaurar neste momento.",
                        L"Windows Lite Restore", MB_ICONINFORMATION | MB_OK);
        } else {
            MessageBoxW(hwnd_, L"Nenhum item está selecionado.\r\nMarque os itens na lista ou use o botão Recomendados.",
                        L"Windows Lite Restore", MB_ICONINFORMATION | MB_OK);
        }
        return;
    }

    if (!admin_ && !dryRun_) {
        int answer = MessageBoxW(
            hwnd_, L"Esta ação exige privilégios de administrador.\r\n\r\nDeseja reiniciar o aplicativo elevado? Sua seleção será preservada.",
            L"Elevação necessária", MB_ICONINFORMATION | MB_YESNO);
        if (answer == IDYES) {
            if (ElevateSelf(true)) {
                closing_ = true;
                DestroyWindow(hwnd_);
            } else {
                MessageBoxW(hwnd_, L"Não foi possível iniciar o aplicativo com privilégios de administrador.",
                            L"Windows Lite Restore", MB_ICONWARNING | MB_OK);
            }
        }
        return;
    }

    ConfirmOptions options;
    options.createRestorePoint = settings_.createRestorePoint;
    options.createBackup = settings_.createBackup;
    options.accepted = false;
    ShowApplyConfirmation(plan, &options, recommendedProfile);
    if (!options.accepted) return;

    applyRestorePoint_ = options.createRestorePoint;
    applyBackup_ = options.createBackup;
    applyDryRun_ = dryRun_;
    StartApply(plan);
}

void App::StartApply(const std::vector<Item*>& plan) {
    if (busy_.load()) return;
    if (!cancelEvent_) cancelEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    else ResetEvent(cancelEvent_);
    cancelRequested_.store(false);
    SetBusyUi(true, true);
    SetProgress(0, static_cast<int>(plan.size()));
    SetStatus(L"Preparando aplicação...");
    StartWorkerApply(plan);
}

void App::StartWorkerApply(const std::vector<Item*>& plan) {
    auto* param = new ApplyThreadParam;
    param->app = this;
    param->plan = plan;
    applyThread_ = CreateThread(nullptr, 0, App::ApplyThreadProc, param, 0, nullptr);
    if (!applyThread_) {
        delete param;
        SetBusyUi(false, false);
        SetStatus(L"Não foi possível iniciar a aplicação.");
        MessageBoxW(hwnd_, L"Não foi possível iniciar a aplicação das alterações.",
                    L"Windows Lite Restore", MB_ICONERROR | MB_OK);
    }
}

DWORD WINAPI App::ApplyThreadProc(LPVOID param) {
    auto* p = static_cast<ApplyThreadParam*>(param);
    App* app = p->app;
    std::vector<Item*> plan = std::move(p->plan);
    delete p;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ApplySummary* summary = new ApplySummary();
    summary->dryRun = app->applyDryRun_;
    try {
        if (!summary->dryRun) {
            if (app->applyRestorePoint_) {
                app->PostStatus(L"Criando ponto de restauração...");
                std::wstring error;
                if (!app->CreateRestorePoint(&error)) {
                    summary->messages.push_back(L"[AVISO] " + error);
                    app->Log(L"[AVISO] " + error);
                }
            }
            if (app->applyBackup_) {
                app->PostStatus(L"Criando backup reversível...");
                std::wstring error;
                std::wstring path = app->CreateBackup(plan, &error);
                if (path.empty()) {
                    summary->failed++;
                    summary->messages.push_back(L"[FALHA] " + error);
                    app->Log(L"[FALHA] " + error);
                    summary->logPath = app->logPath_;
                } else {
                    summary->backupPath = path;
                }
            }
        }
        if (summary->failed == 0) {
            app->ApplyPlan(plan, summary->dryRun, summary, app->cancelEvent_);
        }
    } catch (...) {
        summary->failed++;
        summary->messages.push_back(L"[FALHA] Exceção inesperada durante a aplicação.");
    }
    CoUninitialize();
    if (app->hwnd_) {
        PostMessageW(app->hwnd_, WM_APP_APPLY_DONE, 0, reinterpret_cast<LPARAM>(summary));
    } else {
        delete summary;
    }
    return 0;
}

void App::FinishApply(ApplySummary* summary) {
    if (applyThread_) {
        CloseHandle(applyThread_);
        applyThread_ = nullptr;
    }
    if (cancelEvent_) {
        CloseHandle(cancelEvent_);
        cancelEvent_ = nullptr;
    }
    cancelRequested_.store(false);
    SetBusyUi(false, false);

    if (!summary) {
        SetStatus(L"Operação concluída.");
        return;
    }

    std::wstring title;
    if (summary->dryRun) title = L"Simulação concluída";
    else if (summary->cancelled) title = L"Aplicação cancelada";
    else title = L"Aplicação concluída";

    std::wstring body = summary->dryRun
                            ? L"Nenhuma alteração foi feita no sistema."
                            : L"Alterações processadas.";
    body += L"\r\nSucessos: " + FormatInt(summary->success);
    body += L"\r\nFalhas: " + FormatInt(summary->failed);
    body += L"\r\nIgnorados: " + FormatInt(summary->skipped);
    if (!summary->backupPath.empty()) body += L"\r\nBackup: " + summary->backupPath;
    if (!summary->reportPath.empty()) body += L"\r\nRelatório: " + summary->reportPath;
    if (summary->rebootRequired) {
        body += L"\r\n\r\nRecomenda-se reiniciar o Windows para concluir as alterações.";
    }

    std::wstring reportPath = summary->reportPath;
    bool needRescan = !summary->dryRun && summary->success > 0;
    bool hadFailures = summary->failed > 0;
    delete summary;

    UINT box = hadFailures ? MB_ICONWARNING : MB_ICONINFORMATION;
    if (reportPath.empty()) box |= MB_OK;
    else {
        box |= MB_YESNO;
        body += L"\r\n\r\nDeseja abrir o relatório agora?";
    }
    int answer = MessageBoxW(hwnd_, body.c_str(), title.c_str(), box);
    if (answer == IDYES && !reportPath.empty()) OpenFolder(reportPath);

    if (needRescan) StartScan(false);
    else UpdateDiagnostics();
}

// ---------------------------------------------------------------------------
// Desfazer
// ---------------------------------------------------------------------------

void App::OnUndoClicked() {
    if (busy_.load()) return;
    if (!admin_) {
        int answer = MessageBoxW(
            hwnd_, L"Desfazer a última execução exige privilégios de administrador.\r\n\r\nDeseja reiniciar o aplicativo elevado?",
            L"Elevação necessária", MB_ICONINFORMATION | MB_YESNO);
        if (answer == IDYES) {
            if (ElevateSelf(true)) {
                closing_ = true;
                DestroyWindow(hwnd_);
            }
        }
        return;
    }

    int answer = MessageBoxW(
        hwnd_, L"Desfazer a última execução?\r\n\r\nServiços, políticas e tarefas serão revertidos para o estado registrado no backup. Recursos opcionais instalados não serão removidos.",
        L"Desfazer última execução", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
    if (answer != IDYES) return;

    SetBusyUi(true, false);
    SetStatus(L"Revertendo último backup...");
    std::wstring summary;
    std::wstring error;
    bool ok = UndoLatestBackup(&summary, &error);
    SetBusyUi(false, false);
    if (ok) {
        UpdateDiagnostics();
        MessageBoxW(hwnd_, summary.c_str(), L"Desfazer concluído", MB_ICONINFORMATION | MB_OK);
        StartScan(false);
    } else {
        MessageBoxW(hwnd_, error.c_str(), L"Desfazer", MB_ICONWARNING | MB_OK);
    }
}

// ---------------------------------------------------------------------------
// Abrir pastas e salvar arquivos
// ---------------------------------------------------------------------------

void App::OpenFolder(const std::wstring& path) {
    if (path.empty()) return;
    std::wstring target = path;
    if (!FileExists(target)) {
        EnsureDirectory(target);
    }
    HINSTANCE result = ShellExecuteW(hwnd_, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        MessageBoxW(hwnd_, (L"Não foi possível abrir:\r\n" + target).c_str(),
                    L"Windows Lite Restore", MB_ICONWARNING | MB_OK);
    }
}

bool SaveFileDialog(HWND owner, const wchar_t* defaultName, std::wstring* out) {
    if (!out) return false;
    wchar_t file[MAX_PATH * 2] = {};
    wcscpy_s(file, defaultName);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Arquivos de texto (*.txt)\0*.txt\0Todos os arquivos (*.*)\0*.*\0\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetSaveFileNameW(&ofn)) {
        *out = file;
        return true;
    }
    return false;
}

void App::SaveLogAs() {
    std::wstring path;
    if (!SaveFileDialog(hwnd_, L"restore-log.txt", &path)) return;
    std::wstring text;
    if (logCsInit_) EnterCriticalSection(&logCs_);
    text = logBuffer_;
    if (logCsInit_) LeaveCriticalSection(&logCs_);
    if (text.empty() && FileExists(logPath_)) ReadTextFile(logPath_, &text);
    if (WriteTextFile(path, text)) {
        MessageBoxW(hwnd_, (L"Log salvo em:\r\n" + path).c_str(), L"Windows Lite Restore",
                    MB_ICONINFORMATION | MB_OK);
    } else {
        MessageBoxW(hwnd_, L"Não foi possível salvar o arquivo de log.",
                    L"Windows Lite Restore", MB_ICONWARNING | MB_OK);
    }
}

void App::WriteDiagnosticReportToFile() {
    std::wstring path;
    if (!SaveFileDialog(hwnd_, L"diagnostico-windows-lite.txt", &path)) return;
    std::wstring report;
    BuildReport(&report, nullptr);
    if (WriteTextFile(path, report)) {
        MessageBoxW(hwnd_, (L"Relatório salvo em:\r\n" + path).c_str(), L"Windows Lite Restore",
                    MB_ICONINFORMATION | MB_OK);
    } else {
        MessageBoxW(hwnd_, L"Não foi possível salvar o relatório.",
                    L"Windows Lite Restore", MB_ICONWARNING | MB_OK);
    }
}

// ---------------------------------------------------------------------------
// Diálogo de confirmação
// ---------------------------------------------------------------------------

namespace {
struct ConfirmContext {
    const std::vector<Item*>* plan;
    ConfirmOptions* options;
    bool recommended;
    bool dryRun;
    int features;
    std::wstring summary;
};

void UpdateConfirmNote(HWND dialog, ConfirmContext* ctx) {
    if (!ctx) return;
    std::wstring note;
    if (ctx->dryRun) {
        note = L"Nenhuma alteração será feita no sistema.";
    } else {
        bool restore = Button_GetCheck(GetDlgItem(dialog, IDC_CONFIRM_RESTORE)) == BST_CHECKED;
        bool backup = Button_GetCheck(GetDlgItem(dialog, IDC_CONFIRM_BACKUP)) == BST_CHECKED;
        if (restore) note += L"O ponto de restauração exige a Proteção do Sistema ativa; se falhar, o processo continua.\r\n";
        if (backup) note += L"O backup reversível permite desfazer serviços, políticas e tarefas.\r\n";
        if (!restore && !backup) note += L"Atenção: você desativou o ponto de restauração e o backup. Continue por sua conta e risco.\r\n";
        if (ctx->features > 0) note += L"Recursos opcionais usam o DISM e podem exigir mídia/ISO de origem e reinicialização.";
    }
    SetDlgItemTextW(dialog, IDC_CONFIRM_NOTE, note.c_str());
}

INT_PTR CALLBACK ConfirmDlgProc(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) {
    ConfirmContext* ctx = reinterpret_cast<ConfirmContext*>(GetWindowLongPtrW(dialog, DWLP_USER));
    switch (msg) {
        case WM_INITDIALOG:
            ctx = reinterpret_cast<ConfirmContext*>(lp);
            SetWindowLongPtrW(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));
            SetDialogIcon(dialog);
            SetDlgItemTextW(dialog, IDC_CONFIRM_SUMMARY, ctx->summary.c_str());
            SendDlgItemMessageW(dialog, IDC_CONFIRM_SUMMARY, EM_SETSEL, 0, 0);
            if (ctx->dryRun) {
                SetWindowTextW(dialog, L"Simular restauração");
                SetDlgItemTextW(dialog, IDOK, L"Simular");
                Button_SetCheck(GetDlgItem(dialog, IDC_CONFIRM_RESTORE), BST_UNCHECKED);
                Button_SetCheck(GetDlgItem(dialog, IDC_CONFIRM_BACKUP), BST_UNCHECKED);
                EnableWindow(GetDlgItem(dialog, IDC_CONFIRM_RESTORE), FALSE);
                EnableWindow(GetDlgItem(dialog, IDC_CONFIRM_BACKUP), FALSE);
            } else {
                Button_SetCheck(GetDlgItem(dialog, IDC_CONFIRM_RESTORE),
                                ctx->options->createRestorePoint ? BST_CHECKED : BST_UNCHECKED);
                Button_SetCheck(GetDlgItem(dialog, IDC_CONFIRM_BACKUP),
                                ctx->options->createBackup ? BST_CHECKED : BST_UNCHECKED);
            }
            UpdateConfirmNote(dialog, ctx);
            CenterDialog(dialog);
            return TRUE;
        case WM_CTLCOLORDLG:
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, kColorText);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        }
        case WM_CTLCOLORBTN:
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, kColorWhite);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        }
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
                    if (ctx) {
                        if (ctx->dryRun) {
                            ctx->options->createRestorePoint = false;
                            ctx->options->createBackup = false;
                        } else {
                            ctx->options->createRestorePoint =
                                Button_GetCheck(GetDlgItem(dialog, IDC_CONFIRM_RESTORE)) == BST_CHECKED;
                            ctx->options->createBackup =
                                Button_GetCheck(GetDlgItem(dialog, IDC_CONFIRM_BACKUP)) == BST_CHECKED;
                        }
                        ctx->options->accepted = true;
                        EndDialog(dialog, IDOK);
                    }
                    return TRUE;
                case IDCANCEL:
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                case IDC_CONFIRM_RESTORE:
                case IDC_CONFIRM_BACKUP:
                    UpdateConfirmNote(dialog, ctx);
                    return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(dialog, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}
}  // namespace

void App::ShowApplyConfirmation(const std::vector<Item*>& plan, ConfirmOptions* options,
                                bool recommendedProfile) {
    int feat = 0, serv = 0, pol = 0, task = 0;
    std::wstring featList, servList, polList, taskList;
    auto append = [](std::wstring& out, const std::wstring& line) {
        int count = 1;
        for (wchar_t c : out) {
            if (c == L'\n') ++count;
        }
        if (count < 15) out += line + L"\r\n";
        else if (count == 15) out += L"  … mais itens\r\n";
    };
    for (Item* item : plan) {
        switch (item->kind) {
            case ItemKind::Feature:
                ++feat;
                append(featList, L"  • " + item->name + L" — instalar");
                break;
            case ItemKind::Service:
                ++serv;
                append(servList, L"  • " + item->name + L" — " + item->target);
                break;
            case ItemKind::Policy:
                ++pol;
                append(polList, L"  • " + item->name + L" — " + item->target);
                break;
            case ItemKind::Task:
                ++task;
                append(taskList, L"  • " + item->name + L" — habilitar");
                break;
        }
    }

    std::wstring summary;
    summary += L"Perfil: " + std::wstring(ProfileName(activeProfile_)) + L"\r\n";
    summary += recommendedProfile ? L"Seleção: itens recomendados do perfil\r\n"
                                  : L"Seleção: manual\r\n";
    if (dryRun_) summary += L"Modo: SIMULAÇÃO — nenhuma alteração será feita.\r\n";
    summary += L"\r\nResumo do plano:\r\n";
    if (feat) summary += L"\r\nRecursos (" + FormatInt(feat) + L"):\r\n" + featList;
    if (serv) summary += L"\r\nServiços (" + FormatInt(serv) + L"):\r\n" + servList;
    if (pol) summary += L"\r\nPolíticas (" + FormatInt(pol) + L"):\r\n" + polList;
    if (task) summary += L"\r\nTarefas (" + FormatInt(task) + L"):\r\n" + taskList;
    summary += L"\r\nTotal: " + FormatInt(static_cast<long long>(plan.size())) + L" itens.\r\n";

    ConfirmContext ctx;
    ctx.plan = &plan;
    ctx.options = options;
    ctx.recommended = recommendedProfile;
    ctx.dryRun = dryRun_;
    ctx.features = feat;
    ctx.summary = summary;
    DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_CONFIRM), hwnd_, ConfirmDlgProc,
                    reinterpret_cast<LPARAM>(&ctx));
}

// ---------------------------------------------------------------------------
// Diálogo de configurações
// ---------------------------------------------------------------------------

namespace {
struct SettingsDialogContext {
    Settings settings;
    Language language;
};

void UpdateSettingsNote(HWND dialog) {
    bool wu = Button_GetCheck(GetDlgItem(dialog, IDC_SET_WU)) == BST_CHECKED;
    wchar_t source[4096] = {};
    GetDlgItemTextW(dialog, IDC_SET_SOURCE, source, 4096);
    std::wstring s = Trim(source);
    std::wstring note;
    if (!wu && s.empty()) {
        note = L"Sem Windows Update e sem pasta de origem, a instalação de recursos pode falhar.";
    } else {
        note = L"Deixe vazio para usar o Windows Update e a mídia padrão do sistema.";
    }
    SetDlgItemTextW(dialog, IDC_SET_NOTE, note.c_str());
}

INT_PTR CALLBACK SettingsDlgProc(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) {
    SettingsDialogContext* ctx =
        reinterpret_cast<SettingsDialogContext*>(GetWindowLongPtrW(dialog, DWLP_USER));
    switch (msg) {
        case WM_INITDIALOG:
            ctx = reinterpret_cast<SettingsDialogContext*>(lp);
            SetWindowLongPtrW(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));
            SetDialogIcon(dialog);
            SetWindowTextW(dialog, Tr(STR_BTN_SETTINGS));
            Button_SetCheck(GetDlgItem(dialog, IDC_SET_WU),
                            ctx->settings.useWindowsUpdate ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(dialog, IDC_SET_RESTORE),
                            ctx->settings.createRestorePoint ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(dialog, IDC_SET_BACKUP),
                            ctx->settings.createBackup ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(dialog, IDC_SET_AUTOSCAN),
                            ctx->settings.autoScan ? BST_CHECKED : BST_UNCHECKED);
            SetDlgItemTextW(dialog, IDC_SET_SOURCE, ctx->settings.sourcePath.c_str());
            SetDlgItemTextW(dialog, IDC_SET_LANGUAGE_LABEL, Tr(STR_LANG_LABEL));
            {
                HWND combo = GetDlgItem(dialog, IDC_SET_LANGUAGE);
                for (int i = 0; i < kLanguageCount; ++i) {
                    SendMessageW(combo, CB_ADDSTRING, 0,
                                 reinterpret_cast<LPARAM>(LanguageName(static_cast<Language>(i))));
                }
                ComboBox_SetCurSel(combo, static_cast<int>(ctx->language));
            }
            UpdateSettingsNote(dialog);
            CenterDialog(dialog);
            return TRUE;
        case WM_CTLCOLORDLG:
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, kColorText);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        }
        case WM_CTLCOLORBTN:
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, kColorWhite);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        }
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK: {
                    if (!ctx) break;
                    Settings settings;
                    settings.useWindowsUpdate =
                        Button_GetCheck(GetDlgItem(dialog, IDC_SET_WU)) == BST_CHECKED;
                    settings.createRestorePoint =
                        Button_GetCheck(GetDlgItem(dialog, IDC_SET_RESTORE)) == BST_CHECKED;
                    settings.createBackup =
                        Button_GetCheck(GetDlgItem(dialog, IDC_SET_BACKUP)) == BST_CHECKED;
                    settings.autoScan =
                        Button_GetCheck(GetDlgItem(dialog, IDC_SET_AUTOSCAN)) == BST_CHECKED;
                    wchar_t source[4096] = {};
                    GetDlgItemTextW(dialog, IDC_SET_SOURCE, source, 4096);
                    settings.sourcePath = Trim(source);
                    if (!settings.sourcePath.empty() && !DirectoryExists(settings.sourcePath) &&
                        !FileExists(settings.sourcePath)) {
                        MessageBoxW(dialog,
                                    L"A pasta de origem informada não existe.\r\nVerifique o caminho ou deixe vazio para usar o Windows Update.",
                                    L"Windows Lite Restore", MB_ICONWARNING | MB_OK);
                        return TRUE;
                    }
                    ctx->settings = settings;
                    int langIndex =
                        ComboBox_GetCurSel(GetDlgItem(dialog, IDC_SET_LANGUAGE));
                    if (langIndex >= 0 && langIndex < kLanguageCount) {
                        ctx->language = static_cast<Language>(langIndex);
                    }
                    EndDialog(dialog, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                case IDC_SET_BROWSE: {
                    std::wstring path;
                    if (BrowseForFolder(dialog, &path)) {
                        SetDlgItemTextW(dialog, IDC_SET_SOURCE, path.c_str());
                        UpdateSettingsNote(dialog);
                    }
                    return TRUE;
                }
                case IDC_SET_WU:
                    UpdateSettingsNote(dialog);
                    return TRUE;
                case IDC_SET_SOURCE:
                    if (HIWORD(wp) == EN_CHANGE) UpdateSettingsNote(dialog);
                    return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(dialog, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}
}  // namespace

void App::ShowSettingsDialog() {
    SettingsDialogContext ctx;
    ctx.settings = settings_;
    ctx.language = language_;
    INT_PTR result = DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_SETTINGS), hwnd_,
                                     SettingsDlgProc, reinterpret_cast<LPARAM>(&ctx));
    if (result == IDOK) {
        settings_ = ctx.settings;
        if (ctx.language != language_) {
            language_ = ctx.language;
            SetUiLanguage(language_);
            ApplyLanguage();
            SetStatus(L"");
        }
        SaveSettings();
        UpdateDiagnostics();
        SetStatus(L"Configurações salvas.");
    }
}

// ---------------------------------------------------------------------------
// Diálogo sobre
// ---------------------------------------------------------------------------

namespace {
INT_PTR CALLBACK AboutDlgProc(HWND dialog, UINT msg, WPARAM wp, LPARAM lp) {
    (void)wp;
    (void)lp;
    switch (msg) {
        case WM_INITDIALOG:
            SetDialogIcon(dialog);
            SetWindowTextW(dialog, (std::wstring(Tr(STR_BTN_ABOUT)) + L" — Windows Lite Restore").c_str());
            SetDlgItemTextW(dialog, IDC_ABOUT_VERSION, L"Versão 1.0.0");
            SetDlgItemTextW(dialog, IDC_ABOUT_BUILD, GetWindowsBuildString().c_str());
            CenterDialog(dialog);
            return TRUE;
        case WM_CTLCOLORDLG:
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, kColorText);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        }
        case WM_CTLCOLORBTN:
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(dialog, IDOK);
                    return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(dialog, IDOK);
            return TRUE;
    }
    return FALSE;
}
}  // namespace

void App::ShowAboutDialog() {
    DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_ABOUT), hwnd_, AboutDlgProc, 0);
}
