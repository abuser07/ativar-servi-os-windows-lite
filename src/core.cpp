#include "app.h"

#include <dismapi.h>
#include <taskschd.h>
#include <srrestoreptapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <winternl.h>

#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// GUIDs necessários para o Task Scheduler. O MinGW não fornece estas
// definições na biblioteca de importação; declaramos os valores localmente.
// ---------------------------------------------------------------------------

namespace {
const GUID kClsidTaskScheduler =
    {0x0f87369f, 0xa4e5, 0x4cfc, {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}};
const GUID kIidITaskService =
    {0x2faba4c7, 0x4da9, 0x4013, {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}};

constexpr DWORD kKeyWow64_64 = 0x0100;

std::wstring TrimTrailingNewlines(std::wstring s) {
    while (!s.empty() && (s.back() == L'\r' || s.back() == L'\n' ||
                          s.back() == L' ' || s.back() == L'\t')) {
        s.pop_back();
    }
    return s;
}

bool IsHResultWin32(HRESULT hr) {
    return HRESULT_FACILITY(hr) == FACILITY_WIN32;
}

std::wstring GuidString(const GUID& guid) {
    wchar_t buffer[64] = {};
    StringFromGUID2(guid, buffer, _countof(buffer));
    return buffer;
}
}  // namespace

// ---------------------------------------------------------------------------
// Utilidades de texto, arquivos e erros
// ---------------------------------------------------------------------------

std::wstring FormatWinError(DWORD error) {
    LPWSTR message = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD length = FormatMessageW(flags, nullptr, error, 0,
                                  reinterpret_cast<LPWSTR>(&message), 0, nullptr);
    std::wstring result;
    if (length && message) {
        result.assign(message, length);
        result = TrimTrailingNewlines(result);
    }
    if (message) LocalFree(message);
    if (result.empty()) {
        wchar_t fallback[64];
        swprintf(fallback, _countof(fallback), L"código %lu (0x%08lX)",
                 static_cast<unsigned long>(error), static_cast<unsigned long>(error));
        result = fallback;
    }
    return result;
}

std::wstring FormatHResult(HRESULT hr) {
    wchar_t code[24];
    swprintf(code, _countof(code), L"0x%08lX", static_cast<unsigned long>(hr));
    if (IsHResultWin32(hr)) {
        return std::wstring(code) + L" — " + FormatWinError(HRESULT_CODE(hr));
    }
    return code;
}

std::wstring FormatHResultWithHint(HRESULT hr, const wchar_t* context) {
    std::wstring text = FormatHResult(hr);
    if (context && *context) text = std::wstring(context) + L": " + text;
    return text;
}

std::wstring Trim(const std::wstring& s) {
    size_t first = 0;
    while (first < s.size() && iswspace(s[first])) ++first;
    size_t last = s.size();
    while (last > first && iswspace(s[last - 1])) --last;
    return s.substr(first, last - first);
}

std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (start <= s.size()) {
        size_t pos = s.find(sep, start);
        if (pos == std::wstring::npos) {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

std::wstring Join(const std::vector<std::wstring>& parts, const std::wstring& sep) {
    std::wstring out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += sep;
        out += parts[i];
    }
    return out;
}

bool FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirectoryExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool EnsureDirectory(const std::wstring& path) {
    if (path.empty()) return false;
    if (DirectoryExists(path)) return true;
    std::wstring current;
    size_t start = 0;
    if (path.size() >= 2 && path[1] == L':') {
        current = path.substr(0, 2);
        start = 2;
    }
    while (start <= path.size()) {
        size_t pos = path.find(L'\\', start);
        std::wstring piece = (pos == std::wstring::npos)
                                 ? path.substr(start)
                                 : path.substr(start, pos - start);
        if (!piece.empty()) {
            if (!current.empty() && current.back() != L'\\') current += L'\\';
            current += piece;
            if (!current.empty() && !DirectoryExists(current)) {
                if (!CreateDirectoryW(current.c_str(), nullptr) &&
                    GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
        if (pos == std::wstring::npos) break;
        start = pos + 1;
    }
    return DirectoryExists(path);
}

std::wstring GetExePath() {
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                          static_cast<DWORD>(buffer.size()));
        if (length == 0) return std::wstring();
        if (length < buffer.size()) return std::wstring(buffer.data(), length);
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring GetProgramDataDir() {
    wchar_t buffer[MAX_PATH * 2] = {};
    DWORD length = GetEnvironmentVariableW(L"ProgramData", buffer, _countof(buffer));
    if (length > 0 && length < _countof(buffer)) return buffer;
    return L"C:\\ProgramData";
}

std::wstring GetLocalAppDataDir() {
    wchar_t buffer[MAX_PATH * 2] = {};
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, _countof(buffer));
    if (length > 0 && length < _countof(buffer)) return buffer;
    return GetProgramDataDir();
}

std::wstring TimestampNow(bool forFile) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buffer[64];
    if (forFile) {
        swprintf(buffer, _countof(buffer), L"%04u%02u%02u-%02u%02u%02u",
                 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    } else {
        swprintf(buffer, _countof(buffer), L"%04u-%02u-%02u %02u:%02u:%02u",
                 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    }
    return buffer;
}

std::wstring StartTypeText(DWORD startType, bool delayed) {
    switch (startType) {
        case SERVICE_BOOT_START: return L"Inicialização do sistema";
        case SERVICE_SYSTEM_START: return L"Inicialização do SO";
        case SERVICE_AUTO_START:
            return delayed ? L"Automático (Atraso)" : L"Automático";
        case SERVICE_DEMAND_START: return L"Manual";
        case SERVICE_DISABLED: return L"Desabilitado";
        default: return L"Desconhecido";
    }
}

std::wstring ServiceStateText(DWORD state) {
    switch (state) {
        case SERVICE_STOPPED: return L"Parado";
        case SERVICE_START_PENDING: return L"Iniciando";
        case SERVICE_STOP_PENDING: return L"Parando";
        case SERVICE_RUNNING: return L"Em execução";
        case SERVICE_CONTINUE_PENDING: return L"Continuando";
        case SERVICE_PAUSE_PENDING: return L"Pausando";
        case SERVICE_PAUSED: return L"Pausado";
        default: return L"Desconhecido";
    }
}

std::wstring FeatureStateText(int state) {
    switch (state) {
        case DismStateNotPresent: return L"Não presente";
        case DismStateUninstallPending: return L"Remoção pendente";
        case DismStateStaged: return L"Preparado";
        case DismStateInstalled: return L"Instalado";
        case DismStateInstallPending: return L"Instalação pendente";
        case DismStateSuperseded: return L"Substituído";
        case DismStatePartiallyInstalled: return L"Parcialmente instalado";
        default: return L"Desconhecido";
    }
}

std::wstring BoolText(bool value) {
    return value ? L"Sim" : L"Não";
}

std::wstring FormatInt(long long value) {
    wchar_t buffer[64];
    swprintf(buffer, _countof(buffer), L"%lld", value);
    return buffer;
}

std::wstring Lower(const std::wstring& s) {
    std::wstring out = s;
    if (!out.empty()) CharLowerBuffW(&out[0], static_cast<DWORD>(out.size()));
    return out;
}

bool ContainsNoCase(const std::wstring& haystack, const std::wstring& needle) {
    if (needle.empty()) return true;
    return StrStrIW(haystack.c_str(), needle.c_str()) != nullptr;
}

bool ReadTextFile(const std::wstring& path, std::wstring* out) {
    out->clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart > 64 * 1024 * 1024) {
        CloseHandle(file);
        return false;
    }
    std::vector<char> data(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    BOOL ok = ReadFile(file, data.data(), static_cast<DWORD>(data.size()), &read, nullptr);
    CloseHandle(file);
    if (!ok) return false;

    if (read >= 2 && static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xFE) {
        *out = std::wstring(reinterpret_cast<const wchar_t*>(data.data() + 2),
                            (read - 2) / sizeof(wchar_t));
        return true;
    }
    size_t offset = 0;
    if (read >= 3 && static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        offset = 3;
    }
    int chars = MultiByteToWideChar(CP_UTF8, 0, data.data() + offset,
                                    static_cast<int>(read - offset), nullptr, 0);
    if (chars <= 0) return false;
    std::vector<wchar_t> wide(static_cast<size_t>(chars));
    MultiByteToWideChar(CP_UTF8, 0, data.data() + offset,
                        static_cast<int>(read - offset), wide.data(), chars);
    out->assign(wide.data(), static_cast<size_t>(chars));
    return true;
}

static std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return std::string();
    int bytes = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                    static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return std::string();
    std::string out(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        &out[0], bytes, nullptr, nullptr);
    return out;
}

bool WriteTextFile(const std::wstring& path, const std::wstring& text, bool bom) {
    std::string data = WideToUtf8(text);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = true;
    if (bom) {
        static const unsigned char kUtf8Bom[3] = {0xEF, 0xBB, 0xBF};
        ok = WriteFile(file, kUtf8Bom, 3, &written, nullptr) != FALSE;
    }
    if (ok && !data.empty()) {
        ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written,
                       nullptr) != FALSE;
    }
    CloseHandle(file);
    return ok;
}

bool AppendTextFile(const std::wstring& path, const std::wstring& text) {
    std::string data = WideToUtf8(text);
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = data.empty() ? TRUE
                           : WriteFile(file, data.data(), static_cast<DWORD>(data.size()),
                                       &written, nullptr);
    CloseHandle(file);
    return ok != FALSE;
}

// ---------------------------------------------------------------------------
// Registro
// ---------------------------------------------------------------------------

std::wstring QueryRegistryString(HKEY root, const std::wstring& key,
                                 const std::wstring& value) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ | kKeyWow64_64, &hKey) != ERROR_SUCCESS) {
        return std::wstring();
    }
    DWORD type = 0;
    DWORD bytes = 0;
    std::wstring result;
    if (RegQueryValueExW(hKey, value.c_str(), nullptr, &type, nullptr, &bytes) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ)) {
        std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 2, 0);
        if (RegQueryValueExW(hKey, value.c_str(), nullptr, &type,
                             reinterpret_cast<BYTE*>(buffer.data()), &bytes) == ERROR_SUCCESS) {
            result.assign(buffer.data());
            if (type == REG_EXPAND_SZ) {
                wchar_t expanded[4096] = {};
                DWORD n = ExpandEnvironmentStringsW(result.c_str(), expanded, _countof(expanded));
                if (n > 0 && n <= _countof(expanded)) result = expanded;
            }
        }
    }
    RegCloseKey(hKey);
    return result;
}

