#pragma once

#include <vector>
#include <string>
#include <d2d1.h>
#include <dwrite.h>

#include "util/Util.h"
#include "window/WindowHandler.h"
#include "pdf/PDFHandler.h"

#ifndef RENDER_HANDLER_H
#define RENDER_HANDLER_H

namespace PDFHandler {
	struct PDF;
}

namespace WindowHandler {
	class Window;
}

namespace RenderHandler {
	class StrokeBuilder;
	class Direct2DContext {
		WindowHandler::Window* m_mainWindow;
		static ID2D1Factory* m_pD2DFactory;

		IDWriteFactory* m_writeFactory = nullptr;
		IDWriteTextFormat* m_debugtextformat = nullptr;

		ID2D1HwndRenderTarget* m_hwndRendertarget = nullptr;
		ID2D1SolidColorBrush* m_debugBrush = nullptr;

		Point2D<float> m_matrixTranslationOffset = { 0, 0 };
		float m_matrixScaleOffset = 1;
		Point2D<float> m_matrixScaleOffsetCenter = { 0, 0 };
		Rect2D<unsigned int> m_displayViewSize;
		D2D1::Matrix3x2F m_scaleMatrix = D2D1::Matrix3x2F::Identity();
		D2D1::Matrix3x2F m_transformationMatrix = D2D1::Matrix3x2F::Identity();

		void (*RenderCallbackFunction)(ID2D1HwndRenderTarget* const);

		std::wstring m_debugString;
		size_t m_maxDebugStringSize = 2500;
		bool m_renderDebugInfo = true;

		// Is incremented everytime someone calls beginDraw and decremented if endDraw is called
		UINT32 m_isRenderInProgress = 0;

		void renderText();

		Direct2DContext(WindowHandler::Window* w);
		~Direct2DContext();
	public:

		void render();
		void resize(Rect2D<UINT> r);
		void clearCanvas();

		void beginDraw();
		void endDraw();

		void addDebugText(const std::wstring& s, bool forceDraw = false);

		// Will always call this callback function when render() is called.
		void addRepaintCallbackFunction(void (*fun)(ID2D1HwndRenderTarget* const));

		void setRenderDebugText(bool b);
		void setRenderDebugTextMaxSize(size_t size);

		void setMatrixTranslationOffset(Point2D<float> m);
		void setMatrixTranslationOffset(const D2D1::Matrix3x2F& mat);
		void addMatrixTranslationOffset(Point2D<float> m);
		void setMatrixScaleOffset(float f, Point2D<float> center);
		void setMatrixScaleOffset(const D2D1::Matrix3x2F& mat);
		void addMatrixScaleOffset(float f, Point2D<float> center);

		void setCurrentViewPortMatrixActive();
		void setIdentityViewPortMatrixActive();

		void resetMatrixOffsets();

		Point2D<float> getMatrixTranslationOffset() const;
		float getMatrixScaleOffset() const;

		float getDpi() const;

		Rect2D<unsigned int> getDisplayViewport() const;
		Point2D<float> scaleFromViewPortPixelsToDisplayPixels(Point2D<float> f) const;
		Rect2D<float> scaleFromViewPortPixelsToDisplayPixels(Rect2D<float> f) const;
		Point2D<float> scaleFromDisplayPixelsToViewPortPixels(Point2D<float> f) const;
		Rect2D<float> scaleFromDisplayPixelsToViewPortPixels(Rect2D<float> f) const;

		Point2D<float> transformPoint(Point2D<float> f) const;
		Rect2D<float> transformRect(Rect2D<float> f) const;

		// inverted transformation
		Point2D<float> transformPointInv(Point2D<float> f) const;
		Rect2D<float> transformRectInv(Rect2D<float> f) const;

		ID2D1Factory* getFactory() const;
		ID2D1HwndRenderTarget* getRenderTarget() const;
		D2D1::Matrix3x2F getScaleMatrix() const;
		ID2D1SolidColorBrush* getDebugBrush() const;

		friend WindowHandler::Window;
		friend StrokeBuilder;
	};

