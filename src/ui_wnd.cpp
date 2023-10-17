#include "globals.h"
#include "ui_wnd.h"
#include <WindowsX.h>
#include <algorithm>
#include <strsafe.h>

CHTMLViewWnd::CHTMLViewWnd(HINSTANCE hInst, const wchar_t* class_name, const wchar_t* res_path)
{
    m_hInst			= hInst;
    m_class			= class_name;
    m_hWnd			= NULL;
    m_top			= 0;
    m_left			= 0;
    m_max_top		= 0;
    m_max_left		= 0;
    m_base_path     = res_path;

    InitializeCriticalSection(&m_sync);

    WNDCLASS wc;
    if(!GetClassInfo(m_hInst, m_class.c_str(), &wc))
    {
        ZeroMemory(&wc, sizeof(wc));
        wc.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc    = (WNDPROC)CHTMLViewWnd::WndProc;
        wc.cbClsExtra     = 0;
        wc.cbWndExtra     = 0;
        wc.hInstance      = m_hInst;
        wc.hIcon          = NULL;
        wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = m_class.c_str();

        RegisterClass(&wc);
    }
}

CHTMLViewWnd::~CHTMLViewWnd(void)
{
    DeleteCriticalSection(&m_sync);
}

LRESULT CALLBACK CHTMLViewWnd::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    CHTMLViewWnd* pThis = NULL;
    if(IsWindow(hWnd))
    {
        pThis = (CHTMLViewWnd*)GetProp(hWnd, TEXT("htmlview_this"));
        if(pThis && pThis->m_hWnd != hWnd)
        {
            pThis = NULL;
        }
    }

    if(pThis || uMessage == WM_CREATE)
    {
        switch (uMessage)
        {
        case WM_SETCURSOR:
            pThis->update_cursor();
            break;
        case WM_ERASEBKGND:
            return TRUE;
        case WM_CREATE:
            {
                LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
                pThis = (CHTMLViewWnd*)(lpcs->lpCreateParams);
                SetProp(hWnd, TEXT("htmlview_this"), (HANDLE) pThis);
                pThis->m_hWnd = hWnd;
                pThis->OnCreate();
            }
            break;
        case WM_PAINT:
            {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                pThis->create_dib(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                pThis->OnPaint(&pThis->m_dib, &ps.rcPaint);

                BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
                    ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top, pThis->m_dib, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

                EndPaint(hWnd, &ps);
            }
            return 0;
        case WM_SIZE:
            pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_DESTROY:
            RemoveProp(hWnd, TEXT("htmlview_this"));
            pThis->OnDestroy();
            return 0;
        case WM_VSCROLL:
            pThis->OnVScroll(HIWORD(wParam), LOWORD(wParam));
            return 0;
        case WM_HSCROLL:
            pThis->OnHScroll(HIWORD(wParam), LOWORD(wParam));
            return 0;
        case WM_MOUSEWHEEL:
            pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;
        case WM_KEYDOWN:
            pThis->OnKeyDown((UINT) wParam);
            return 0;
        case WM_MOUSEMOVE:
            {
                TRACKMOUSEEVENT tme;
                ZeroMemory(&tme, sizeof(TRACKMOUSEEVENT));
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags		= TME_QUERY;
                tme.hwndTrack	= hWnd;
                TrackMouseEvent(&tme);
                if(!(tme.dwFlags & TME_LEAVE))
                {
                    tme.dwFlags		= TME_LEAVE;
                    tme.hwndTrack	= hWnd;
                    TrackMouseEvent(&tme);
                }
                pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
        case WM_MOUSELEAVE:
            pThis->OnMouseLeave();
            return 0;
        case WM_LBUTTONDOWN:
            pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
            pThis->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

char* CHTMLViewWnd::load_text_file(const std::wstring& path)
{
    char* ret = NULL;

    HANDLE fl = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fl != INVALID_HANDLE_VALUE)
    {
        DWORD size = GetFileSize(fl, NULL);
        ret = new char[size + 1];

        DWORD cbRead = 0;
        if (size >= 3)
        {
            ReadFile(fl, ret, 3, &cbRead, NULL);
            if (ret[0] == '\xEF' && ret[1] == '\xBB' && ret[2] == '\xBF')
            {
                ReadFile(fl, ret, size - 3, &cbRead, NULL);
                ret[cbRead] = 0;
            }
            else
            {
                ReadFile(fl, ret + 3, size - 3, &cbRead, NULL);
                ret[cbRead + 3] = 0;
            }
        }
        CloseHandle(fl);
    }

    return ret;
}

void CHTMLViewWnd::OnCreate()
{

}

void CHTMLViewWnd::OnPaint( simpledib::dib* dib, LPRECT rcDraw )
{
    cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*) dib->bits(), CAIRO_FORMAT_ARGB32, dib->width(), dib->height(), dib->width() * 4);
    cairo_t* cr = cairo_create(surface);

    cairo_rectangle(cr, rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
    cairo_clip(cr);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    lock();

    auto doc = get_doc();

    if(doc)
    {
        litehtml::position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
        doc->draw((litehtml::uint_ptr) cr, -m_left, -m_top, &clip);
    }

    unlock();

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void CHTMLViewWnd::OnSize( int width, int height )
{
    auto doc = get_doc();

    if(doc)
    {
        doc->media_changed();
    }

    render();
}

void CHTMLViewWnd::OnDestroy()
{

}

void CHTMLViewWnd::create()
{
    m_hWnd = CreateWindow(m_class.c_str(), L"Light HTML", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInst, (LPVOID)this);

    ShowWindow(m_hWnd, SW_SHOW);
}

void CHTMLViewWnd::open( LPCWSTR url, bool reload )
{
    std::wstring path;
    make_url(url, nullptr, path);
    char* html = load_text_file(path);

    if (!html)
    {
        LPCSTR txt = "<h1>Something Wrong</h1>";
        html = new char[lstrlenA(txt) + 1];
        lstrcpyA(html, txt);
    }

    m_doc = litehtml::document::createFromString(html, this);
    delete html;

    render();
    m_top = 0;
    m_left = 0;
    redraw(NULL, FALSE);
    set_caption();
}

void CHTMLViewWnd::make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out)
{
    out = m_base_path;
    out += L"\\";
    out += url;
}

cairo_container::image_ptr CHTMLViewWnd::get_image(LPCWSTR url, bool redraw_on_ready)
{
    cairo_container::image_ptr img;
    img = cairo_container::image_ptr(new CTxDIB);
    if (!img->load(url))
    {
        img = nullptr;
    }
    return img;
}

void CHTMLViewWnd::set_caption(const char* caption)
{
}

void CHTMLViewWnd::set_base_url(const char* base_url)
{
}

void CHTMLViewWnd::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
    std::wstring css_url;
    make_url_utf8(url.c_str(), baseurl.c_str(), css_url);

    LPSTR css = load_text_file(css_url.c_str());
    if (css)
    {
        text = css;
        delete css;
    }
}

void CHTMLViewWnd::on_anchor_click(const char* url, const litehtml::element::ptr& el)
{
    std::wstring anchor;
    make_url_utf8(url, NULL, anchor);
    open(anchor.c_str());
}

void CHTMLViewWnd::render()
{
    if(!m_hWnd)
    {
        return;
    }

    auto doc = get_doc();

    if(doc)
    {
        RECT rcClient;
        GetClientRect(m_hWnd, &rcClient);

        int width	= rcClient.right - rcClient.left;
        int height	= rcClient.bottom - rcClient.top;

        doc->render(width);
        redraw(NULL, FALSE);
    }
}

void CHTMLViewWnd::redraw(LPRECT rcDraw, BOOL update)
{
    if(m_hWnd)
    {
        InvalidateRect(m_hWnd, rcDraw, TRUE);
        if(update)
        {
            UpdateWindow(m_hWnd);
        }
    }
}

void CHTMLViewWnd::OnVScroll( int pos, int flags )
{
}

void CHTMLViewWnd::OnHScroll( int pos, int flags )
{
}

void CHTMLViewWnd::OnMouseWheel( int delta )
{
}

void CHTMLViewWnd::OnKeyDown( UINT vKey )
{
    switch(vKey)
    {
    case VK_F5:
        refresh();
        break;
    }
}

void CHTMLViewWnd::refresh()
{
    if(!m_url.empty())
    {
        open(m_url.c_str(), true);
    }
}

void CHTMLViewWnd::set_caption()
{
}

void CHTMLViewWnd::OnMouseMove( int x, int y )
{
    auto doc = get_doc();

    if(doc)
    {
        litehtml::position::vector redraw_boxes;
        if(doc->on_mouse_over(x + m_left, y + m_top, x, y, redraw_boxes))
        {
            for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
            {
                box->x -= m_left;
                box->y -= m_top;
                RECT rcRedraw;
                rcRedraw.left	= box->left();
                rcRedraw.right	= box->right();
                rcRedraw.top	= box->top();
                rcRedraw.bottom	= box->bottom();
                redraw(&rcRedraw, FALSE);
            }
            UpdateWindow(m_hWnd);
            update_cursor();
        }
    }
}

void CHTMLViewWnd::OnMouseLeave()
{
    auto doc = get_doc();

    if(doc)
    {
        litehtml::position::vector redraw_boxes;
        if(doc->on_mouse_leave(redraw_boxes))
        {
            for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
            {
                box->x -= m_left;
                box->y -= m_top;
                RECT rcRedraw;
                rcRedraw.left	= box->left();
                rcRedraw.right	= box->right();
                rcRedraw.top	= box->top();
                rcRedraw.bottom	= box->bottom();
                redraw(&rcRedraw, FALSE);
            }
            UpdateWindow(m_hWnd);
        }
    }
}

void CHTMLViewWnd::OnLButtonDown( int x, int y )
{
    auto doc = get_doc();

    if(doc)
    {
        litehtml::position::vector redraw_boxes;
        if(doc->on_lbutton_down(x + m_left, y + m_top, x, y, redraw_boxes))
        {
            for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
            {
                box->x -= m_left;
                box->y -= m_top;
                RECT rcRedraw;
                rcRedraw.left	= box->left();
                rcRedraw.right	= box->right();
                rcRedraw.top	= box->top();
                rcRedraw.bottom	= box->bottom();
                redraw(&rcRedraw, FALSE);
            }
            UpdateWindow(m_hWnd);
        }
    }
}

void CHTMLViewWnd::OnLButtonUp( int x, int y )
{
    auto doc = get_doc();

    if(doc)
    {
        litehtml::position::vector redraw_boxes;
        if(doc->on_lbutton_up(x + m_left, y + m_top, x, y, redraw_boxes))
        {
            for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
            {
                box->x -= m_left;
                box->y -= m_top;
                RECT rcRedraw;
                rcRedraw.left	= box->left();
                rcRedraw.right	= box->right();
                rcRedraw.top	= box->top();
                rcRedraw.bottom	= box->bottom();
                redraw(&rcRedraw, FALSE);
            }
            UpdateWindow(m_hWnd);
        }
    }
}

void CHTMLViewWnd::update_cursor()
{
    LPCWSTR defArrow = IDC_ARROW;

    auto doc = get_doc();

    if(!doc)
    {
        SetCursor(LoadCursor(NULL, defArrow));
    } else
    {
        if(m_cursor == L"pointer")
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        } else
        {
            SetCursor(LoadCursor(NULL, defArrow));
        }
    }
}

void CHTMLViewWnd::set_cursor(const char* cursor)
{
    LPWSTR v = cairo_font::utf8_to_wchar(cursor);
    if (v)
    {
        m_cursor = v;
        delete v;
    }
}

void CHTMLViewWnd::get_client_rect( litehtml::position& client ) const
{
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    client.x		= rcClient.left;
    client.y		= rcClient.top;
    client.width	= rcClient.right - rcClient.left;
    client.height	= rcClient.bottom - rcClient.top;
}

bool CHTMLViewWnd::is_valid_page(bool with_lock)
{
    if (m_doc) return true;
    return false;
}

void CHTMLViewWnd::create_dib( int width, int height )
{
    if(m_dib.width() < width || m_dib.height() < height)
    {
        m_dib.destroy();
        m_dib.create(width, height, true);
    }
}
