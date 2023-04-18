#include "WindowHandler.h"
#include "util/Logger.h"
#include <WinUser.h>
#include <Windowsx.h>
#include <atlbase.h>
#include <atlwin.h>
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

bool closeRequest = false;
std::map<HWND, WindowHandler::Window*> WindowHandler::Window::m_allWindowInstances;


static WindowHandler::VK getVirtualKeyCode(int windowsKey) {
	switch (windowsKey) {
	case VK_LBUTTON:                           return WindowHandler::VK::LEFT_MB;
	case VK_RBUTTON:		                   return WindowHandler::VK::RIGHT_MB;
	case VK_CANCEL:			                   return WindowHandler::VK::CANCEL;
	case VK_MBUTTON:		                   return WindowHandler::VK::MIDDLE_MB;
	case VK_XBUTTON1:		                   return WindowHandler::VK::X1_MB;
	case VK_XBUTTON2:		                   return WindowHandler::VK::X2_MB;
	case VK_LSHIFT:			                   return WindowHandler::VK::LEFT_SHIFT;
	case VK_RSHIFT:			                   return WindowHandler::VK::RIGHT_SHIFT;
	case VK_LCONTROL:		                   return WindowHandler::VK::LEFT_CONTROL;
	case VK_RCONTROL:		                   return WindowHandler::VK::RIGHT_CONTROL;
	case VK_BACK:			                   return WindowHandler::VK::BACKSPACE;
	case VK_TAB:			                   return WindowHandler::VK::TAB;
	case VK_RETURN:			                   return WindowHandler::VK::ENTER;
	case VK_MENU:			                   return WindowHandler::VK::ALT;
	case VK_PAUSE:			                   return WindowHandler::VK::PAUSE;
	case VK_CAPITAL:		                   return WindowHandler::VK::CAPSLOCK;
	case VK_ESCAPE:			                   return WindowHandler::VK::ESCAPE;
	case VK_SPACE:			                   return WindowHandler::VK::SPACE;
	case VK_PRIOR:			                   return WindowHandler::VK::PAGE_UP;
	case VK_NEXT:			                   return WindowHandler::VK::PAGE_DOWN;
	case VK_END:			                   return WindowHandler::VK::END;
	case VK_HOME:			                   return WindowHandler::VK::HOME;
	case VK_LEFT:			                   return WindowHandler::VK::LEFTARROW;
	case VK_UP:				                   return WindowHandler::VK::UPARROW;
	case VK_RIGHT:			                   return WindowHandler::VK::RIGHTARROW;
	case VK_DOWN:			                   return WindowHandler::VK::DOWNARROW;
	case VK_SELECT:			                   return WindowHandler::VK::SELECT;
	case VK_PRINT:			                   return WindowHandler::VK::PRINT;
	case VK_EXECUTE:		                   return WindowHandler::VK::EXECUTE;
	case VK_SNAPSHOT:		                   return WindowHandler::VK::PRINT_SCREEN;
	case VK_INSERT:			                   return WindowHandler::VK::INSERT;
	case VK_DELETE:			                   return WindowHandler::VK::DEL;
	case VK_HELP:			                   return WindowHandler::VK::HELP;
	case 0x30:				                   return WindowHandler::VK::KEY_0;
	case 0x31:				                   return WindowHandler::VK::KEY_1;
	case 0x32:				                   return WindowHandler::VK::KEY_2;
	case 0x33:				                   return WindowHandler::VK::KEY_3;
	case 0x34:				                   return WindowHandler::VK::KEY_4;
	case 0x35:				                   return WindowHandler::VK::KEY_5;
	case 0x36:				                   return WindowHandler::VK::KEY_6;
	case 0x37:				                   return WindowHandler::VK::KEY_7;
	case 0x38:				                   return WindowHandler::VK::KEY_8;
	case 0x39:				                   return WindowHandler::VK::KEY_9;
	case 0x41:				                   return WindowHandler::VK::A;
	case 0x42:				                   return WindowHandler::VK::B;
	case 0x43:				                   return WindowHandler::VK::C;
	case 0x44:				                   return WindowHandler::VK::D;
	case 0x45:				                   return WindowHandler::VK::E;
	case 0x46:				                   return WindowHandler::VK::F;
	case 0x47:				                   return WindowHandler::VK::G;
	case 0x48:				                   return WindowHandler::VK::H;
	case 0x49:				                   return WindowHandler::VK::I;
	case 0x4a:				                   return WindowHandler::VK::J;
	case 0x4b:				                   return WindowHandler::VK::K;
	case 0x4c:				                   return WindowHandler::VK::L;
	case 0x4d:				                   return WindowHandler::VK::M;
	case 0x4e:				                   return WindowHandler::VK::N;
	case 0x4f:				                   return WindowHandler::VK::O;
	case 0x50:				                   return WindowHandler::VK::P;
	case 0x51:				                   return WindowHandler::VK::Q;
	case 0x52:				                   return WindowHandler::VK::R;
	case 0x53:				                   return WindowHandler::VK::S;
	case 0x54:				                   return WindowHandler::VK::T;
	case 0x55:				                   return WindowHandler::VK::U;
	case 0x56:				                   return WindowHandler::VK::V;
	case 0x57:				                   return WindowHandler::VK::W;
	case 0x58:				                   return WindowHandler::VK::X;
	case 0x59:				                   return WindowHandler::VK::Y;
	case 0x5a:				                   return WindowHandler::VK::Z;
	case VK_LWIN:			                   return WindowHandler::VK::LEFT_WINDOWS;
	case VK_RWIN:			                   return WindowHandler::VK::RIGHT_WINDOWS;
	case VK_APPS:			                   return WindowHandler::VK::APPLICATION;
	case VK_SLEEP:			                   return WindowHandler::VK::SLEEP;
	case VK_SCROLL:			                   return WindowHandler::VK::SCROLL_LOCK;
	case VK_LMENU:			                   return WindowHandler::VK::LEFT_MENU;
	case VK_RMENU:			                   return WindowHandler::VK::RIGHT_MENU;
	case VK_VOLUME_MUTE:	                   return WindowHandler::VK::VOLUME_MUTE;
	case VK_VOLUME_DOWN:	                   return WindowHandler::VK::VOLUME_DOWN;
	case VK_VOLUME_UP:		                   return WindowHandler::VK::VOLUME_UP;
	case VK_MEDIA_NEXT_TRACK:                  return WindowHandler::VK::MEDIA_NEXT;
	case VK_MEDIA_PREV_TRACK:                  return WindowHandler::VK::MEDIA_LAST;
	case VK_MEDIA_STOP:		                   return WindowHandler::VK::MEDIA_STOP;
	case VK_MEDIA_PLAY_PAUSE:                  return WindowHandler::VK::MEDIA_PLAY_PAUSE;
	case VK_OEM_1:			                   return WindowHandler::VK::OEM_1;
	case VK_OEM_2:			                   return WindowHandler::VK::OEM_2;
	case VK_OEM_3:			                   return WindowHandler::VK::OEM_3;
	case VK_OEM_4:			                   return WindowHandler::VK::OEM_4;
	case VK_OEM_5:			                   return WindowHandler::VK::OEM_5;
	case VK_OEM_6:			                   return WindowHandler::VK::OEM_6;
	case VK_OEM_7:			                   return WindowHandler::VK::OEM_7;
	case VK_OEM_8:			                   return WindowHandler::VK::OEM_8;
	case VK_OEM_102:                           return WindowHandler::VK::OEM_102;
	case VK_OEM_CLEAR:		                   return WindowHandler::VK::OEM_CLEAR;
	case VK_OEM_PLUS:		                   return WindowHandler::VK::OEM_PLUS;
	case VK_OEM_COMMA:		                   return WindowHandler::VK::OEM_COMMA;
	case VK_OEM_MINUS:		                   return WindowHandler::VK::OEM_MINUS;
	case VK_OEM_PERIOD:		                   return WindowHandler::VK::OEM_PERIOD;
	case VK_NUMPAD0:		                   return WindowHandler::VK::NUMPAD_0;
	case VK_NUMPAD1:		                   return WindowHandler::VK::NUMPAD_1;
	case VK_NUMPAD2:		                   return WindowHandler::VK::NUMPAD_2;
	case VK_NUMPAD3:		                   return WindowHandler::VK::NUMPAD_3;
	case VK_NUMPAD4:		                   return WindowHandler::VK::NUMPAD_4;
	case VK_NUMPAD5:		                   return WindowHandler::VK::NUMPAD_5;
	case VK_NUMPAD6:		                   return WindowHandler::VK::NUMPAD_6;
	case VK_NUMPAD7:		                   return WindowHandler::VK::NUMPAD_7;
	case VK_NUMPAD8:		                   return WindowHandler::VK::NUMPAD_8;
	case VK_NUMPAD9:		                   return WindowHandler::VK::NUMPAD_9;
	case VK_MULTIPLY:		                   return WindowHandler::VK::NUMPAD_MULTIPLY;
	case VK_ADD:			                   return WindowHandler::VK::NUMPAD_ADD;
	case VK_SEPARATOR:		                   return WindowHandler::VK::NUMPAD_SEPERATOR;
	case VK_SUBTRACT:		                   return WindowHandler::VK::NUMPAD_SUBTRACT;
	case VK_DECIMAL:		                   return WindowHandler::VK::NUMPAD_COMMA;
	case VK_DIVIDE:			                   return WindowHandler::VK::NUMPAD_DIVIDE;
	case VK_NUMLOCK:		                   return WindowHandler::VK::NUMPAD_LOCK;
	case VK_F1:				                   return WindowHandler::VK::F1;
	case VK_F2:				                   return WindowHandler::VK::F2;
	case VK_F3:				                   return WindowHandler::VK::F3;
	case VK_F4:				                   return WindowHandler::VK::F4;
	case VK_F5:				                   return WindowHandler::VK::F5;
	case VK_F6:				                   return WindowHandler::VK::F6;
	case VK_F7:				                   return WindowHandler::VK::F7;
	case VK_F8:				                   return WindowHandler::VK::F8;
	case VK_F9:				                   return WindowHandler::VK::F9;
	case VK_F10:			                   return WindowHandler::VK::F10;
	case VK_F11:			                   return WindowHandler::VK::F11;
	case VK_F12:			                   return WindowHandler::VK::F12;
	case VK_F13:			                   return WindowHandler::VK::F13;
	case VK_F14:			                   return WindowHandler::VK::F14;
	case VK_F15:			                   return WindowHandler::VK::F15;
	case VK_F16:			                   return WindowHandler::VK::F16;
	case VK_F17:			                   return WindowHandler::VK::F17;
	case VK_F18:			                   return WindowHandler::VK::F18;
	case VK_F19:			                   return WindowHandler::VK::F19;
	case VK_F20:			                   return WindowHandler::VK::F20;
	case VK_F21:			                   return WindowHandler::VK::F21;
	case VK_F22:			                   return WindowHandler::VK::F22;
	case VK_F23:			                   return WindowHandler::VK::F23;
	case VK_F24:			                   return WindowHandler::VK::F24;
	case VK_PLAY:			                   return WindowHandler::VK::PLAY;
	case VK_ZOOM: 			                   return WindowHandler::VK::ZOOM;
	default:				                   return WindowHandler::VK::UNKWON;
	}
}

