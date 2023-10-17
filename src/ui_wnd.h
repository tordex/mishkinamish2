#pragma once

#include "../libs/litehtml/containers/cairo/cairo_container.h"
#include "../libs/litehtml/containers/cairo/cairo_font.h"

using namespace litehtml;

class CHTMLViewWnd : public cairo_container
{
	HWND						m_hWnd;
	HINSTANCE					m_hInst;
	int							m_top;
	int							m_left;
	int							m_max_top;
	int							m_max_left;
	CRITICAL_SECTION			m_sync;
	litehtml::document::ptr		m_doc;
	simpledib::dib				m_dib;
	std::wstring				m_class;
	std::wstring				m_base_path;
	std::wstring				m_url;
	std::wstring				m_cursor;
public:
	CHTMLViewWnd(HINSTANCE	hInst, const wchar_t* class_name, const wchar_t* res_path);
	virtual ~CHTMLViewWnd(void);

	void				create();
	void				open(LPCWSTR url, bool reload = FALSE);
	HWND				wnd()	{ return m_hWnd;	}
	void				refresh();

	void				set_caption();
	void				lock();
	void				unlock();
	bool				is_valid_page(bool with_lock = true);

	void				render();

	// cairo container members
	virtual void		make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out) override;
	virtual image_ptr	get_image(LPCWSTR url, bool redraw_on_ready) override;
	virtual	void		set_caption(const char* caption) override;
	virtual	void		set_base_url(const char* base_url) override;
	virtual void		import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
	virtual	void		on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
	virtual	void		set_cursor(const char* cursor) override;
	virtual void		get_client_rect(litehtml::position& client)  const;

protected:
	virtual void		OnCreate();
	virtual void		OnPaint(simpledib::dib* dib, LPRECT rcDraw);
	virtual void		OnSize(int width, int height);
	virtual void		OnDestroy();
	virtual void		OnVScroll(int pos, int flags);
	virtual void		OnHScroll(int pos, int flags);
	virtual void		OnMouseWheel(int delta);
	virtual void		OnKeyDown(UINT vKey);
	virtual void		OnMouseMove(int x, int y);
	virtual void		OnLButtonDown(int x, int y);
	virtual void		OnLButtonUp(int x, int y);
	virtual void		OnMouseLeave();
	
	void				redraw(LPRECT rcDraw, BOOL update);
	void				update_cursor();
	void				create_dib(int width, int height);
	
	litehtml::document::ptr get_doc() { return m_doc;  }
	

private:
	static LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	char* load_text_file(const std::wstring& path);
};

inline void CHTMLViewWnd::lock()
{
	EnterCriticalSection(&m_sync);
}

inline void CHTMLViewWnd::unlock()
{
	LeaveCriticalSection(&m_sync);
}