DWORD QueryRegistryDword(HKEY root, const std::wstring& key, const std::wstring& value,
                         DWORD fallback) {
    bool exists = false;
    DWORD type = 0;
    DWORD data = fallback;
    if (!ReadRegistryDword(root, key, value, &data, &exists, &type) || !exists ||
        type != REG_DWORD) {
        return fallback;
    }
    return data;
}

bool ReadRegistryDword(HKEY root, const std::wstring& key, const std::wstring& value,
                       DWORD* data, bool* exists, DWORD* type) {
    *data = 0;
    *exists = false;
    *type = 0;
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, key.c_str(), 0, KEY_READ | kKeyWow64_64, &hKey) != ERROR_SUCCESS) {
        return true;
    }
    DWORD valueType = 0;
    DWORD bytes = sizeof(DWORD);
    DWORD valueData = 0;
    LONG status = RegQueryValueExW(hKey, value.c_str(), nullptr, &valueType,
                                   reinterpret_cast<BYTE*>(&valueData), &bytes);
    if (status == ERROR_SUCCESS) {
        *exists = true;
        *type = valueType;
        *data = valueData;
    }
    RegCloseKey(hKey);
    return true;
}

bool WriteRegistryDword(HKEY root, const std::wstring& key, const std::wstring& value,
                        DWORD data) {
    HKEY hKey = nullptr;
    LONG status = RegCreateKeyExW(root, key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                  KEY_SET_VALUE | kKeyWow64_64, nullptr, &hKey, nullptr);
    if (status != ERROR_SUCCESS) return false;
    status = RegSetValueExW(hKey, value.c_str(), 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&data), sizeof(data));
    RegCloseKey(hKey);
    return status == ERROR_SUCCESS;
}

bool DeleteRegistryValue(HKEY root, const std::wstring& key, const std::wstring& value) {
    HKEY hKey = nullptr;
    LONG status = RegOpenKeyExW(root, key.c_str(), 0, KEY_SET_VALUE | kKeyWow64_64, &hKey);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) return true;
    if (status != ERROR_SUCCESS) return false;
    status = RegDeleteValueW(hKey, value.c_str());
    RegCloseKey(hKey);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

// ---------------------------------------------------------------------------
// Serviços
// ---------------------------------------------------------------------------

bool GetServiceStart(const std::wstring& name, DWORD* startType, bool* delayed,
                     DWORD* state, DWORD* win32Exit, DWORD* lastError) {
    if (startType) *startType = SERVICE_DISABLED;
    if (delayed) *delayed = false;
    if (state) *state = SERVICE_STOPPED;
    if (win32Exit) *win32Exit = 0;
    if (lastError) *lastError = 0;

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        if (lastError) *lastError = GetLastError();
        return false;
    }
    SC_HANDLE service = OpenServiceW(scm, name.c_str(),
                                     SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
    if (!service) {
        if (lastError) *lastError = GetLastError();
        CloseServiceHandle(scm);
        return false;
    }

    DWORD needed = 0;
    QueryServiceConfigW(service, nullptr, 0, &needed);
    std::vector<BYTE> buffer(needed ? needed : 8192);
    auto* config = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buffer.data());
    bool ok = QueryServiceConfigW(service, config, static_cast<DWORD>(buffer.size()),
                                  &needed) != FALSE;
    if (ok) {
        if (startType) *startType = config->dwStartType;
        SERVICE_DELAYED_AUTO_START_INFO delayedInfo = {};
        DWORD bytesNeeded = 0;
        if (QueryServiceConfig2W(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                                 reinterpret_cast<LPBYTE>(&delayedInfo), sizeof(delayedInfo),
                                 &bytesNeeded)) {
            if (delayed) *delayed = delayedInfo.fDelayedAutostart != FALSE;
        }
    } else if (lastError) {
        *lastError = GetLastError();
    }

    SERVICE_STATUS_PROCESS status = {};
    DWORD statusBytes = 0;
    bool statusOk = QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                         reinterpret_cast<LPBYTE>(&status), sizeof(status),
                                         &statusBytes) != FALSE;
    if (statusOk) {
        if (state) *state = status.dwCurrentState;
        if (win32Exit) *win32Exit = status.dwWin32ExitCode;
    } else if (lastError && ok) {
        *lastError = GetLastError();
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ok && statusOk;
}

bool SetServiceStart(const std::wstring& name, DWORD startType, bool delayed,
                     std::wstring* error, bool tryStart, bool* started) {
    if (error) error->clear();
    if (started) *started = false;
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        if (error) *error = FormatWinError(GetLastError());
        return false;
    }
    SC_HANDLE service = OpenServiceW(
        scm, name.c_str(),
        SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_START |
            SERVICE_STOP);
    if (!service) {
        DWORD err = GetLastError();
        if (error) {
            *error = FormatWinError(err);
            if (err == ERROR_ACCESS_DENIED) {
                *error += L". O serviço pode ser protegido pelo Windows (por exemplo, Defender).";
            }
        }
        CloseServiceHandle(scm);
        return false;
    }

    BOOL changed = ChangeServiceConfigW(service, SERVICE_NO_CHANGE, startType,
                                        SERVICE_NO_CHANGE, nullptr, nullptr, nullptr,
                                        nullptr, nullptr, nullptr, nullptr);
    if (!changed) {
        DWORD err = GetLastError();
        if (error) {
            *error = FormatWinError(err);
            if (err == ERROR_ACCESS_DENIED) {
                *error += L". O serviço pode ser protegido pelo Windows ou exigir a desativação temporária da proteção contra adulteração.";
            }
        }
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_DELAYED_AUTO_START_INFO delayedInfo = {};
    delayedInfo.fDelayedAutostart = (startType == SERVICE_AUTO_START && delayed) ? TRUE : FALSE;
    if (startType == SERVICE_AUTO_START || startType == SERVICE_DEMAND_START) {
        ChangeServiceConfig2W(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedInfo);
    }

    if (startType == SERVICE_DISABLED) {
        SERVICE_STATUS_PROCESS status = {};
        DWORD bytes = 0;
        if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                 reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytes) &&
            (status.dwCurrentState == SERVICE_RUNNING ||
             status.dwCurrentState == SERVICE_START_PENDING ||
             status.dwCurrentState == SERVICE_PAUSE_PENDING ||
             status.dwCurrentState == SERVICE_PAUSED ||
             status.dwCurrentState == SERVICE_CONTINUE_PENDING)) {
            SERVICE_STATUS stopStatus = {};
            if (ControlService(service, SERVICE_CONTROL_STOP, &stopStatus)) {
                for (int i = 0; i < 100 && stopStatus.dwCurrentState != SERVICE_STOPPED; ++i) {
                    Sleep(100);
                    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                              reinterpret_cast<LPBYTE>(&status), sizeof(status),
                                              &bytes)) {
                        break;
                    }
                    stopStatus.dwCurrentState = status.dwCurrentState;
                }
            } else {
                DWORD err = GetLastError();
                if (err != ERROR_SERVICE_NOT_ACTIVE && error) {
                    *error = L"Configuração alterada, mas o serviço não pôde ser parado: " +
                             FormatWinError(err);
                }
            }
        }
    }

    if (tryStart && startType == SERVICE_AUTO_START) {
        SERVICE_STATUS_PROCESS status = {};
        DWORD bytes = 0;
        if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                 reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytes)) {
            if (status.dwCurrentState != SERVICE_RUNNING &&
                status.dwCurrentState != SERVICE_START_PENDING) {
                if (StartServiceW(service, 0, nullptr)) {
                    if (started) *started = true;
                } else {
                    DWORD err = GetLastError();
                    if (err == ERROR_SERVICE_ALREADY_RUNNING) {
                        if (started) *started = true;
                    } else if (error) {
                        *error = L"Configuração alterada, mas o serviço não iniciou: " +
                                 FormatWinError(err);
                    }
                }
            } else {
                if (started) *started = true;
            }
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}

std::wstring GetServiceDisplayName(const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return std::wstring();
    SC_HANDLE service = OpenServiceW(scm, name.c_str(), SERVICE_QUERY_CONFIG);
    std::wstring result;
    if (service) {
        DWORD needed = 0;
        QueryServiceConfigW(service, nullptr, 0, &needed);
        std::vector<BYTE> buffer(needed ? needed : 8192);
        auto* config = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buffer.data());
        if (QueryServiceConfigW(service, config, static_cast<DWORD>(buffer.size()), &needed) &&
            config->lpDisplayName) {
            result = config->lpDisplayName;
        }
        CloseServiceHandle(service);
    }
    CloseServiceHandle(scm);
    return result;
}

// ---------------------------------------------------------------------------
// Tarefas agendadas (COM)
// ---------------------------------------------------------------------------

