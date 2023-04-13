#include "WindowHandler.h"
#include "util/Logger.h"
#include <WinUser.h>
#include <Windowsx.h>
#include <atlbase.h>
#include <atlwin.h>
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

bool closeRequest = false;
std::map<HWND, WindowHandler::Window*> WindowHandler::Window::m_allWindowInstances;

WindowHandler::POINTER_INFO WindowHandler::Window::parsePointerInfo(Window* w, WPARAM wParam, LPARAM lParam) {
	WindowHandler::POINTER_INFO info;

	info.id = GET_POINTERID_WPARAM(wParam);
	POINTER_INPUT_TYPE pointerType = PT_POINTER;
	GetPointerType(info.id, &pointerType);

	POINT p = { 0, 0 };
	ClientToScreen(w->m_hwnd, &p);

	tagPOINTER_INFO pointerinfo;
	switch (pointerType) {
	case PT_TOUCH:
		POINTER_TOUCH_INFO touchinfo;
		info.type = WindowHandler::POINTER_TYPE::TOUCH;
		if (!GetPointerTouchInfo(info.id, &touchinfo)) {
			return info;
		}
		pointerinfo = touchinfo.pointerInfo;
		break;
	case PT_PEN:
		POINTER_PEN_INFO peninfo;
		info.type = WindowHandler::POINTER_TYPE::STYLUS;
		if (!GetPointerPenInfo(info.id, &peninfo)) {
			return info;
		}
		info.pressure = peninfo.pressure;
		info.button1pressed = CHECK_BIT(peninfo.penFlags, 0) != 0;
		info.button2pressed = CHECK_BIT(peninfo.penFlags, 1) != 0 || CHECK_BIT(peninfo.penFlags, 2) != 0;
		pointerinfo = peninfo.pointerInfo;
		break;
	default:
		info.type = WindowHandler::POINTER_TYPE::MOUSE;
		if (!GetPointerInfo(info.id, &pointerinfo)) {
			return info;
		}
		info.button1pressed = CHECK_BIT(pointerinfo.pointerFlags, 4) != 0;
		info.button2pressed = CHECK_BIT(pointerinfo.pointerFlags, 5) != 0;
		info.button3pressed = CHECK_BIT(pointerinfo.pointerFlags, 6) != 0;
		info.button4pressed = CHECK_BIT(pointerinfo.pointerFlags, 7) != 0;
		info.button5pressed = CHECK_BIT(pointerinfo.pointerFlags, 8) != 0;
		break;
	}
	//ScreenToClient(w->m_hwnd, &pointerinfo.ptPixelLocation);
	info.pos.x = pointerinfo.ptPixelLocation.x - p.x;
	info.pos.y = pointerinfo.ptPixelLocation.y - p.y;
	//info.pos = Point2D<float>(pointerinfo.ptPixelLocation);
	info.pos = w->PxToDp(info.pos);

	return info;
}

LRESULT WindowHandler::Window::parseWindowMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window* currentInstance = m_allWindowInstances[hWnd];
	if (currentInstance == nullptr)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_CLOSE:
		closeRequest = true;
		break;
	case WM_POINTERDOWN:
	{
		if (currentInstance->PointerDownCallback == nullptr)
			break;
		currentInstance->PointerDownCallback(parsePointerInfo(currentInstance, wParam, lParam));
		return 0;
	}
	case WM_POINTERUPDATE: 
	{
		if (currentInstance->PointerMoveCallback == nullptr)
			break;
		currentInstance->PointerMoveCallback(parsePointerInfo(currentInstance, wParam, lParam));
		return 0;
	}
	case WM_POINTERUP:
	{
		if (currentInstance->PointerUpCallback == nullptr)
			break;
		currentInstance->PointerUpCallback(parsePointerInfo(currentInstance, wParam, lParam));
		return 0;
	}
	case WM_POINTERHWHEEL:
	{
		if (currentInstance->PointerScrollCallback == nullptr)
			break;
		POINT ppp = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(currentInstance->m_hwnd, &ppp);
		auto p = Point2D<float>(ppp);
		currentInstance->PointerScrollCallback(-GET_WHEEL_DELTA_WPARAM(wParam), true, currentInstance->PxToDp(p));
		return 0;
	}
	case WM_POINTERWHEEL:
	{
		if (currentInstance->PointerScrollCallback == nullptr)
			break; 
		POINT ppp = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(currentInstance->m_hwnd, &ppp);
		auto p = Point2D<float>(ppp);
		currentInstance->PointerScrollCallback(GET_WHEEL_DELTA_WPARAM(wParam), GetKeyState(VK_SHIFT) & 0x8000, currentInstance->PxToDp(p));
		return 0;
	}
	case WM_SIZE:
	case WM_SIZING:
	{
		if (currentInstance->m_renderContext == nullptr)
			break;
		Rect2D<UINT> area;
		RECT r;
		GetClientRect(currentInstance->m_hwnd, &r);
		area.width = r.right;
		area.height = r.bottom;
		currentInstance->m_renderContext->resize(area);
		return 0;
	}
	case WM_PAINT:
	{
		if (currentInstance->m_renderContext != nullptr) {
			currentInstance->m_renderContext->render();
		}
		if (currentInstance->WindowPaintCallback != nullptr)
			currentInstance->WindowPaintCallback();
			
		ValidateRect(currentInstance->m_hwnd, NULL);
		return 0;
	}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