//Will only check for CTRL, ALT and SHIFT. It is rather ugly but it gets the job done
// https://github.com/Gamanius/GRenderer-Project/blob/master/Renderer/src/core/window/Window.cpp
// i dont know how the code works but it works
static std::vector<WindowHandler::VK> processUnknownKeys(bool pressed) {
	std::vector<WindowHandler::VK> returnValue;
	static int keyToCheck[6] = {
		//Shift               //Control                 //Alt
		VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU
	};

	static bool wasPressed[] = {
		0, 0, 0, 0, 0, 0
	};

	for (byte i = 0; i < 6; i++) {
		auto key = GetKeyState(keyToCheck[i]);
		char isPressed = (key & -128) + 129;
		if (isPressed == 1 && pressed) {
			wasPressed[i] = true;
			returnValue.push_back(getVirtualKeyCode(keyToCheck[i]));
		}
		else if (isPressed != 1 && !pressed && wasPressed[i]) {
			wasPressed[i] = false;
			returnValue.push_back(getVirtualKeyCode(keyToCheck[i]));
		}
	}

	return returnValue;
}

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
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		if (currentInstance->KeyDownCallback == nullptr) 
			return 0;
		auto key = getVirtualKeyCode(wParam);

		if (key == VK::UNKWON) {
			auto moreKeys = processUnknownKeys(true);
			for (size_t i = 0; i < moreKeys.size(); i++) {
				currentInstance->KeyDownCallback(moreKeys[i]);
			}
		}
		currentInstance->KeyDownCallback(key);

		return 0;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		if (currentInstance->KeyUpCallback == nullptr)
			return 0;
		auto key = getVirtualKeyCode(wParam);

		if (key == VK::UNKWON) {
			auto moreKeys = processUnknownKeys(false);
			for (size_t i = 0; i < moreKeys.size(); i++) {
				currentInstance->KeyUpCallback(moreKeys[i]);
			}
		}
		currentInstance->KeyUpCallback(key);

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

