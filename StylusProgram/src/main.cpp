#pragma once
#include "helper/include.h"
#include "mupdf/pdf.h"
#include <filesystem>

WindowHandler::Window* _mainWindow = nullptr;
WindowHandler::TouchHandler* touchHandler = nullptr;
RenderHandler::Direct2DContext* context = nullptr;

PDFHandler::AnnotationHandler* annothandler = nullptr;
PDFHandler::MUPDF* pdfhandler = nullptr;
PDFHandler::PDF pdf;

RenderHandler::PDFBuilder* pdfbuilder = nullptr;
RenderHandler::StrokeBuilder* builder = nullptr;

unsigned long long lastScrollTime = 0;

bool isCtrlPressed = false;
bool isAltPressed = false;

void LoadPdf() {
	delete pdfbuilder;
	delete annothandler;
	builder = nullptr;
	pdf.~PDF();

	context->resetMatrixOffsets(); 

	auto filepath = _mainWindow->getOpenFileDialogBox(L"PDF Files (*.pdf)\0*.pdf\0\0");
	if (!filepath.empty()) {
		pdf = pdfhandler->loadPDF(filepath);
		pdfbuilder = new RenderHandler::PDFBuilder(context, &pdf);
		annothandler = new PDFHandler::AnnotationHandler(&pdf, pdfbuilder);
		builder = annothandler->getStrokeBuilder();
	}

	context->render();
}

void SavePdf() {
	if (annothandler == nullptr)
		return;

	auto filepath = _mainWindow->getSaveFileDialogBox(L"PDF Files (*.pdf)\0*.pdf\0\0");
	std::filesystem::path p(filepath);
	p.replace_extension(".pdf");
	filepath = std::wstring(p.c_str());

	annothandler->bakeAnnotations();
	pdf.save(filepath);
	
}

void PenDown(WindowHandler::POINTER_INFO state) {
	if (annothandler == nullptr)
		return;

	if (state.type == WindowHandler::TOUCH)
		touchHandler->startTouchGesture(state);
	if (state.type == WindowHandler::MOUSE) {
		if (state.button1pressed)
			annothandler->startStroke(context->transformPointInv(state.pos), state.id);
		else if (state.button2pressed)
			annothandler->eraser(context->transformPointInv(state.pos));
	}
	if (state.type == WindowHandler::STYLUS) {
		if (state.button2pressed)
			annothandler->eraser(context->transformPointInv(state.pos));
		else
			annothandler->startStroke(context->transformPointInv(state.pos), state.id);
	}
}

void PointerMove(WindowHandler::POINTER_INFO state) {
	if (annothandler == nullptr)
		return;

	if (state.type == WindowHandler::TOUCH) {
		touchHandler->updateTouchGesture(state);
		context->render();
	}

	if (state.type == WindowHandler::MOUSE) {
		if (state.button1pressed)
			annothandler->addStroke(context->transformPointInv(state.pos), state.id);
		else if (state.button2pressed)
			annothandler->eraser(context->transformPointInv(state.pos));
	}
	if (state.type == WindowHandler::STYLUS) {
		if (state.button2pressed)
			annothandler->eraser(context->transformPointInv(state.pos));
		else
			annothandler->addStroke(context->transformPointInv(state.pos), state.id);
	}
}

void PointerUp(WindowHandler::POINTER_INFO state) {
	if (annothandler == nullptr)
		return;

	if (state.type == WindowHandler::MOUSE || state.type == WindowHandler::STYLUS) {
		annothandler->endStroke(context->transformPointInv(state.pos), state.id);
	}
	if (state.type == WindowHandler::TOUCH) {
		touchHandler->stopTouchGestureOfFinger(state);

		if (pdfbuilder == nullptr)
			return;
		
		context->render();
	}
}