WindowHandler::Window::Window() {}

WindowHandler::Window::~Window() {
	delete m_renderContext;
}

bool WindowHandler::Window::init(std::wstring windowName, HINSTANCE instance) {
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = reinterpret_cast<WNDPROC>(this->parseWindowMessages);
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(3289650);
	wc.lpszClassName = windowName.c_str();

	if (!RegisterClass(&wc)) {
		Logger::err(L"Error registering Window");
		return false;
	}

	m_hwnd = CreateWindow(wc.lpszClassName, windowName.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

	if (!m_hwnd) {
		Logger::err(L"Error creating Window");
		return false;
	}

	m_hdc = GetDC(m_hwnd);

	if (!m_hwnd) {
		Logger::err(L"Couldn't retrieve DC");
		return false;
	}

	m_allWindowInstances[m_hwnd] = this;

	bool b = EnableMouseInPointer(true);
	if (!b) {
		Logger::add(L"Couldn't add Mouse input into Pointer Input Stack API");
	}


	Logger::add(L"DPI: " + std::to_wstring(GetDpiForSystem()));
	m_dpi = GetDpiForWindow(m_hwnd);
	
	// Create a new Render Context
	m_renderContext = new RenderHandler::Direct2DContext(this);
	

	// force a redraw of the surface
	InvalidateRect(m_hwnd, NULL, true);
	UpdateWindow(m_hwnd);
	ShowWindow(m_hwnd, SW_SHOWNORMAL);


	return true;
}

RenderHandler::Direct2DContext* WindowHandler::Window::getRenderContext() {
	return m_renderContext;
}

void WindowHandler::Window::forceDraw() {
	InvalidateRect(m_hwnd, NULL, true);
	UpdateWindow(m_hwnd);
}

void WindowHandler::Window::setState(WINDOW_STATE state) {
	int nCmdShow = 0;
	switch (state) {
	case WINDOW_STATE::HIDDEN:    nCmdShow = SW_HIDE;     break;
	case WINDOW_STATE::MAXIMIZED: nCmdShow = SW_MAXIMIZE; break;
	case WINDOW_STATE::NORMAL:    nCmdShow = SW_RESTORE;  break;
	case WINDOW_STATE::MINIMIZED: nCmdShow = SW_MINIMIZE; break;
	}
	ShowWindow(m_hwnd, nCmdShow);
}

void WindowHandler::Window::getMsg(bool blocking) {
	MSG msg;
	BOOL result;

	//first block
	if (blocking)
		result = GetMessage(&msg, 0, 0, 0);
	else
		result = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);

	while (result != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		//now just peek messages
		result = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
	}
}

bool WindowHandler::Window::getCloseRequest() {
	return closeRequest;
}

void WindowHandler::Window::setPointerDownCallback(void(*fun)(POINTER_INFO)) {
	PointerDownCallback = fun;
}

void WindowHandler::Window::setPointerUpdateCallback(void(*fun)(POINTER_INFO)) {
	PointerMoveCallback = fun;
}

void WindowHandler::Window::setPointerUpCallback(void(*fun)(POINTER_INFO)) {
	PointerUpCallback = fun;
}

void WindowHandler::Window::setPointerScrollCallback(void(*fun)(SHORT, bool, Point2D<int>)) {
	PointerScrollCallback = fun;
}

void WindowHandler::Window::setWindowPaintCallback(void(*fun)()) {
	WindowPaintCallback = fun;
}

std::wstring WindowHandler::Window::getOpenFileDialogBox(const wchar_t* filter) {
	// Initialize the OPENFILENAME structure
	TCHAR szFile[MAX_PATH] = { 0 };
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd; // set the parent window handle
	ofn.lpstrFilter = filter; // set the file filter
	ofn.lpstrFile = szFile; // set the file name buffer
	ofn.nMaxFile = MAX_PATH; // set the maximum file name length

	// Open the file dialog
	if (GetOpenFileName(&ofn) == TRUE) {
		return szFile;
	}
	return std::wstring();
}

std::wstring WindowHandler::Window::getSaveFileDialogBox(const wchar_t* filter) {
	// TODO
	
	return std::wstring();
}

Rect2D<long> WindowHandler::Window::getClientSize() const {
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	return Rect2D<long>(rect);
}
