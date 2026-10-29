#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <deque>
#include <atomic>
#include <cstdint>

#include "resource.h"
#include "i18n.h"

// ---------------------------------------------------------------------------
// Catálogo estático (catalog.cpp)
// ---------------------------------------------------------------------------

enum class StartKind : DWORD {
    Boot = 0,
    System = 1,
    Automatic = 2,
    Manual = 3,
    Disabled = 4
};

// Perfis de otimização de serviços.
enum class Profile {
    MicrosoftDefault = 0,
    Gaming = 1,
    AI = 2,
    Office = 3
};

struct ServiceDef {
    const wchar_t* name;
    const wchar_t* display;
    const wchar_t* category;
    const wchar_t* description;
    DWORD targetStart;
    bool targetDelayed;
    bool recommended;
    bool critical;
    bool optional;
};

struct FeatureDef {
    const wchar_t* name;
    const wchar_t* display;
    const wchar_t* category;
    const wchar_t* description;
    bool microsoftDefaultEnabled;
    bool optionalInstall;
};

struct PolicyDef {
    HKEY root;
    const wchar_t* subkey;
    const wchar_t* value;
    DWORD desired;
    bool desiredExists;
    const wchar_t* display;
    const wchar_t* category;
    const wchar_t* description;
    bool recommended;
};

struct TaskDef {
    const wchar_t* folder;
    const wchar_t* name;
    const wchar_t* display;
    const wchar_t* category;
    const wchar_t* description;
    bool recommended;
};

extern const ServiceDef g_serviceCatalog[];
extern const size_t g_serviceCatalogCount;
extern const FeatureDef g_featureCatalog[];
extern const size_t g_featureCatalogCount;
extern const PolicyDef g_policyCatalog[];
extern const size_t g_policyCatalogCount;
extern const TaskDef g_taskCatalog[];
extern const size_t g_taskCatalogCount;

const FeatureDef* FindFeatureDef(const std::wstring& name);
const ServiceDef* FindServiceDef(const std::wstring& name);
bool IsEssentialService(const std::wstring& name);

const wchar_t* ProfileName(Profile profile);
const wchar_t* ProfileDescription(Profile profile);
bool GetProfileServiceTarget(const std::wstring& name, Profile profile, DWORD* start, bool* delayed);

// ---------------------------------------------------------------------------
// Modelo de itens e páginas
// ---------------------------------------------------------------------------

enum class ItemKind {
    Feature = 0,
    Service = 1,
    Policy = 2,
    Task = 3
};

struct Item {
    ItemKind kind = ItemKind::Feature;
    std::wstring id;
    std::wstring name;
    std::wstring category;
    std::wstring description;
    std::wstring extra;       // nome técnico / caminho / chave
    std::wstring current;     // estado atual legível
    std::wstring target;      // padrão Microsoft legível

    bool present = true;
    bool supported = true;    // pode ser alterado por este aplicativo
    bool needsChange = false; // a ação marcada altera o sistema
    bool recommended = false; // selecionado pelo perfil "Padrão Microsoft"
    bool selected = false;
    bool critical = false;
    bool essential = false;   // essencial para inicialização/logon (não pode ser desativado)
    bool optional = false;

    // Recurso (DISM)
    int featureState = -1;

    // Serviço
    DWORD serviceState = 0;
    DWORD startType = 0;
    bool delayed = false;
    DWORD targetStart = 0;
    bool targetDelayed = false;

    // Política de registro
    HKEY hive = nullptr;
    std::wstring key;
    std::wstring valueName;
    bool currentExists = false;
    DWORD currentType = 0;
    DWORD currentData = 0;
    bool desiredExists = true;
    DWORD desiredData = 0;

    // Tarefa agendada
    std::wstring taskFolder;
    std::wstring taskName;
    bool taskEnabled = false;
};

struct Page {
    ItemKind kind = ItemKind::Feature;
    HWND list = nullptr;
    HWND search = nullptr;
    HWND combo = nullptr;
    HWND count = nullptr;
    HWND btnRecommended = nullptr;
    HWND btnAll = nullptr;
    HWND btnClear = nullptr;
    std::vector<Item*> visible;
    std::vector<std::wstring> categories;
    bool shown = false;
};

