#pragma once
#include "helper/include.h"

WindowHandler::Window* _mainWindow = nullptr;
RenderHandler::Direct2DContext* context = nullptr;
RenderHandler::StrokeBuilder* builder = nullptr;
PDFHandler::MUPDF* pdfhandler = nullptr;
WindowHandler::TouchHandler* touchHandler = nullptr;
PDFHandler::PDF pdf;
RenderHandler::PDFBuilder* pdfbuilder = nullptr;


ID2D1Bitmap* bitmap;

void PenDown(WindowHandler::POINTER_INFO state) {
	if (state.type == WindowHandler::TOUCH)
		touchHandler->startTouchGesture(state);
	if (state.type == WindowHandler::MOUSE || state.type == WindowHandler::STYLUS)
		builder->startStroke(context->scaleFromViewPortPixelsToDisplayPixels(state.pos), state.id);
}

void PointerMove(WindowHandler::POINTER_INFO state) {
	//Logger::add(builder->getLastStroke(state.id));
	//if (context->scaleFromDisplayPixelsToViewPortPixels(builder->getLastStroke(state.id) + context->getMatrixTranslationOffset()).distance(state.pos) < 3)
	//	return;
	if (state.type == WindowHandler::TOUCH) {
		touchHandler->updateTouchGesture(state);

		context->render();
	}

	if (state.type == WindowHandler::MOUSE || state.type == WindowHandler::STYLUS)
		builder->addSafeStroke(context->scaleFromViewPortPixelsToDisplayPixels(state.pos), state.id);
}

void PointerUp(WindowHandler::POINTER_INFO state) {
	if (state.type == WindowHandler::MOUSE || state.type == WindowHandler::STYLUS)
		builder->endStroke(context->scaleFromViewPortPixelsToDisplayPixels(state.pos), state.id);
	if (state.type == WindowHandler::TOUCH) {
		touchHandler->stopTouchGestureOfFinger(state);

		pdfbuilder->renderBitmap();

		pdfbuilder->render();
	}
}

void PointerScroll(SHORT delta, bool hwehl, Point2D<int> p) {
	if (builder->isStrokeInProgress())
		return;
	if (hwehl)
		context->addMatrixTranslationOffset({ (float)delta * (1 / context->getMatrixScaleOffset()) , 0.0 });
	else {
		//check if ctrl is pressed
		bool isCTRLPressed = GetKeyState(VK_CONTROL) & 0x8000;
		if (!isCTRLPressed) {
			context->addMatrixTranslationOffset({ 0.0 , (float)delta * (1 / context->getMatrixScaleOffset()) });
		}
		else {
			if (context->getMatrixScaleOffset() + ((float)delta) / 1000.0 < 0.1)
				return;
			context->addMatrixScaleOffset(((float)delta)  / 1000.0, p);
			
		}
	}
	context->render();
	//Logger::add(context->getDisplayViewport());
}

void WindowRepaint(ID2D1HwndRenderTarget* const h) {
	context->clearCanvas();
	if (pdfbuilder != nullptr ) {
		pdfbuilder->calculateOutOfBoundsPDF(); 
		if (!touchHandler->isZoomGestureInProgress()) {
			pdfbuilder->renderBitmap();
		}
	//	if (pdfbuilder->isInvalid())
		pdfbuilder->render();
	}
	builder->renderAllStrokes();
}

bool init(HINSTANCE& hInstance) {
	//AllocConsole();
	//FILE* pCout;
	//freopen_s(&pCout, "CONOUT$", "w", stdout);

	Logger::init(Logger::PRINT_TARGET::STD_OUT);


	_mainWindow = new WindowHandler::Window();
	if (_mainWindow->init(L"ReeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeealTimeStylus API Teset", hInstance) == false)
		return false;

	_mainWindow->setPointerDownCallback(PenDown);
	_mainWindow->setPointerUpdateCallback(PointerMove);
	_mainWindow->setPointerUpCallback(PointerUp);
	_mainWindow->setPointerScrollCallback(PointerScroll);
	context = _mainWindow->getRenderContext();

	builder = new RenderHandler::StrokeBuilder();
	builder->setRenderContext(context);

	context->addRepaintCallbackFunction(WindowRepaint);

	pdfhandler = new PDFHandler::MUPDF();

	touchHandler = new WindowHandler::TouchHandler(context);

	Logger::setDirect2DContext(_mainWindow->getRenderContext());

	return true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	auto s = TimeSince1970();
	if (init(hInstance) == false)
		return -1;
	s = TimeSince1970() - s;

	Logger::add(L"Everything initialized in " + std::to_wstring(s) + L"ms");

	auto filepath = _mainWindow->getOpenFileDialogBox(L"PDF Files (*.pdf)\0*.pdf\0\0");
	if (!filepath.empty()) {
		pdf = pdfhandler->loadPDF(filepath);
		pdfbuilder = new RenderHandler::PDFBuilder(context, &pdf);
	}
	
	context->render();
	s = TimeSince1970();
	while (!_mainWindow->getCloseRequest()) {
		_mainWindow->getMsg();
		if (TimeSince1970() - s > 30) {
			//Logger::add(L"test");
			s = TimeSince1970();
			builder->renderDynamicLines();
		}
	}

	delete _mainWindow;
	delete builder;
	delete touchHandler;
	delete pdfbuilder;

	return 0;
}