	struct Bitmap {
		const unsigned int PIXEL_SIZE = 8;
		ID2D1Bitmap* m_bitmap = nullptr;

		Bitmap() = default;
		Bitmap(Direct2DContext* context, Rect2D<unsigned int> size, float dpi = 96);
		Bitmap(Direct2DContext* context, const byte* const f, Rect2D<unsigned int> size, unsigned int stride, float dpi = 96);
		Bitmap(const Bitmap& f);
		Bitmap& operator=(const Bitmap& f);
		Bitmap(Bitmap&& f);
		Bitmap& operator=(Bitmap&& f);
		~Bitmap();
	};

	class StrokeBuilder {
		struct Stroke {
			// the points that define a stroke
			std::vector<Point2D<double>>* m_points;
			ID2D1PathGeometry* m_bezierCurveGeometry = nullptr;
			ID2D1Brush* m_strokeBrush = nullptr;
			ID2D1StrokeStyle* m_strokeStyle = nullptr;
			Rect2D<double> m_boundingBox;

			Stroke(Point2D<double> p);

			~Stroke();

			Stroke(const Stroke& s) = delete;
			Stroke& operator=(const Stroke&) = delete; // dont copy that array...or mess with reference count

			Stroke& operator=(Stroke&& s);
			Stroke(Stroke&& s);

			void setStrokeBrush(ID2D1Brush* brush);
			void setStrokeStyle(ID2D1StrokeStyle* style);
		};

		Direct2DContext* m_renderContext;
		std::map<UINT32, Stroke> m_dynamicStroke;
		std::vector<std::tuple<Point2D<double>, Point2D<double>>> m_dynamicLines;
		std::vector<Stroke> m_longTimeStroke;

		//Render Stuff
		ID2D1SolidColorBrush* m_currentInkBrush = nullptr;
		ID2D1StrokeStyle* m_currentLineStyle = nullptr;
		float m_currentStrokeWidht = 0.5;

	public:
		void startStroke(Point2D<double> p, UINT32 id);
		void addSafeStroke(Point2D<double> p, UINT32 id);
		void addStroke(Point2D<double> p, UINT32 id);
		void endStroke(Point2D<double> p, UINT32 id);

		Point2D<double> getLastStroke(UINT32 id);

		void renderAllStrokes();

		void renderDynamicLines();

		void setRenderContext(Direct2DContext* c);

		bool isStrokeInProgress() const;

		~StrokeBuilder();
	};

	class PDFBuilder {
		size_t padding = 10;
		Direct2DContext* m_rendercontext = nullptr;
		PDFHandler::PDF* m_pdf = nullptr;

		struct CachedPDFBitmap {
			float m_scale = 1;
			Rect2D<float> m_positionandsize;
			Rect2D<float> m_intersectionWithViewPort;
			RenderHandler::Bitmap m_bitmap;

			float m_previewscale = 0;
			RenderHandler::Bitmap m_previewbitmap;
		};
		std::vector<CachedPDFBitmap*> m_bitmapbuffer;

		size_t m_startpagerender = 0;
		size_t m_endpagerender = 1;

		size_t m_previewPages = 10;
		float m_previewScale = 0.5;

		size_t m_currentPage = 0;

		bool m_invalid = true;
	public:
		//constructor
		PDFBuilder(Direct2DContext* context, PDFHandler::PDF* pdf);
		~PDFBuilder();

		// will calculate the out of bounds pdf 
		void calculateOutOfBoundsPDF(); 
		// will render a lower resolution pdf onto a bitmap and save it in a buffer
		void createPreviewBitmaps(float scale = 0.5);
		// Will retrieve the current viewport and render the pdf onto a cached bitmap.
		void renderBitmap();
		// Will render a given page
		void renderBitmap(size_t page, bool viewportintersection = true);
		// Will render all visible pages
		void render();
		void renderpreview();

		size_t getCurrentPage() const;

		void invalidate();
		bool isInvalid() const;
	};

	class UIBuilder {

	};
}


#endif // !RENDER_HANDLER_H