void PointerScroll(SHORT delta, bool hwehl, Point2D<int> p) {
	if (annothandler == nullptr)
		return;

	if (annothandler->isStrokeinProgress())
		return;
	if (hwehl)
		context->addMatrixTranslationOffset({ (float)delta * (1 / context->getMatrixScaleOffset()) , 0.0 });
	else {
		//check if ctrl is pressed
		//bool isCtrlPressed = GetKeyState(VK_CONTROL) & 0x8000;
		if (!isCtrlPressed) { 
			context->addMatrixTranslationOffset({ 0.0 , (float)delta * (1 / context->getMatrixScaleOffset()) });
		}
		else {
			if (context->getMatrixScaleOffset() + ((float)delta) / 1000.0 < 0.1)
				return;
			context->addMatrixScaleOffset(((float)delta)  / 1000.0, p);
			
		}
	}

	if (pdfbuilder != nullptr) {
		pdfbuilder->calculateOutOfBoundsPDF();
		pdfbuilder->createPreviewBitmaps();
	}

	context->render();
}


void WindowRepaint(ID2D1HwndRenderTarget* const h) {
	if (pdfbuilder == nullptr)
		return;

	context->clearCanvas();

	pdfbuilder->calculateOutOfBoundsPDF(); 
	pdfbuilder->renderpreview(); 

	if (!touchHandler->isGestureInProgress()) {
		auto time = TimeSince1970();
		pdfbuilder->renderBitmap(); 
		Logger::add("Rendered pdf to bitmap in " + std::to_string(TimeSince1970() - time));
		pdfbuilder->render();
	}

	builder->renderAllStrokes(); 
}

void KeyDown(WindowHandler::VK key) {
	switch (key) {
	case WindowHandler::VK::LEFT_CONTROL:
		isCtrlPressed = true;
		break;
	case WindowHandler::VK::ALT:
		isAltPressed = true;
		break;
	}
}

void KeyUp(WindowHandler::VK key) {
	switch (key) {
	case WindowHandler::VK::LEFT_CONTROL:
		isCtrlPressed = false;
		break;
	case WindowHandler::VK::ALT:
		isAltPressed = false;
		break;
	case WindowHandler::VK::F4:
	{
		if (isAltPressed) {
			_mainWindow->sendCloseRequest();
		}
	}
	case WindowHandler::VK::O:
	{
		if (isCtrlPressed) {
			LoadPdf();
		}
	}
	case WindowHandler::VK::S:
	{
		if (isCtrlPressed) {
			SavePdf();
		}
	}
	}
}

bool init(HINSTANCE& hInstance) {
#ifdef _DEBUG
	AllocConsole();
	FILE* pCout;
	freopen_s(&pCout, "CONOUT$", "w", stdout);
#endif

	Logger::init(Logger::PRINT_TARGET::STD_OUT);


	_mainWindow = new WindowHandler::Window();
	if (_mainWindow->init(L"ReeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeealTimeStylus API Teset", hInstance) == false)
		return false;

	_mainWindow->setPointerDownCallback(PenDown);
	_mainWindow->setPointerUpdateCallback(PointerMove);
	_mainWindow->setPointerUpCallback(PointerUp);
	_mainWindow->setPointerScrollCallback(PointerScroll);

	_mainWindow->setKeyDownCallback(KeyDown);
	_mainWindow->setKeyUpCallback(KeyUp);
	context = _mainWindow->getRenderContext();

	context->addRepaintCallbackFunction(WindowRepaint);

	pdfhandler = new PDFHandler::MUPDF();

	touchHandler = new WindowHandler::TouchHandler(context);
	
	Logger::setDirect2DContext(_mainWindow->getRenderContext());

	return true;
}

// put this command into the pre built event thingy and then everything should programagically work :P
// msbuild.exe $(SolutionDir)mupdf\platform\win32\mupdf.sln -p:Configuration=$(ConfigurationName);Platform=$(PlatformName)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	auto s = TimeSince1970();
	if (init(hInstance) == false)
		return -1;
	s = TimeSince1970() - s;

	Logger::add(L"Everything initialized in " + std::to_wstring(s) + L"ms");

	LoadPdf();

	// main loop
	context->render();
	s = TimeSince1970();
	while (!_mainWindow->getCloseRequest()) {
		_mainWindow->getMsg();
		if (builder != nullptr && TimeSince1970() - s > 30) {
			//Logger::add(L"test");
			s = TimeSince1970();
			builder->renderDynamicLines();
		}
	}

	// clean up
	delete _mainWindow;
	delete touchHandler;
	delete pdfbuilder;
	delete annothandler;
	pdf.~PDF();
	pdf = PDFHandler::PDF();
	delete pdfhandler;

	return 0;
}