struct Settings {
    bool useWindowsUpdate = true;
    std::wstring sourcePath;
    bool createRestorePoint = true;
    bool createBackup = true;
    bool autoScan = true;
};

struct ScanOutcome {
    bool ok = true;
    bool featuresOk = true;
    std::wstring error;
};

struct ApplySummary {
    bool dryRun = false;
    bool cancelled = false;
    bool rebootRequired = false;
    int success = 0;
    int failed = 0;
    int skipped = 0;
    int featuresChanged = 0;
    int servicesChanged = 0;
    int policiesChanged = 0;
    int tasksChanged = 0;
    std::wstring backupPath;
    std::wstring reportPath;
    std::wstring logPath;
    std::vector<std::wstring> messages;
};

struct ConfirmOptions {
    bool createRestorePoint = true;
    bool createBackup = true;
    bool accepted = false;
};

// ---------------------------------------------------------------------------
// Aplicativo
// ---------------------------------------------------------------------------

class App {
public:
    static App& Instance();

    int Run(HINSTANCE instance, const std::wstring& commandLine, int showCmd);

    // Usado pelo modo headless (--scan-report).
    int RunHeadless(const std::wstring& reportPath);

    // Chamado pelo scanner/actions; pode ser usado de threads de trabalho.
    void Log(const std::wstring& text);
    void PostStatus(const std::wstring& text);
    void SetStatus(const std::wstring& text);

    HWND Window() const { return hwnd_; }
    bool IsAdmin() const { return admin_; }
    bool IsReadOnly() const { return readOnly_; }
    bool IsBusy() const { return busy_.load(); }
    HINSTANCE InstanceHandle() const { return instance_; }
    const Settings& GetSettings() const { return settings_; }
    void SaveSettings();

    // Scanner/actions (core.cpp)
    bool ScanAll(std::wstring* error);
    bool ApplyPlan(const std::vector<Item*>& plan, bool dryRun, ApplySummary* summary,
                   HANDLE cancelEvent);
    bool CreateRestorePoint(std::wstring* error);
    std::wstring CreateBackup(const std::vector<Item*>& plan, std::wstring* error);
    bool UndoLatestBackup(std::wstring* summary, std::wstring* error);
    void BuildReport(std::wstring* report, const ApplySummary* summary = nullptr);

    // UI (ui.cpp)
    bool CreateMainWindow(int showCmd);
    void Layout();
    void PopulatePages(bool autoSelectRecommended);
    void RefreshPage(Page& page);
    void RefreshAllCounts();
    void ShowTab(int index);
    void UpdateDetails(Item* item);
    void FocusPageFor(ItemKind kind);
    void RecomputeServiceTargets();
    void ApplyProfileSelection();
    void UpdateFooterLabels();
    void UpdateProfileButtons();
    void ApplyLanguage();
    void SetBusyUi(bool busy, bool applying = false);
    void AppendLogLine(const std::wstring& line);
    void UpdateDiagnostics();
    std::wstring SelectedSummaryText() const;
    std::vector<Item*> BuildPlan(bool recommendedOnly) const;

