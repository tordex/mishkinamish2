#include "globals.h"
#include "mishkinamish2.h"
#include "..\libs\litehtml\containers\cairo\cairo_font.h"
#include "ui_wnd.h"

CRITICAL_SECTION cairo_font::m_sync;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    CoInitialize(NULL);
    InitCommonControls();

    InitializeCriticalSectionAndSpinCount(&cairo_font::m_sync, 1000);

    {
        CHTMLViewWnd wnd(hInstance, L"mishkinamish_class", L"C:\\Users\\tordex\\source\\repos\\mishkinamish2\\src\\html");

        wnd.create();
        wnd.open(L"index.html");

        MSG msg;

        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
