#pragma once

#include <Windows.h>
#include <map>
#include <string>
#include "render/RenderHandler.h"

#ifndef WINDOW_HANDLER_H
#define WINDOW_HANDLER_H

namespace RenderHandler {
	class Direct2DContext;
}

namespace WindowHandler {

	enum POINTER_TYPE {
		UNKNOWN,
		MOUSE,
		STYLUS,
		TOUCH
	};

	struct POINTER_INFO {
		UINT id = 0;
		POINTER_TYPE type = POINTER_TYPE::UNKNOWN;
		Point2D<float> pos = { 0, 0 };
		UINT32 pressure = 0;
		bool button1pressed = false; /* Left mouse button or barrel button */
		bool button2pressed = false; /* Right mouse button eareser button */
		bool button3pressed = false; /* Middle mouse button */
		bool button4pressed = false; /* X1 Button */
		bool button5pressed = false; /* X2 Button */
	};


	enum WINDOW_STATE {
		HIDDEN,
		MAXIMIZED,
		NORMAL,
		MINIMIZED
	};

	enum VK {
		LEFT_MB,
		RIGHT_MB,
		CANCEL,
		MIDDLE_MB,
		X1_MB,
		X2_MB,
		LEFT_SHIFT,
		RIGHT_SHIFT,
		LEFT_CONTROL,
		RIGHT_CONTROL,
		BACKSPACE,
		TAB,
		ENTER,
		ALT,
		PAUSE,
		CAPSLOCK,
		ESCAPE,
		SPACE,
		PAGE_UP,
		PAGE_DOWN,
		END,
		HOME,
		LEFTARROW,
		UPARROW,
		RIGHTARROW,
		DOWNARROW,
		SELECT,
		PRINT,
		EXECUTE,
		PRINT_SCREEN,
		INSERT,
		DEL,
		HELP,
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LEFT_WINDOWS,
		RIGHT_WINDOWS,
		APPLICATION,
		SLEEP,
		SCROLL_LOCK,
		LEFT_MENU,
		RIGHT_MENU,
		VOLUME_MUTE,
		VOLUME_DOWN,
		VOLUME_UP,
		MEDIA_NEXT,
		MEDIA_LAST,
		MEDIA_STOP,
		MEDIA_PLAY_PAUSE,
		OEM_1,
		OEM_2,
		OEM_3,
		OEM_4,
		OEM_5,
		OEM_6,
		OEM_7,
		OEM_8,
		OEM_102,
		OEM_CLEAR,
		OEM_PLUS,
		OEM_COMMA,
		OEM_MINUS,
		OEM_PERIOD,
		NUMPAD_0,
		NUMPAD_1,
		NUMPAD_2,
		NUMPAD_3,
		NUMPAD_4,
		NUMPAD_5,
		NUMPAD_6,
		NUMPAD_7,
		NUMPAD_8,
		NUMPAD_9,
		NUMPAD_MULTIPLY,
		NUMPAD_ADD,
		NUMPAD_SEPERATOR,
		NUMPAD_SUBTRACT,
		NUMPAD_COMMA,
		NUMPAD_DIVIDE,
		NUMPAD_LOCK,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		PLAY,
		ZOOM,
		UNKWON
	};

	class Window {
		HDC m_hdc;
		HWND m_hwnd;
		RenderHandler::Direct2DContext* m_renderContext;
		float m_dpi = 96;

		void (*PointerDownCallback)(POINTER_INFO) = nullptr;
		void (*PointerMoveCallback)(POINTER_INFO) = nullptr;
		void (*PointerUpCallback)(POINTER_INFO) = nullptr;

		void (*PointerScrollCallback)(SHORT, bool, Point2D<int>) = nullptr;

		void (*WindowPaintCallback)() = nullptr;

		void (*KeyDownCallback)(VK) = nullptr;
		void (*KeyUpCallback)(VK) = nullptr;

		/* TODO */
		void (*WindowResizeCallback)(WindowHandler::WINDOW_STATE, Rect2D<int>);
		void (*WindowCloseCallback)();
		void (*WindowMoveCallback)(Point2D<int>);

		static std::map<HWND, Window*> m_allWindowInstances;

		static WindowHandler::POINTER_INFO parsePointerInfo(Window* w, WPARAM wParam, LPARAM lParam);
		static LRESULT parseWindowMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	public:
		Window();
		~Window();

		bool init(std::wstring windowName, HINSTANCE instance);

		void setState(WindowHandler::WINDOW_STATE state);

		void getMsg(bool blocking = true);
		void forceDraw();

		bool getCloseRequest();
		void sendCloseRequest();

		void setPointerDownCallback(void (*fun)(POINTER_INFO));
		void setPointerUpdateCallback(void (*fun)(POINTER_INFO));
		void setPointerUpCallback(void (*fun)(POINTER_INFO));

		void setPointerScrollCallback(void (*fun)(SHORT, bool, Point2D<int>));

		void setKeyDownCallback(void (*fun)(VK));
		void setKeyUpCallback(void (*fun)(VK));


		void setWindowPaintCallback(void (*fun)());

		std::wstring getOpenFileDialogBox(const wchar_t* filter);
		std::wstring getSaveFileDialogBox(const wchar_t* filter);


		Rect2D<unsigned int> getClientSize() const;

		template <typename T>
		Point2D<T> PxToDp(Point2D<T> p) const {
			p.x = (float)p.x / (m_dpi / 96.0f);
			p.y = (float)p.y / (m_dpi / 96.0f);
			return p;
		}


		RenderHandler::Direct2DContext* getRenderContext();
		friend RenderHandler::Direct2DContext;
	};

	class TouchHandler {
		Point2D<double> m_initialOffset = { 0, 0 };
		D2D1::Matrix3x2F m_initialScaleMatrix;
		D2D1::Matrix3x2F m_initialScaleMatrixInv;
		float m_initialScale = 1;
		RenderHandler::Direct2DContext* m_renderContext;

		bool m_firstFingerActive;
		POINTER_INFO m_initialFirstFinger;
		POINTER_INFO m_lastFirstFinger;
		bool m_secondFingerActive;
		POINTER_INFO m_initialSecondFinger;
		POINTER_INFO m_lastSecondFinger;
	public:
		TouchHandler(RenderHandler::Direct2DContext* context);

		void startTouchGesture(POINTER_INFO fingerPosition);

		void updateTouchGesture(POINTER_INFO fingerPosition);

		void stopTouchGestureOfFinger(POINTER_INFO finger);

		bool isGestureInProgress() const;
		bool isZoomGestureInProgress() const;
		bool isPanGestureInProgress() const;
	};

}

#endif // !WINDOW_HANDLER_H