bool QueryScheduledTask(const std::wstring& folder, const std::wstring& name,
                        bool* enabled, bool* present, std::wstring* error) {
    if (enabled) *enabled = false;
    if (present) *present = false;
    if (error) error->clear();

    ITaskService* service = nullptr;
    HRESULT hr = CoCreateInstance(kClsidTaskScheduler, nullptr, CLSCTX_INPROC_SERVER,
                                  kIidITaskService, reinterpret_cast<void**>(&service));
    if (FAILED(hr) || !service) {
        if (error) *error = L"Task Scheduler indisponível: " + FormatHResult(hr);
        return false;
    }
    VARIANT empty;
    VariantInit(&empty);
    hr = service->Connect(empty, empty, empty, empty);
    if (FAILED(hr)) {
        if (error) *error = L"Falha ao conectar ao Task Scheduler: " + FormatHResult(hr);
        service->Release();
        return false;
    }

    BSTR folderBstr = SysAllocString(folder.c_str());
    ITaskFolder* taskFolder = nullptr;
    hr = service->GetFolder(folderBstr, &taskFolder);
    SysFreeString(folderBstr);
    if (FAILED(hr) || !taskFolder) {
        if (error) {
            *error = (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                         ? std::wstring()
                         : L"Pasta de tarefas não encontrada: " + FormatHResult(hr);
        }
        service->Release();
        return SUCCEEDED(hr) ? true : false;
    }

    BSTR nameBstr = SysAllocString(name.c_str());
    IRegisteredTask* task = nullptr;
    hr = taskFolder->GetTask(nameBstr, &task);
    SysFreeString(nameBstr);
    if (FAILED(hr) || !task) {
        if (present) *present = false;
        if (error && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            *error = L"Tarefa não encontrada: " + FormatHResult(hr);
        }
        taskFolder->Release();
        service->Release();
        return true;
    }

    VARIANT_BOOL value = VARIANT_FALSE;
    hr = task->get_Enabled(&value);
    if (SUCCEEDED(hr)) {
        if (enabled) *enabled = value != VARIANT_FALSE;
        if (present) *present = true;
    } else if (error) {
        *error = L"Falha ao consultar tarefa: " + FormatHResult(hr);
    }
    task->Release();
    taskFolder->Release();
    service->Release();
    return SUCCEEDED(hr);
}

bool EnableScheduledTask(const std::wstring& folder, const std::wstring& name,
                         bool enable, std::wstring* error) {
    if (error) error->clear();
    ITaskService* service = nullptr;
    HRESULT hr = CoCreateInstance(kClsidTaskScheduler, nullptr, CLSCTX_INPROC_SERVER,
                                  kIidITaskService, reinterpret_cast<void**>(&service));
    if (FAILED(hr) || !service) {
        if (error) *error = L"Task Scheduler indisponível: " + FormatHResult(hr);
        return false;
    }
    VARIANT empty;
    VariantInit(&empty);
    hr = service->Connect(empty, empty, empty, empty);
    if (FAILED(hr)) {
        if (error) *error = L"Falha ao conectar ao Task Scheduler: " + FormatHResult(hr);
        service->Release();
        return false;
    }
    BSTR folderBstr = SysAllocString(folder.c_str());
    ITaskFolder* taskFolder = nullptr;
    hr = service->GetFolder(folderBstr, &taskFolder);
    SysFreeString(folderBstr);
    if (FAILED(hr) || !taskFolder) {
        if (error) *error = L"Pasta de tarefas não encontrada: " + FormatHResult(hr);
        service->Release();
        return false;
    }
    BSTR nameBstr = SysAllocString(name.c_str());
    IRegisteredTask* task = nullptr;
    hr = taskFolder->GetTask(nameBstr, &task);
    SysFreeString(nameBstr);
    if (FAILED(hr) || !task) {
        if (error) *error = L"Tarefa não encontrada: " + FormatHResult(hr);
        taskFolder->Release();
        service->Release();
        return false;
    }
    hr = task->put_Enabled(enable ? VARIANT_TRUE : VARIANT_FALSE);
    if (FAILED(hr) && error) *error = L"Falha ao alterar a tarefa: " + FormatHResult(hr);
    task->Release();
    taskFolder->Release();
    service->Release();
    return SUCCEEDED(hr);
}

std::wstring GuidToText(const GUID& guid) {
    return GuidString(guid);
}

std::wstring BytesToHex(const BYTE* data, size_t size) {
    static const wchar_t kHex[] = L"0123456789ABCDEF";
    std::wstring out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        out.push_back(kHex[(data[i] >> 4) & 0xF]);
        out.push_back(kHex[data[i] & 0xF]);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Informações do sistema
// ---------------------------------------------------------------------------

std::wstring GetWindowsBuildString() {
    OSVERSIONINFOW os = {};
    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionExW(&os);

    std::wstring product = QueryRegistryString(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName");
    std::wstring display = QueryRegistryString(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
    std::wstring build = QueryRegistryString(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber");
    DWORD ubr = QueryRegistryDword(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR", 0);
    if (build.empty()) build = FormatInt(os.dwBuildNumber);
    if (product.find(L"Windows 10") != std::wstring::npos && os.dwBuildNumber >= 22000) {
        size_t pos = product.find(L"Windows 10");
        product.replace(pos, 10, L"Windows 11");
    }
    if (product.empty()) product = L"Windows";

    SYSTEM_INFO si = {};
    GetNativeSystemInfo(&si);
    const wchar_t* arch = L"x86";
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) arch = L"x64";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) arch = L"ARM64";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) arch = L"ARM";

    std::wstring result = product;
    if (!display.empty()) result += L" " + display;
    result += L" (build " + build;
    if (ubr) result += L"." + FormatInt(ubr);
    result += L") ";
    result += arch;
    return result;
}

std::wstring GetSystemSummary() {
    std::wstring text;
    text += L"Sistema: " + GetWindowsBuildString() + L"\r\n";

    wchar_t computer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD computerSize = _countof(computer);
    GetComputerNameW(computer, &computerSize);
    wchar_t user[256] = {};
    DWORD userSize = _countof(user);
    GetUserNameW(user, &userSize);
    text += L"Computador: " + std::wstring(computer) + L"\r\n";
    text += L"Usuário: " + std::wstring(user) + L"\r\n";

    MEMORYSTATUSEX memory = {};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        unsigned long long totalGb = memory.ullTotalPhys / (1024ull * 1024ull * 1024ull);
        unsigned long long freeGb = memory.ullAvailPhys / (1024ull * 1024ull * 1024ull);
        text += L"Memória: " + FormatInt(static_cast<long long>(freeGb)) + L" GB livres de " +
                FormatInt(static_cast<long long>(totalGb)) + L" GB\r\n";
    }
    ULARGE_INTEGER freeBytes = {}, totalBytes = {};
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytes, &totalBytes, nullptr)) {
        unsigned long long freeGb = freeBytes.QuadPart / (1024ull * 1024ull * 1024ull);
        unsigned long long totalGb = totalBytes.QuadPart / (1024ull * 1024ull * 1024ull);
        text += L"Disco C: " + FormatInt(static_cast<long long>(freeGb)) + L" GB livres de " +
                FormatInt(static_cast<long long>(totalGb)) + L" GB\r\n";
    }

    DWORD secureBoot = QueryRegistryDword(
        HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State",
        L"UEFISecureBootEnabled", 0xFFFFFFFF);
    text += L"Secure Boot: ";
    if (secureBoot == 0xFFFFFFFF) text += L"não informado";
    else text += secureBoot ? L"habilitado" : L"desabilitado";
    text += L"\r\n";

    DWORD disableSr = QueryRegistryDword(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore",
        L"DisableSR", 0);
    text += L"Restauração do Sistema: " + std::wstring(disableSr ? L"desativada por política" : L"permitida") + L"\r\n";

    wchar_t psPath[MAX_PATH] = {};
    bool hasPowerShell = SearchPathW(nullptr, L"powershell.exe", nullptr, _countof(psPath),
                                     psPath, nullptr) > 0;
    text += L"Windows PowerShell: " + std::wstring(hasPowerShell ? L"disponível" : L"não encontrado") + L"\r\n";

    HMODULE dism = LoadLibraryW(L"dismapi.dll");
    text += L"API DISM: " + std::wstring(dism ? L"disponível" : L"não encontrada") + L"\r\n";
    if (dism) FreeLibrary(dism);

    wchar_t windir[MAX_PATH] = {};
    GetWindowsDirectoryW(windir, _countof(windir));
    text += L"Pasta do Windows: " + std::wstring(windir) + L"\r\n";
    return text;
}

// ---------------------------------------------------------------------------
// DISM
// ---------------------------------------------------------------------------