void WindowHandler::Window::sendCloseRequest() {
	SendMessage(m_hwnd, WM_CLOSE, 0, 0);
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

void WindowHandler::Window::setKeyDownCallback(void(*fun)(VK)) {
	KeyDownCallback = fun;
}

void WindowHandler::Window::setKeyUpCallback(void(*fun)(VK)) {
	KeyUpCallback = fun;
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
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; // set the flags
	
	// Open the file dialog
	if (GetOpenFileName(&ofn) == TRUE) {
		return szFile;
	}
	return std::wstring();
}

std::wstring WindowHandler::Window::getSaveFileDialogBox(const wchar_t* filter) {
	// Initialize the OPENFILENAME structure
	TCHAR szFile[MAX_PATH] = { 0 };
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd; // set the parent window handle
	ofn.lpstrFilter = filter; // set the file filter
	ofn.lpstrFile = szFile; // set the file name buffer
	ofn.nMaxFile = MAX_PATH; // set the maximum file name length
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT; // set the flags

	// Open the file dialog
	if (GetSaveFileName(&ofn) == TRUE) {
		return szFile;
	}
	
	return std::wstring();
}

Rect2D<unsigned int> WindowHandler::Window::getClientSize() const {
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	return Rect2D<unsigned int>(rect);
}
