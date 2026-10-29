#include "app.h"

#include <objbase.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int showCmd) {
    INITCOMMONCONTROLSEX controls = {};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES |
                     ICC_PROGRESS_CLASS | ICC_TAB_CLASSES;
    InitCommonControlsEx(&controls);

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
        MessageBoxW(nullptr, L"Não foi possível inicializar o COM.", L"Windows Lite Restore",
                    MB_ICONERROR | MB_OK);
        return 1;
    }

    const wchar_t* args = commandLine ? commandLine : L"";
    int result = App::Instance().Run(instance, args, showCmd);

    if (SUCCEEDED(com)) CoUninitialize();
    return result;
}