namespace {

typedef HRESULT(WINAPI* DismInitializeFn)(DismLogLevel, PCWSTR, PCWSTR);
typedef HRESULT(WINAPI* DismShutdownFn)();
typedef HRESULT(WINAPI* DismOpenSessionFn)(PCWSTR, PCWSTR, PCWSTR, DismSession*);
typedef HRESULT(WINAPI* DismCloseSessionFn)(DismSession);
typedef HRESULT(WINAPI* DismGetFeaturesFn)(DismSession, PCWSTR, DismPackageIdentifier,
                                           DismFeature**, UINT*);
typedef HRESULT(WINAPI* DismGetFeatureInfoFn)(DismSession, PCWSTR, PCWSTR,
                                              DismPackageIdentifier, DismFeatureInfo**);
typedef HRESULT(WINAPI* DismEnableFeatureFn)(DismSession, PCWSTR, PCWSTR,
                                             DismPackageIdentifier, WINBOOL, PCWSTR*, UINT,
                                             WINBOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
typedef HRESULT(WINAPI* DismDeleteFn)(VOID*);
typedef HRESULT(WINAPI* DismGetLastErrorMessageFn)(DismString**);

struct DismApi {
    HMODULE module = nullptr;
    DismInitializeFn Initialize = nullptr;
    DismShutdownFn Shutdown = nullptr;
    DismOpenSessionFn OpenSession = nullptr;
    DismCloseSessionFn CloseSession = nullptr;
    DismGetFeaturesFn GetFeatures = nullptr;
    DismGetFeatureInfoFn GetFeatureInfo = nullptr;
    DismEnableFeatureFn EnableFeature = nullptr;
    DismDeleteFn Delete = nullptr;
    DismGetLastErrorMessageFn GetLastErrorMessage = nullptr;

    bool Load() {
        module = LoadLibraryW(L"dismapi.dll");
        if (!module) return false;
        Initialize = reinterpret_cast<DismInitializeFn>(GetProcAddress(module, "DismInitialize"));
        Shutdown = reinterpret_cast<DismShutdownFn>(GetProcAddress(module, "DismShutdown"));
        OpenSession = reinterpret_cast<DismOpenSessionFn>(GetProcAddress(module, "DismOpenSession"));
        CloseSession = reinterpret_cast<DismCloseSessionFn>(GetProcAddress(module, "DismCloseSession"));
        GetFeatures = reinterpret_cast<DismGetFeaturesFn>(GetProcAddress(module, "DismGetFeatures"));
        GetFeatureInfo = reinterpret_cast<DismGetFeatureInfoFn>(GetProcAddress(module, "DismGetFeatureInfo"));
        EnableFeature = reinterpret_cast<DismEnableFeatureFn>(GetProcAddress(module, "DismEnableFeature"));
        Delete = reinterpret_cast<DismDeleteFn>(GetProcAddress(module, "DismDelete"));
        GetLastErrorMessage = reinterpret_cast<DismGetLastErrorMessageFn>(
            GetProcAddress(module, "DismGetLastErrorMessage"));
        if (!Initialize || !Shutdown || !OpenSession || !CloseSession || !GetFeatures ||
            !EnableFeature || !Delete) {
            FreeLibrary(module);
            module = nullptr;
            return false;
        }
        return true;
    }

    ~DismApi() {
        if (module) FreeLibrary(module);
    }
};

std::wstring DismLastError(DismApi& api, HRESULT hr) {
    std::wstring text;
    if (api.GetLastErrorMessage) {
        DismString* message = nullptr;
        if (SUCCEEDED(api.GetLastErrorMessage(&message)) && message && message->Value) {
            text = message->Value;
            if (api.Delete) api.Delete(message);
        }
    }
    if (text.empty()) text = FormatHResult(hr);
    else text = FormatHResult(hr) + L" — " + text;
    return text;
}

std::wstring FeatureHint(HRESULT hr) {
    switch (static_cast<unsigned long>(hr)) {
        case 0x800F0906UL:
        case 0x800F081FUL:
        case 0x800F0954UL:
            return L"Arquivos de origem não encontrados. Verifique a Internet ou informe uma pasta/mídia de origem nas Configurações.";
        case 0x800F0902UL:
            return L"Outra operação do DISM está em andamento. Reinicie o computador e tente novamente.";
        case 0x800F0922UL:
            return L"Falha de instalação. Pode faltar espaço em disco ou ser necessária uma reinicialização.";
        case 0x800702E4UL:
            return L"É necessário executar o aplicativo como administrador.";
        case 0x800F080CUL:
            return L"O recurso não existe nesta versão do Windows.";
        default:
            return std::wstring();
    }
}

struct DismProgressContext {
    HWND window = nullptr;
    std::atomic<int> lastPercent{-1};
};

void CALLBACK DismProgressCallback(UINT current, UINT total, PVOID userData) {
    auto* context = static_cast<DismProgressContext*>(userData);
    if (!context) return;
    int percent = total ? static_cast<int>((static_cast<unsigned long long>(current) * 100) / total)
                        : 0;
    if (percent == context->lastPercent.load()) return;
    context->lastPercent.store(percent);
    if (context->window) {
        PostMessageW(context->window, WM_APP_DISM_PROG, static_cast<WPARAM>(percent), 0);
    }
}

std::wstring DismLogPath(const App& app) {
    (void)app;
    std::wstring dir = GetProgramDataDir() + L"\\WindowsLiteRestore\\dism-logs";
    if (!EnsureDirectory(dir)) {
        dir = GetLocalAppDataDir() + L"\\WindowsLiteRestore\\dism-logs";
        EnsureDirectory(dir);
    }
    return dir + L"\\dism-" + TimestampNow(true) + L".log";
}

void AddPseudoItem(std::deque<Item>& items, ItemKind kind, const std::wstring& title,
                   const std::wstring& detail) {
    Item item;
    item.kind = kind;
    item.id = L"__info__";
    item.name = title;
    item.category = L"Diagnóstico";
    item.description = detail;
    item.extra = L"";
    item.current = L"Indisponível";
    item.target = L"—";
    item.present = false;
    item.supported = false;
    item.needsChange = false;
    item.recommended = false;
    item.selected = false;
    items.push_back(std::move(item));
}

std::wstring PolicyValueText(DWORD value) {
    if (value == 0) return L"0 (desativado)";
    if (value == 1) return L"1 (ativado)";
    return FormatInt(value);
}

}  // namespace

// ---------------------------------------------------------------------------
// Resolução do destino de um serviço conforme o perfil ativo
// ---------------------------------------------------------------------------

void ResolveServiceTarget(Item& item, Profile profile) {
    if (!item.present) {
        item.target = L"—";
        item.targetStart = item.startType;
        item.targetDelayed = item.delayed;
        item.needsChange = false;
        item.recommended = false;
        return;
    }

    if (profile == Profile::MicrosoftDefault) {
        const ServiceDef* def = FindServiceDef(item.id);
        DWORD start = def ? def->targetStart : SERVICE_DEMAND_START;
        bool delayed = def ? def->targetDelayed : false;
        item.targetStart = start;
        item.targetDelayed = delayed;
        item.target = StartTypeText(start, delayed);
        item.needsChange = (item.startType != start) ||
                           (start == SERVICE_AUTO_START && item.delayed != delayed);
        item.recommended = def ? def->recommended : false;
        return;
    }

    // Perfil de otimização: apenas serviços listados no perfil são alterados.
    DWORD start = 0;
    bool delayed = false;
    if (GetProfileServiceTarget(item.id, profile, &start, &delayed) &&
        !(item.essential && start == SERVICE_DISABLED)) {
        item.targetStart = start;
        item.targetDelayed = delayed;
        item.target = StartTypeText(start, delayed);
        item.needsChange = (item.startType != start) ||
                           (start == SERVICE_AUTO_START && item.delayed != delayed);
        item.recommended = true;
    } else {
        item.targetStart = item.startType;
        item.targetDelayed = item.delayed;
        item.target = L"Sem alteração";
        item.needsChange = false;
        item.recommended = false;
    }
}

// ---------------------------------------------------------------------------
// Análise do sistema
// ---------------------------------------------------------------------------

bool App::ScanAll(std::wstring* error) {
    if (error) error->clear();
    featureScanError_.clear();
    items_.clear();

    Log(L"=== Início da análise ===");
    Log(L"Sistema: " + GetWindowsBuildString());
    Log(L"Modo: " + std::wstring(admin_ ? L"administrador" : L"somente leitura"));

    // Serviços
    int serviceCount = 0;
    int serviceMissing = 0;
    for (size_t i = 0; i < g_serviceCatalogCount; ++i) {
        const ServiceDef& def = g_serviceCatalog[i];
        Item item;
        item.kind = ItemKind::Service;
        item.id = def.name;
        item.name = def.display;
        item.category = def.category;
        item.description = def.description;
        item.extra = def.name;
        item.optional = def.optional;
        item.critical = def.critical;
        item.essential = IsEssentialService(def.name);

        DWORD startType = SERVICE_DISABLED;
        bool delayed = false;
        DWORD state = SERVICE_STOPPED;
        DWORD lastError = 0;
        bool ok = GetServiceStart(def.name, &startType, &delayed, &state, nullptr, &lastError);
        if (!ok) {
            if (lastError == ERROR_SERVICE_DOES_NOT_EXIST) {
                item.present = false;
                item.supported = false;
                item.current = L"Não instalado";
                item.description += L" O serviço não existe nesta instalação; pode ter sido removido pela imagem Lite. Neste caso, o componente de origem do Windows pode ser necessário.";
                serviceMissing++;
            } else {
                item.present = false;
                item.supported = false;
                item.current = L"Erro: " + FormatWinError(lastError);
            }
        } else {
            item.present = true;
            item.startType = startType;
            item.delayed = delayed;
            item.serviceState = state;
            item.current = StartTypeText(startType, delayed) + L" • " + ServiceStateText(state);
            serviceCount++;
        }
        ResolveServiceTarget(item, activeProfile_);
        items_.push_back(std::move(item));
    }
    Log(L"Serviços analisados: " + FormatInt(serviceCount) + L" presentes, " +
        FormatInt(serviceMissing) + L" ausentes.");

    // Políticas de registro
    int policyCount = 0;
    int policyUnsupported = 0;
    for (size_t i = 0; i < g_policyCatalogCount; ++i) {
        const PolicyDef& def = g_policyCatalog[i];
        Item item;
        item.kind = ItemKind::Policy;
        item.id = def.value;
        item.name = def.display;
        item.category = def.category;
        item.description = def.description;
        item.hive = def.root;
        item.key = def.subkey;
        item.valueName = def.value;
        item.desiredExists = def.desiredExists;
        item.desiredData = def.desired;
        item.recommended = def.recommended;
        item.optional = !def.recommended;
        item.extra = std::wstring(def.root == HKEY_LOCAL_MACHINE ? L"HKLM\\" : L"HKU\\") +
                     def.subkey + L"\\" + def.value;

        bool exists = false;
        DWORD type = 0;
        DWORD data = 0;
        ReadRegistryDword(def.root, def.subkey, def.value, &data, &exists, &type);
        item.currentExists = exists;
        item.currentType = type;
        item.currentData = data;
        if (exists) {
            item.current = (type == REG_DWORD) ? PolicyValueText(data)
                                               : L"Tipo de valor " + FormatInt(type);
        } else {
            item.current = L"(ausente)";
        }
        item.target = def.desiredExists ? PolicyValueText(def.desired) : L"(remover política)";
        if (exists && type != REG_DWORD && def.desiredExists) {
            item.supported = false;
            item.needsChange = false;
            item.recommended = false;
            item.description += L" O valor existente não é DWORD; por segurança, o aplicativo não o altera automaticamente.";
            policyUnsupported++;
        } else {
            item.needsChange = (exists != def.desiredExists) ||
                               (exists && def.desiredExists && data != def.desired);
            policyCount++;
        }
        items_.push_back(std::move(item));
    }
    Log(L"Políticas analisadas: " + FormatInt(policyCount) +
        (policyUnsupported ? L" (não suportadas: " + FormatInt(policyUnsupported) + L")" : L"") + L".");

    // Tarefas agendadas
    int taskCount = 0;
    int taskMissing = 0;
    int taskErrors = 0;
    std::wstring firstTaskError;
    for (size_t i = 0; i < g_taskCatalogCount; ++i) {
        const TaskDef& def = g_taskCatalog[i];
        Item item;
        item.kind = ItemKind::Task;
        item.id = std::wstring(def.folder) + L"\\" + def.name;
        item.name = def.display;
        item.category = def.category;
        item.description = def.description;
        item.extra = std::wstring(def.folder) + L"\\" + def.name;
        item.taskFolder = def.folder;
        item.taskName = def.name;
        item.recommended = def.recommended;
        item.optional = !def.recommended;
        item.target = L"Habilitada";

        bool enabled = false;
        bool present = false;
        std::wstring taskError;
        bool ok = QueryScheduledTask(def.folder, def.name, &enabled, &present, &taskError);
        if (!ok) {
            taskErrors++;
            if (firstTaskError.empty()) firstTaskError = taskError;
            item.present = false;
            item.supported = false;
            item.current = L"Erro ao consultar";
            item.target = L"—";
            item.needsChange = false;
            item.recommended = false;
        } else if (!present) {
            taskMissing++;
            item.present = false;
            item.supported = false;
            item.current = L"Não encontrada";
            item.target = L"—";
            item.needsChange = false;
            item.recommended = false;
        } else {
            item.present = true;
            item.taskEnabled = enabled;
            item.current = enabled ? L"Habilitada" : L"Desabilitada";
            item.needsChange = !enabled;
            taskCount++;
        }
        items_.push_back(std::move(item));
    }
    if (!firstTaskError.empty()) {
        Log(L"Aviso ao consultar tarefas: " + firstTaskError);
    }
    if (taskErrors > 0 && taskCount == 0) {
        AddPseudoItem(items_, ItemKind::Task, L"Falha ao consultar tarefas agendadas",
                      L"Não foi possível acessar o Task Scheduler. " + firstTaskError);
    }
    Log(L"Tarefas analisadas: " + FormatInt(taskCount) + L" presentes, " +
        FormatInt(taskMissing) + L" ausentes.");

    // Recursos opcionais (DISM)
    if (!admin_) {
        featureScanError_ = L"Os recursos do Windows só podem ser analisados com privilégios de administrador.";
        AddPseudoItem(items_, ItemKind::Feature, L"Recursos exigem administrador",
                      L"Reinicie o aplicativo como administrador para listar e instalar recursos opcionais do Windows.");
        Log(L"Recursos: análise ignorada (sem privilégios de administrador).");
    } else {
        DismApi api;
        if (!api.Load()) {
            featureScanError_ = L"A biblioteca dismapi.dll não está disponível neste sistema.";
            AddPseudoItem(items_, ItemKind::Feature, L"API DISM indisponível", featureScanError_);
            Log(featureScanError_);
        } else {
            std::wstring dismLog = DismLogPath(*this);
            HRESULT hr = api.Initialize(DismLogErrorsWarningsInfo,
                                        dismLog.empty() ? nullptr : dismLog.c_str(), nullptr);
            if (FAILED(hr)) {
                featureScanError_ = L"Falha ao iniciar o DISM: " + DismLastError(api, hr);
                if (FeatureHint(hr).size()) featureScanError_ += L" " + FeatureHint(hr);
                AddPseudoItem(items_, ItemKind::Feature, L"Falha ao iniciar o DISM", featureScanError_);
                Log(featureScanError_);
            } else {
                DismSession session = 0;
                hr = api.OpenSession(DISM_ONLINE_IMAGE, nullptr, nullptr, &session);
                if (FAILED(hr)) {
                    featureScanError_ = L"Falha ao abrir a imagem online do Windows: " +
                                        DismLastError(api, hr);
                    if (FeatureHint(hr).size()) featureScanError_ += L" " + FeatureHint(hr);
                    AddPseudoItem(items_, ItemKind::Feature, L"Falha ao abrir a imagem", featureScanError_);
                    Log(featureScanError_);
                    api.Shutdown();
                } else {
                    DismFeature* features = nullptr;
                    UINT count = 0;
                    hr = api.GetFeatures(session, nullptr, DismPackageNone, &features, &count);
                    if (FAILED(hr)) {
                        featureScanError_ = L"Falha ao enumerar recursos: " + DismLastError(api, hr);
                        AddPseudoItem(items_, ItemKind::Feature, L"Falha ao enumerar recursos", featureScanError_);
                        Log(featureScanError_);
                    } else {
                        int featureCount = 0;
                        for (UINT i = 0; i < count; ++i) {
                            const DismFeature& feature = features[i];
                            if (!feature.FeatureName) continue;
                            std::wstring name = feature.FeatureName;
                            const FeatureDef* def = FindFeatureDef(name);
                            Item item;
                            item.kind = ItemKind::Feature;
                            item.id = name;
                            item.extra = name;
                            item.featureState = feature.State;
                            item.present = feature.State != DismStateNotPresent;
                            item.current = FeatureStateText(feature.State);
                            item.supported = true;
                            if (def) {
                                item.name = def->display;
                                item.category = def->category;
                                item.description = def->description;
                                item.recommended = def->microsoftDefaultEnabled;
                                item.optional = def->optionalInstall;
                                item.target = def->microsoftDefaultEnabled ? L"Habilitado"
                                                                          : L"Desabilitado (opcional)";
                            } else {
                                item.name = name;
                                item.category = L"Outros recursos";
                                item.description = L"Recurso opcional do Windows. Marcar instala o recurso; consulte a documentação da Microsoft antes de habilitar componentes de servidor, virtualização ou rede.";
                                item.recommended = false;
                                item.optional = true;
                                item.target = L"Sem referência";
                            }
                            item.needsChange = (feature.State != DismStateInstalled &&
                                                feature.State != DismStateInstallPending);
                            if (feature.State == DismStateInstallPending) {
                                item.current = L"Instalação pendente";
                            }
                            items_.push_back(std::move(item));
                            featureCount++;
                        }
                        Log(L"Recursos analisados: " + FormatInt(featureCount) + L".");
                    }
                    if (features) api.Delete(features);
                    api.CloseSession(session);
                    api.Shutdown();
                }
            }
        }
    }

    // Resumo
    int pendingFeatures = 0, pendingServices = 0, pendingPolicies = 0, pendingTasks = 0;
    for (const Item& item : items_) {
        if (!item.supported) continue;
        if (item.recommended && item.needsChange) {
            switch (item.kind) {
                case ItemKind::Feature: pendingFeatures++; break;
                case ItemKind::Service: pendingServices++; break;
                case ItemKind::Policy: pendingPolicies++; break;
                case ItemKind::Task: pendingTasks++; break;
            }
        }
    }
    wchar_t summary[512];
    swprintf(summary, _countof(summary),
             L"Pendências recomendadas: %d recursos, %d serviços, %d políticas, %d tarefas.",
             pendingFeatures, pendingServices, pendingPolicies, pendingTasks);
    lastScanSummary_ = summary;
    Log(L"Análise concluída. " + lastScanSummary_);
    return true;
}

// ---------------------------------------------------------------------------
// Log e configurações
// ---------------------------------------------------------------------------

std::wstring App::MakeDataPath(const wchar_t* child) const {
    std::wstring root = GetProgramDataDir() + L"\\WindowsLiteRestore";
    std::wstring path = root;
    if (child && *child) path += L"\\" + std::wstring(child);
    return path;
}

std::wstring App::MakeUserPath(const wchar_t* child) const {
    std::wstring root = GetLocalAppDataDir() + L"\\WindowsLiteRestore";
    std::wstring path = root;
    if (child && *child) path += L"\\" + std::wstring(child);
    return path;
}

std::wstring App::TimestampForFile() const {
    return TimestampNow(true);
}

void App::InitLogging() {
    if (!logCsInit_) {
        InitializeCriticalSection(&logCs_);
        logCsInit_ = true;
    }
    std::wstring dir = admin_ ? MakeDataPath(L"logs") : MakeUserPath(L"logs");
    if (!EnsureDirectory(dir)) {
        dir = MakeUserPath(L"logs");
        EnsureDirectory(dir);
    }
    logPath_ = dir + L"\\restore-" + TimestampNow(true) + L".log";
    logFile_ = CreateFileW(logPath_.c_str(), FILE_APPEND_DATA | GENERIC_WRITE,
                           FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (logFile_ != INVALID_HANDLE_VALUE) {
        static const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
        DWORD written = 0;
        WriteFile(logFile_, bom, 3, &written, nullptr);
    }
}

void App::CloseLogging() {
    if (logFile_ != INVALID_HANDLE_VALUE) {
        CloseHandle(logFile_);
        logFile_ = INVALID_HANDLE_VALUE;
    }
    if (logCsInit_) {
        DeleteCriticalSection(&logCs_);
        logCsInit_ = false;
    }
}

void App::Log(const std::wstring& text) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t prefix[32];
    swprintf(prefix, _countof(prefix), L"[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
    std::wstring line = std::wstring(prefix) + text;

    if (logCsInit_) EnterCriticalSection(&logCs_);
    logBuffer_ += line + L"\r\n";
    if (logBuffer_.size() > 1024 * 1024) {
        logBuffer_.erase(0, logBuffer_.size() - 512 * 1024);
    }
    if (logFile_ != INVALID_HANDLE_VALUE) {
        std::wstring fileLine = line + L"\r\n";
        std::string utf8 = WideToUtf8(fileLine);
        DWORD written = 0;
        WriteFile(logFile_, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    if (logCsInit_) LeaveCriticalSection(&logCs_);

    if (hwnd_ && !closing_) {
        PostMessageW(hwnd_, WM_APP_LOG, 0, reinterpret_cast<LPARAM>(new std::wstring(line)));
    }
}

void App::PostStatus(const std::wstring& text) {
    if (hwnd_ && !closing_) {
        PostMessageW(hwnd_, WM_APP_STATUS, 0, reinterpret_cast<LPARAM>(new std::wstring(text)));
    }
}

void App::LoadSettings() {
    std::wstring dir = MakeUserPath(L"");
    EnsureDirectory(dir);
    std::wstring ini = dir + L"\\settings.ini";
    settings_.useWindowsUpdate =
        GetPrivateProfileIntW(L"Geral", L"UseWindowsUpdate", 1, ini.c_str()) != 0;
    settings_.createRestorePoint =
        GetPrivateProfileIntW(L"Geral", L"CreateRestorePoint", 1, ini.c_str()) != 0;
    settings_.createBackup =
        GetPrivateProfileIntW(L"Geral", L"CreateBackup", 1, ini.c_str()) != 0;
    settings_.autoScan =
        GetPrivateProfileIntW(L"Geral", L"AutoScan", 1, ini.c_str()) != 0;
    wchar_t source[4096] = {};
    GetPrivateProfileStringW(L"Geral", L"SourcePath", L"", source, _countof(source), ini.c_str());
    settings_.sourcePath = source;
    wchar_t lang[64] = {};
    GetPrivateProfileStringW(L"Geral", L"Language", L"", lang, _countof(lang), ini.c_str());
    if (lang[0]) {
        language_ = LanguageFromName(lang);
    }
    SetUiLanguage(language_);
}

void App::SaveSettings() {
    std::wstring dir = MakeUserPath(L"");
    EnsureDirectory(dir);
    std::wstring ini = dir + L"\\settings.ini";
    WritePrivateProfileStringW(L"Geral", L"UseWindowsUpdate",
                               settings_.useWindowsUpdate ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Geral", L"CreateRestorePoint",
                               settings_.createRestorePoint ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Geral", L"CreateBackup",
                               settings_.createBackup ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Geral", L"AutoScan",
                               settings_.autoScan ? L"1" : L"0", ini.c_str());
    WritePrivateProfileStringW(L"Geral", L"SourcePath", settings_.sourcePath.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Geral", L"Language", LanguageName(language_), ini.c_str());
}

// ---------------------------------------------------------------------------
// Ponto de restauração
// ---------------------------------------------------------------------------

bool App::CreateRestorePoint(std::wstring* error) {
    if (error) error->clear();
    HMODULE module = LoadLibraryW(L"srclient.dll");
    if (!module) {
        if (error) *error = L"srclient.dll não está disponível: " + FormatWinError(GetLastError());
        return false;
    }
    typedef BOOL(WINAPI* SRSetRestorePointWFn)(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
    auto setRestorePoint = reinterpret_cast<SRSetRestorePointWFn>(
        GetProcAddress(module, "SRSetRestorePointW"));
    if (!setRestorePoint) {
        FreeLibrary(module);
        if (error) *error = L"A função SRSetRestorePointW não foi encontrada.";
        return false;
    }

    RESTOREPOINTINFOW info = {};
    STATEMGRSTATUS status = {};
    info.dwEventType = BEGIN_SYSTEM_CHANGE;
    info.dwRestorePtType = MODIFY_SETTINGS;
    info.llSequenceNumber = 0;
    wcscpy_s(info.szDescription, MAX_DESC_W,
             L"Windows Lite Restore - antes de restaurar padroes");
    BOOL ok = setRestorePoint(&info, &status);
    DWORD lastError = GetLastError();
    FreeLibrary(module);
    if (!ok) {
        if (error) {
            *error = L"Falha ao criar ponto de restauração: " + FormatWinError(lastError);
            if (status.nStatus != 0) {
                *error += L" (status " + FormatInt(status.nStatus) + L")";
            }
            *error += L". Verifique se a Proteção do Sistema está ativada.";
        }
        return false;
    }
    Log(L"Ponto de restauração criado. Sequência: " + FormatInt(
        static_cast<long long>(status.llSequenceNumber)));
    return true;
}

// ---------------------------------------------------------------------------
// Backup e desfazer
// ---------------------------------------------------------------------------

static std::wstring RegRootName(HKEY root) {
    if (root == HKEY_LOCAL_MACHINE) return L"HKLM";
    if (root == HKEY_CURRENT_USER) return L"HKCU";
    if (root == HKEY_CLASSES_ROOT) return L"HKCR";
    if (root == HKEY_USERS) return L"HKU";
    return L"HKLM";
}

static HKEY RegRootFromName(const std::wstring& name) {
    if (_wcsicmp(name.c_str(), L"HKLM") == 0) return HKEY_LOCAL_MACHINE;
    if (_wcsicmp(name.c_str(), L"HKCU") == 0) return HKEY_CURRENT_USER;
    if (_wcsicmp(name.c_str(), L"HKCR") == 0) return HKEY_CLASSES_ROOT;
    if (_wcsicmp(name.c_str(), L"HKU") == 0) return HKEY_USERS;
    return HKEY_LOCAL_MACHINE;
}

std::wstring App::CreateBackup(const std::vector<Item*>& plan, std::wstring* error) {
    if (error) error->clear();
    std::wstring base = MakeDataPath(L"Backups");
    if (!EnsureDirectory(base)) {
        if (error) *error = L"Não foi possível criar a pasta de backups: " + base;
        return std::wstring();
    }
    std::wstring dir = base + L"\\" + TimestampNow(true);
    int suffix = 1;
    while (DirectoryExists(dir)) {
        dir = base + L"\\" + TimestampNow(true) + L"-" + FormatInt(suffix++);
    }
    if (!EnsureDirectory(dir)) {
        if (error) *error = L"Não foi possível criar a pasta de backup: " + dir;
        return std::wstring();
    }

    std::wstring manifest = dir + L"\\manifest.tsv";
    std::wstring text = L"# Windows Lite Restore - manifesto de backup\r\n";
    text += L"# Criado em: " + TimestampNow(false) + L"\r\n";
    text += L"# Sistema: " + GetWindowsBuildString() + L"\r\n";
    text += L"# Serviços e políticas podem ser revertidos pelo menu Ações > Desfazer última execução.\r\n";
    text += L"# Recursos opcionais não são revertidos automaticamente.\r\n";
    text += L"# Formato: SERVICE\\tnome\\tstartAntigo\\tdelayedAntigo\r\n";
    text += L"#          POLICY\\traiz\\tchave\\tvalor\\texisteAntigo\\ttipoAntigo\\tdadoAntigo\r\n";
    text += L"#          TASK\\tpasta\\tnome\\thabilitadaAntiga\r\n";
    text += L"#          FEATURE\\tnome\\testadoAntigo\r\n";

    for (Item* item : plan) {
        if (!item || !item->selected || !item->needsChange || !item->supported) continue;
        switch (item->kind) {
            case ItemKind::Service:
                text += L"SERVICE\t" + item->id + L"\t" + FormatInt(item->startType) + L"\t" +
                        (item->delayed ? L"1" : L"0") + L"\r\n";
                break;
            case ItemKind::Policy:
                text += L"POLICY\t" + RegRootName(item->hive) + L"\t" + item->key + L"\t" +
                        item->valueName + L"\t" + (item->currentExists ? L"1" : L"0") + L"\t" +
                        FormatInt(item->currentType) + L"\t" + FormatInt(item->currentData) + L"\r\n";
                break;
            case ItemKind::Task:
                text += L"TASK\t" + item->taskFolder + L"\t" + item->taskName + L"\t" +
                        (item->taskEnabled ? L"1" : L"0") + L"\r\n";
                break;
            case ItemKind::Feature:
                text += L"FEATURE\t" + item->id + L"\t" + FormatInt(item->featureState) + L"\r\n";
                break;
        }
    }
    if (!WriteTextFile(manifest, text)) {
        if (error) *error = L"Não foi possível gravar o manifesto em " + manifest;
        return std::wstring();
    }

    std::wstring readme =
        L"Windows Lite Restore - backup de configurações\r\n"
        L"=================================================\r\n\r\n"
        L"Este backup registra o estado anterior dos serviços, políticas e tarefas alterados.\r\n"
        L"Para desfazer, abra o aplicativo e use Ações > Desfazer última execução.\r\n\r\n"
        L"Recursos opcionais instalados pelo DISM não são revertidos automaticamente, pois\r\n"
        L"a remoção de componentes pode deixar o Windows instável. Use a ferramenta DISM\r\n"
        L"manualmente se precisar removê-los.\r\n\r\n"
        L"O manifesto é um arquivo TSV de texto; não o edite manualmente.\r\n";
    WriteTextFile(dir + L"\\LEIA-ME.txt", readme);
    lastBackupPath_ = dir;
    Log(L"Backup criado em: " + dir);
    return dir;
}

bool App::UndoLatestBackup(std::wstring* summary, std::wstring* error) {
    if (summary) summary->clear();
    if (error) error->clear();
    std::wstring base = MakeDataPath(L"Backups");
    if (!DirectoryExists(base)) {
        if (error) *error = L"Nenhum backup foi encontrado em " + base + L".";
        return false;
    }

    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW((base + L"\\*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        if (error) *error = L"Não foi possível listar a pasta de backups.";
        return false;
    }
    std::vector<std::wstring> dirs;
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0) {
            dirs.push_back(data.cFileName);
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
    std::sort(dirs.begin(), dirs.end(), std::greater<std::wstring>());

    std::wstring chosen;
    std::wstring manifest;
    for (const std::wstring& name : dirs) {
        std::wstring candidate = base + L"\\" + name + L"\\manifest.tsv";
        if (FileExists(candidate)) {
            chosen = base + L"\\" + name;
            manifest = candidate;
            break;
        }
    }
    if (chosen.empty()) {
        if (error) *error = L"Nenhum manifesto de backup válido foi encontrado.";
        return false;
    }

    std::wstring content;
    if (!ReadTextFile(manifest, &content)) {
        if (error) *error = L"Não foi possível ler o manifesto: " + manifest;
        return false;
    }
    std::vector<std::wstring> lines = Split(content, L'\n');
    int services = 0, policies = 0, tasks = 0, features = 0, failures = 0;
    std::wstring details;

    for (std::wstring line : lines) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (line.empty() || line[0] == L'#') continue;
        std::vector<std::wstring> f = Split(line, L'\t');
        if (f.size() < 2) continue;
        if (f[0] == L"SERVICE" && f.size() >= 4) {
            DWORD start = static_cast<DWORD>(_wtoi(f[2].c_str()));
            bool delayed = _wtoi(f[3].c_str()) != 0;
            std::wstring err;
            if (SetServiceStart(f[1], start, delayed, &err, false, nullptr)) {
                services++;
                details += L"[OK] Serviço revertido: " + f[1] + L"\r\n";
            } else {
                failures++;
                details += L"[FALHA] Serviço " + f[1] + L": " + err + L"\r\n";
            }
        } else if (f[0] == L"POLICY" && f.size() >= 7) {
            HKEY root = RegRootFromName(f[1]);
            bool existed = _wtoi(f[4].c_str()) != 0;
            DWORD type = static_cast<DWORD>(_wtoi(f[5].c_str()));
            DWORD value = static_cast<DWORD>(_wtoi(f[6].c_str()));
            bool ok = false;
            if (!existed) {
                ok = DeleteRegistryValue(root, f[2], f[3]);
            } else if (type == REG_DWORD) {
                ok = WriteRegistryDword(root, f[2], f[3], value);
            }
            if (ok) {
                policies++;
                details += L"[OK] Política revertida: " + f[3] + L"\r\n";
            } else {
                failures++;
                details += L"[FALHA] Política " + f[3] + L"\r\n";
            }
        } else if (f[0] == L"TASK" && f.size() >= 4) {
            bool enabled = _wtoi(f[3].c_str()) != 0;
            std::wstring err;
            if (EnableScheduledTask(f[1], f[2], enabled, &err)) {
                tasks++;
                details += L"[OK] Tarefa revertida: " + f[2] + L"\r\n";
            } else {
                failures++;
                details += L"[FALHA] Tarefa " + f[2] + L": " + err + L"\r\n";
            }
        } else if (f[0] == L"FEATURE") {
            features++;
        }
    }

    if (summary) {
        *summary = L"Backup usado: " + chosen + L"\r\n\r\n" +
                   L"Serviços revertidos: " + FormatInt(services) + L"\r\n" +
                   L"Políticas revertidas: " + FormatInt(policies) + L"\r\n" +
                   L"Tarefas revertidas: " + FormatInt(tasks) + L"\r\n" +
                   L"Falhas: " + FormatInt(failures) + L"\r\n" +
                   L"Recursos não revertidos (proteção contra remoção de componentes): " +
                   FormatInt(features) + L"\r\n\r\n" + details;
    }
    Log(L"Desfazer backup concluído: " + chosen + L" (falhas: " + FormatInt(failures) + L")");
    return true;
}

// ---------------------------------------------------------------------------
// Aplicação das alterações
// ---------------------------------------------------------------------------

bool App::ApplyPlan(const std::vector<Item*>& plan, bool dryRun, ApplySummary* summary,
                    HANDLE cancelEvent) {
    if (!summary) return false;
    summary->dryRun = dryRun;
    HWND window = Window();

    std::vector<Item*> features;
    std::vector<Item*> services;
    std::vector<Item*> policies;
    std::vector<Item*> tasks;
    for (Item* item : plan) {
        if (!item || !item->selected || !item->needsChange || !item->supported) continue;
        switch (item->kind) {
            case ItemKind::Feature: features.push_back(item); break;
            case ItemKind::Service: services.push_back(item); break;
            case ItemKind::Policy: policies.push_back(item); break;
            case ItemKind::Task: tasks.push_back(item); break;
        }
    }

    const int total = static_cast<int>(features.size() + services.size() +
                                       policies.size() + tasks.size());
    int done = 0;
    auto progress = [&]() {
        if (window) {
            PostMessageW(window, WM_APP_APPLY_PROG, static_cast<WPARAM>(done),
                         static_cast<LPARAM>(total));
        }
    };
    auto cancelled = [&]() {
        return cancelEvent && WaitForSingleObject(cancelEvent, 0) == WAIT_OBJECT_0;
    };
    auto record = [&](bool ok, const std::wstring& text) {
        summary->messages.push_back((ok ? L"[OK] " : L"[FALHA] ") + text);
        Log((ok ? L"[OK] " : L"[FALHA] ") + text);
    };

    auto beginItem = [&](Item* item) {
        done++;
        progress();
        PostStatus(L"Aplicando: " + item->name + L" (" + FormatInt(done) + L"/" +
                   FormatInt(total) + L")");
    };

    Log(L"=== Início da aplicação" + std::wstring(dryRun ? L" (simulação)" : L"") + L" ===");
    Log(L"Itens no plano: " + FormatInt(total));

    // Políticas primeiro: removem bloqueios que poderiam impedir a instalação.
    for (Item* item : policies) {
        if (cancelled()) { summary->cancelled = true; break; }
        beginItem(item);
        if (dryRun) {
            Log(L"[SIMULAÇÃO] Política: " + item->name + L" -> " + item->target);
            summary->success++;
            summary->policiesChanged++;
            continue;
        }
        bool ok = false;
        if (item->desiredExists) {
            ok = WriteRegistryDword(item->hive, item->key, item->valueName, item->desiredData);
        } else {
            ok = DeleteRegistryValue(item->hive, item->key, item->valueName);
        }
        if (ok) {
            summary->success++;
            summary->policiesChanged++;
            record(true, L"Política restaurada: " + item->name);
            if (_wcsicmp(item->valueName.c_str(), L"EnableLUA") == 0 ||
                _wcsicmp(item->valueName.c_str(), L"PromptOnSecureDesktop") == 0 ||
                _wcsicmp(item->valueName.c_str(), L"ConsentPromptBehaviorAdmin") == 0) {
                summary->rebootRequired = true;
            }
        } else {
            summary->failed++;
            std::wstring err = FormatWinError(GetLastError());
            record(false, L"Política " + item->name + L": " + err);
        }
    }

    // Recursos do Windows
    if (!features.empty()) {
        if (dryRun) {
            for (Item* item : features) {
                if (cancelled()) { summary->cancelled = true; break; }
                beginItem(item);
                Log(L"[SIMULAÇÃO] Instalar recurso: " + item->name + L" (" + item->id + L")");
                summary->success++;
                summary->featuresChanged++;
                summary->rebootRequired = true;
            }
        } else {
            DismApi api;
            bool dismReady = false;
            DismSession session = 0;
            if (!api.Load()) {
                Log(L"Falha ao carregar dismapi.dll; os recursos não serão alterados.");
            } else {
                std::wstring dismLog = DismLogPath(*this);
                HRESULT hr = api.Initialize(DismLogErrorsWarningsInfo,
                                            dismLog.empty() ? nullptr : dismLog.c_str(), nullptr);
                if (FAILED(hr)) {
                    Log(L"Falha ao iniciar DISM: " + DismLastError(api, hr));
                } else {
                    hr = api.OpenSession(DISM_ONLINE_IMAGE, nullptr, nullptr, &session);
                    if (SUCCEEDED(hr)) {
                        dismReady = true;
                    } else {
                        Log(L"Falha ao abrir sessão DISM: " + DismLastError(api, hr));
                        api.Shutdown();
                    }
                }
            }

            std::wstring source = settings_.sourcePath;
            if (!source.empty() && !DirectoryExists(source) && !FileExists(source)) {
                Log(L"A pasta de origem configurada não existe: " + source);
                source.clear();
            }
            PCWSTR sourcePaths[1] = {source.c_str()};
            UINT sourceCount = source.empty() ? 0 : 1;

            for (Item* item : features) {
                if (cancelled()) { summary->cancelled = true; break; }
                beginItem(item);
                if (!dismReady) {
                    summary->failed++;
                    record(false, L"Recurso " + item->name + L": DISM indisponível.");
                    continue;
                }

                DismFeatureInfo* info = nullptr;
                HRESULT infoHr = api.GetFeatureInfo
                                     ? api.GetFeatureInfo(session, item->id.c_str(), nullptr,
                                                          DismPackageNone, &info)
                                     : E_FAIL;
                if (SUCCEEDED(infoHr) && info) {
                    if (info->FeatureState == DismStateInstalled ||
                        info->FeatureState == DismStateInstallPending) {
                        summary->skipped++;
                        record(true, L"Recurso já instalado ou pendente: " + item->name);
                        api.Delete(info);
                        continue;
                    }
                }
                if (info) { api.Delete(info); info = nullptr; }

                DismProgressContext context;
                context.window = window;
                HRESULT hr = api.EnableFeature(
                    session, item->id.c_str(), nullptr, DismPackageNone,
                    settings_.useWindowsUpdate ? FALSE : TRUE, sourceCount ? sourcePaths : nullptr,
                    sourceCount, TRUE, cancelEvent, DismProgressCallback, &context);
                if (cancelled()) {
                    summary->cancelled = true;
                    Log(L"Instalação de recursos cancelada durante: " + item->name);
                    break;
                }
                if (SUCCEEDED(hr)) {
                    summary->success++;
                    summary->featuresChanged++;
                    summary->rebootRequired = true;
                    record(true, L"Recurso instalado: " + item->name);
                    DismFeatureInfo* after = nullptr;
                    if (api.GetFeatureInfo &&
                        SUCCEEDED(api.GetFeatureInfo(session, item->id.c_str(), nullptr,
                                                     DismPackageNone, &after)) &&
                        after) {
                        if (after->RestartRequired == DismRestartNo &&
                            after->FeatureState != DismStateInstallPending) {
                            // Alguns recursos não exigem reinicialização.
                            summary->rebootRequired = true;
                        }
                        api.Delete(after);
                    }
                } else {
                    summary->failed++;
                    std::wstring message = L"Recurso " + item->name + L": " + DismLastError(api, hr);
                    std::wstring hint = FeatureHint(hr);
                    if (!hint.empty()) message += L" " + hint;
                    record(false, message);
                }
            }

            if (dismReady) {
                api.CloseSession(session);
                api.Shutdown();
            }
        }
    }

    // Serviços
    for (Item* item : services) {
        if (cancelled()) { summary->cancelled = true; break; }
        beginItem(item);
        if (dryRun) {
            Log(L"[SIMULAÇÃO] Serviço: " + item->name + L" -> " +
                StartTypeText(item->targetStart, item->targetDelayed));
            summary->success++;
            summary->servicesChanged++;
            continue;
        }
        std::wstring err;
        bool started = false;
        bool shouldStart = (item->targetStart == SERVICE_AUTO_START);
        bool ok = SetServiceStart(item->id, item->targetStart, item->targetDelayed, &err,
                                  shouldStart, &started);
        if (ok) {
            summary->success++;
            summary->servicesChanged++;
            std::wstring text = L"Serviço configurado: " + item->name + L" -> " +
                                StartTypeText(item->targetStart, item->targetDelayed);
            if (started) text += L" (iniciado)";
            else if (item->targetStart == SERVICE_DISABLED) text += L" (parado)";
            else if (!err.empty()) {
                text += L" (" + err + L")";
                summary->rebootRequired = true;
            }
            record(true, text);
        } else {
            summary->failed++;
            record(false, L"Serviço " + item->name + L": " + err);
        }
    }

    // Tarefas agendadas
    for (Item* item : tasks) {
        if (cancelled()) { summary->cancelled = true; break; }
        beginItem(item);
        if (dryRun) {
            Log(L"[SIMULAÇÃO] Habilitar tarefa: " + item->name);
            summary->success++;
            summary->tasksChanged++;
            continue;
        }
        std::wstring err;
        if (EnableScheduledTask(item->taskFolder, item->taskName, true, &err)) {
            summary->success++;
            summary->tasksChanged++;
            record(true, L"Tarefa habilitada: " + item->name);
        } else {
            summary->failed++;
            record(false, L"Tarefa " + item->name + L": " + err);
        }
    }

    if (cancelled()) summary->cancelled = true;
    summary->logPath = logPath_;

    // Relatório final
    std::wstring report;
    BuildReport(&report, summary);
    std::wstring reports = MakeDataPath(L"reports");
    EnsureDirectory(reports);
    std::wstring reportPath = reports + L"\\report-" + TimestampNow(true) + L".txt";
    if (WriteTextFile(reportPath, report)) {
        summary->reportPath = reportPath;
        lastReportPath_ = reportPath;
        Log(L"Relatório gravado em: " + reportPath);
    } else {
        Log(L"Não foi possível gravar o relatório em: " + reportPath);
    }

    Log(L"Aplicação concluída. Sucessos: " + FormatInt(summary->success) +
        L", falhas: " + FormatInt(summary->failed) +
        L", ignorados: " + FormatInt(summary->skipped) +
        (summary->cancelled ? L", cancelada." : L"."));
    return true;
}

// ---------------------------------------------------------------------------
// Relatório
// ---------------------------------------------------------------------------

void App::BuildReport(std::wstring* report, const ApplySummary* summary) {
    if (!report) return;
    std::wstring text;
    text += L"Windows Lite Restore — relatório\r\n";
    text += L"Gerado em: " + TimestampNow(false) + L"\r\n";
    text += L"Modo: " + std::wstring(admin_ ? L"administrador" : L"somente leitura") + L"\r\n";
    text += L"Perfil: " + std::wstring(ProfileName(activeProfile_)) + L"\r\n\r\n";
    text += GetSystemSummary();
    text += L"\r\n";

    int pendingRecommended = 0;
    for (const Item& item : items_) {
        if (item.supported && item.recommended && item.needsChange) pendingRecommended++;
    }
    text += L"Itens analisados: " + FormatInt(static_cast<long long>(items_.size())) + L"\r\n";
    text += L"Pendências recomendadas: " + FormatInt(pendingRecommended) + L"\r\n";
    if (!lastScanSummary_.empty()) text += lastScanSummary_ + L"\r\n";
    if (!featureScanError_.empty()) text += L"Recursos: " + featureScanError_ + L"\r\n";
    text += L"\r\n";

    for (const Item& item : items_) {
        const wchar_t* kind = L"Recurso";
        switch (item.kind) {
            case ItemKind::Feature: kind = L"Recurso"; break;
            case ItemKind::Service: kind = L"Serviço"; break;
            case ItemKind::Policy: kind = L"Política"; break;
            case ItemKind::Task: kind = L"Tarefa"; break;
        }
        text += std::wstring(kind) + L": " + item.name;
        if (!item.id.empty()) text += L" [" + item.id + L"]";
        text += L"\r\n  Atual: " + item.current + L"\r\n  Padrão: " + item.target +
                L"\r\n  Categoria: " + item.category;
        if (!item.extra.empty()) text += L"\r\n  Identificador: " + item.extra;
        text += L"\r\n  Recomendado: " + BoolText(item.recommended) +
                L" | Selecionável: " + BoolText(item.supported) +
                L" | Pendente: " + BoolText(item.needsChange) + L"\r\n";
        if (!item.description.empty()) text += L"  Descrição: " + item.description + L"\r\n";
        text += L"\r\n";
    }

    if (summary) {
        text += L"=== Resultado da execução ===\r\n";
        text += L"Sucessos: " + FormatInt(summary->success) + L"\r\n";
        text += L"Falhas: " + FormatInt(summary->failed) + L"\r\n";
        text += L"Ignorados: " + FormatInt(summary->skipped) + L"\r\n";
        text += L"Recursos alterados: " + FormatInt(summary->featuresChanged) + L"\r\n";
        text += L"Serviços alterados: " + FormatInt(summary->servicesChanged) + L"\r\n";
        text += L"Políticas alteradas: " + FormatInt(summary->policiesChanged) + L"\r\n";
        text += L"Tarefas alteradas: " + FormatInt(summary->tasksChanged) + L"\r\n";
        text += L"Reinicialização recomendada: " + BoolText(summary->rebootRequired) + L"\r\n";
        text += L"Simulação: " + BoolText(summary->dryRun) + L"\r\n";
        if (!summary->backupPath.empty()) text += L"Backup: " + summary->backupPath + L"\r\n";
        if (!summary->reportPath.empty()) text += L"Relatório: " + summary->reportPath + L"\r\n";
        text += L"\r\n";
        for (const std::wstring& message : summary->messages) {
            text += message + L"\r\n";
        }
    }
    *report = text;
}

// ---------------------------------------------------------------------------
// Modo headless
// ---------------------------------------------------------------------------

int App::RunHeadless(const std::wstring& reportPath) {
    admin_ = IsElevated();
    readOnly_ = !admin_;
    activeProfile_ = startupProfile_;
    InitLogging();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::wstring error;
    bool ok = ScanAll(&error);
    std::wstring report;
    BuildReport(&report, nullptr);
    if (!error.empty()) report += L"\r\nErro de análise: " + error + L"\r\n";

    std::wstring target = reportPath;
    if (target.empty()) {
        std::wstring dir = MakeDataPath(L"reports");
        EnsureDirectory(dir);
        target = dir + L"\\scan-" + TimestampNow(true) + L".txt";
    } else {
        std::wstring parent = target;
        size_t pos = parent.find_last_of(L"\\/");
        if (pos != std::wstring::npos) parent.resize(pos);
        if (!parent.empty()) EnsureDirectory(parent);
    }
    bool written = WriteTextFile(target, report);
    Log(written ? (L"Relatório headless gravado em: " + target)
                : (L"Falha ao gravar relatório headless em: " + target));
    CloseLogging();
    CoUninitialize();
    return (ok && written) ? 0 : 1;
}

// ---------------------------------------------------------------------------
// Elevação e persistência de seleção
// ---------------------------------------------------------------------------

bool App::IsElevated() const {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION elevation = {};
    DWORD size = sizeof(elevation);
    BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

bool App::SaveSelectionFile(std::wstring* path) {
    if (path) path->clear();
    wchar_t temp[MAX_PATH * 2] = {};
    DWORD length = GetTempPathW(_countof(temp), temp);
    if (!length) return false;
    std::wstring dir = temp;
    if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
    EnsureDirectory(dir);
    std::wstring file = dir + L"WindowsLiteRestore-selection-" +
                        FormatInt(GetCurrentProcessId()) + L".txt";
    std::wstring text;
    for (const Item& item : items_) {
        if (item.selected && item.needsChange && item.supported) {
            const wchar_t* kind = L"F";
            if (item.kind == ItemKind::Service) kind = L"S";
            else if (item.kind == ItemKind::Policy) kind = L"P";
            else if (item.kind == ItemKind::Task) kind = L"T";
            text += std::wstring(kind) + L"\t" + item.id + L"\r\n";
        }
    }
    if (text.empty()) return false;
    if (!WriteTextFile(file, text, false)) return false;
    if (path) *path = file;
    return true;
}

void App::LoadSelectionFile(const std::wstring& path) {
    std::wstring text;
    if (!ReadTextFile(path, &text)) return;
    for (std::wstring line : Split(text, L'\n')) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<std::wstring> parts = Split(line, L'\t');
        if (parts.size() < 2) continue;
        ItemKind kind = ItemKind::Feature;
        if (parts[0] == L"S") kind = ItemKind::Service;
        else if (parts[0] == L"P") kind = ItemKind::Policy;
        else if (parts[0] == L"T") kind = ItemKind::Task;
        for (Item& item : items_) {
            if (item.kind == kind && item.id == parts[1]) {
                item.selected = true;
                break;
            }
        }
    }
    DeleteFileW(path.c_str());
}

bool App::ElevateSelf(bool preserveSelection) {
    std::wstring selection;
    if (preserveSelection && SaveSelectionFile(&selection)) {
        // A seleção é restaurada depois da nova análise.
    }
    std::wstring exe = GetExePath();
    std::wstring args = L"--elevated";
    if (!selection.empty()) args += L" --resume-selection \"" + selection + L"\"";
    HINSTANCE result = ShellExecuteW(nullptr, L"runas", exe.c_str(), args.c_str(), nullptr,
                                     SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        if (!selection.empty()) DeleteFileW(selection.c_str());
        Log(L"Falha ao solicitar elevação: " + FormatWinError(static_cast<DWORD>(
            reinterpret_cast<INT_PTR>(result))));
        return false;
    }
    return true;
}