    // Handlers
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    App();
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void ParseCommandLine(const std::wstring& commandLine);
    bool ElevateSelf(bool preserveSelection);
    bool SaveSelectionFile(std::wstring* path);
    void LoadSelectionFile(const std::wstring& path);
    bool IsElevated() const;
    void LoadSettings();
    void InitLogging();
    void CloseLogging();
    void InitFonts();
    void FreeFonts();
    int Scale(int value) const;
    void CenterWindow();
    void ShowSettingsDialog();
    void ShowAboutDialog();
    void ShowApplyConfirmation(const std::vector<Item*>& plan, ConfirmOptions* options,
                               bool recommendedProfile);
    void StartScan(bool autoSelectRecommended);
    void StartApply(const std::vector<Item*>& plan);
    void OnApplyClicked(bool recommendedProfile);
    void OnUndoClicked();
    void OpenFolder(const std::wstring& path);
    void SaveLogAs();
    void WriteDiagnosticReportToFile();
    std::wstring MakeDataPath(const wchar_t* child) const;
    std::wstring MakeUserPath(const wchar_t* child) const;
    std::wstring TimestampForFile() const;
    void RegisterPage(Page& page, ItemKind kind);
    void CreatePageControls(Page& page);
    Page& PageFor(ItemKind kind);
    const Page& PageFor(ItemKind kind) const;
    void FillPageCategories(Page& page);
    void UpdatePageCount(Page& page);
    bool PageFilterMatch(const Page& page, const Item* item) const;
    void SelectPageRecommended(Page& page);
    void SelectPageAll(Page& page);
    void ClearPage(Page& page);
    void HandleListNotify(LPNMHDR header);
    void HandleListChanged(Page& page, LPNMLISTVIEW info);
    void HandleCommand(WORD id, WORD code, HWND control);
    void HandleNotify(LPNMHDR header);
    void StartWorkerScan();
    void StartWorkerApply(const std::vector<Item*>& plan);
    void FinishScan(ScanOutcome* outcome);
    void FinishApply(ApplySummary* summary);
    void UpdateSelectionUi();
    void SetProgressMarquee(bool on);
    void SetProgress(int current, int total);
    void ApplyTheme();
    void PaintHeader(HDC dc, const RECT& client);
    void PaintDetailPanel(HDC dc);
    void DrawRoundedRect(HDC dc, const RECT& rc, int radius, COLORREF fill, COLORREF border);
    void DrawHeaderBadge(HDC dc);
    void UpdateTabLabels();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DetailWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ScanThreadProc(LPVOID param);
    static DWORD WINAPI ApplyThreadProc(LPVOID param);

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND tab_ = nullptr;
    HWND status_ = nullptr;
    HWND progress_ = nullptr;
    HWND statusText_ = nullptr;
    HWND summaryText_ = nullptr;
    HWND detailPanel_ = nullptr;
    Item* detailItem_ = nullptr;
    HWND profileLabel_ = nullptr;
    HWND profileButtons_[4] = {};
    HWND languageCombo_ = nullptr;
    HWND dryRunCheck_ = nullptr;
    HWND applyButton_ = nullptr;
    HWND defaultButton_ = nullptr;
    HWND rescanButton_ = nullptr;
    HWND cancelButton_ = nullptr;
    HWND settingsButton_ = nullptr;
    HWND aboutButton_ = nullptr;
    HWND detailTitle_ = nullptr;
    HWND detailText_ = nullptr;
    HWND diagEdit_ = nullptr;
    HWND diagButtons_[5] = {};
    HWND logEdit_ = nullptr;
    HWND logButtons_[3] = {};

    HBRUSH brushBg_ = nullptr;
    HBRUSH brushWhite_ = nullptr;
    HBRUSH brushHeader_ = nullptr;
    HFONT fontUi_ = nullptr;
    HFONT fontUiBold_ = nullptr;
    HFONT fontTitle_ = nullptr;
    HFONT fontSubtitle_ = nullptr;
    HFONT fontSmall_ = nullptr;
    HFONT fontMono_ = nullptr;
    HICON iconLarge_ = nullptr;
    HICON iconSmall_ = nullptr;

    Page pages_[4];
    std::deque<Item> items_;
    Settings settings_;
    bool admin_ = false;
    bool readOnly_ = false;
    std::atomic<bool> busy_{false};
    bool applying_ = false;
    bool dryRun_ = false;
    bool scanAutoSelect_ = false;
    bool applyRestorePoint_ = true;
    bool applyBackup_ = true;
    bool applyDryRun_ = false;
    bool firstScan_ = true;
    Profile activeProfile_ = Profile::MicrosoftDefault;
    Language language_ = Language::Portuguese;
    bool closing_ = false;
    bool updatingList_ = false;
    int dpi_ = 96;
    int currentTab_ = 0;
    RECT elevateRect_{};
    bool elevateHover_ = false;
    TRACKMOUSEEVENT trackMouse_{};

