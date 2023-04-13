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

		void setPointerDownCallback(void (*fun)(POINTER_INFO));
		void setPointerUpdateCallback(void (*fun)(POINTER_INFO));
		void setPointerUpCallback(void (*fun)(POINTER_INFO));

		void setPointerScrollCallback(void (*fun)(SHORT, bool, Point2D<int>));

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