    std::wstring cmdLine_;
    std::wstring noElevateArg_;
    std::wstring resumeSelectionPath_;
    std::wstring headlessReportPath_;
    bool elevatedArg_ = false;
    Profile startupProfile_ = Profile::MicrosoftDefault;

    CRITICAL_SECTION logCs_{};
    bool logCsInit_ = false;
    std::wstring logPath_;
    std::wstring logBuffer_;
    HANDLE logFile_ = INVALID_HANDLE_VALUE;
    std::wstring lastBackupPath_;
    std::wstring lastReportPath_;
    std::wstring lastScanSummary_;
    std::wstring diagnosticsText_;
    std::wstring featureScanError_;

    HANDLE cancelEvent_ = nullptr;
    std::atomic<bool> cancelRequested_{false};
    HANDLE scanThread_ = nullptr;
    HANDLE applyThread_ = nullptr;
    HWND activeWorkerWindow_ = nullptr;
};

// Mensagens internas
#define WM_APP_LOG          (WM_APP + 10)
#define WM_APP_STATUS       (WM_APP + 11)
#define WM_APP_SCAN_DONE    (WM_APP + 12)
#define WM_APP_APPLY_PROG   (WM_APP + 13)
#define WM_APP_APPLY_DONE   (WM_APP + 14)
#define WM_APP_DISM_PROG    (WM_APP + 15)
#define WM_APP_SCAN_FAIL    (WM_APP + 16)

// Funções utilitárias compartilhadas (core.cpp)
std::wstring FormatWinError(DWORD error);
std::wstring FormatHResult(HRESULT hr);
std::wstring FormatHResultWithHint(HRESULT hr, const wchar_t* context);
std::wstring Trim(const std::wstring& s);
std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep);
std::wstring Join(const std::vector<std::wstring>& parts, const std::wstring& sep);
bool FileExists(const std::wstring& path);
bool DirectoryExists(const std::wstring& path);
bool EnsureDirectory(const std::wstring& path);
std::wstring GetExePath();
std::wstring GetProgramDataDir();
std::wstring GetLocalAppDataDir();
std::wstring TimestampNow(bool forFile);
std::wstring StartTypeText(DWORD startType, bool delayed);
std::wstring ServiceStateText(DWORD state);
std::wstring FeatureStateText(int state);
std::wstring BoolText(bool value);
std::wstring FormatInt(long long value);
std::wstring Lower(const std::wstring& s);
bool ContainsNoCase(const std::wstring& haystack, const std::wstring& needle);
bool ReadTextFile(const std::wstring& path, std::wstring* out);
bool WriteTextFile(const std::wstring& path, const std::wstring& text, bool bom = true);
bool AppendTextFile(const std::wstring& path, const std::wstring& text);
std::wstring GetSystemSummary();
std::wstring GetWindowsBuildString();
std::wstring QueryRegistryString(HKEY root, const std::wstring& key, const std::wstring& value);
DWORD QueryRegistryDword(HKEY root, const std::wstring& key, const std::wstring& value, DWORD fallback);
bool ReadRegistryDword(HKEY root, const std::wstring& key, const std::wstring& value,
                       DWORD* data, bool* exists, DWORD* type);
bool WriteRegistryDword(HKEY root, const std::wstring& key, const std::wstring& value, DWORD data);
bool DeleteRegistryValue(HKEY root, const std::wstring& key, const std::wstring& value);
bool GetServiceStart(const std::wstring& name, DWORD* startType, bool* delayed,
                     DWORD* state, DWORD* win32Exit = nullptr, DWORD* lastError = nullptr);
bool SetServiceStart(const std::wstring& name, DWORD startType, bool delayed,
                     std::wstring* error, bool tryStart, bool* started);
std::wstring GetServiceDisplayName(const std::wstring& name);
bool EnableScheduledTask(const std::wstring& folder, const std::wstring& name,
                         bool enabled, std::wstring* error);
bool QueryScheduledTask(const std::wstring& folder, const std::wstring& name,
                        bool* enabled, bool* present, std::wstring* error);
std::wstring GuidToText(const GUID& guid);
std::wstring BytesToHex(const BYTE* data, size_t size);
void ResolveServiceTarget(Item& item, Profile profile);
