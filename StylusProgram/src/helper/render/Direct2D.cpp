#include "RenderHandler.h"
#include "util/Logger.h"
#include <d2d1_1.h>

ID2D1Factory* RenderHandler::Direct2DContext::m_pD2DFactory;

RenderHandler::Direct2DContext::Direct2DContext(WindowHandler::Window* w) {
	if (w == nullptr) {
		Logger::err(L"An Unknown Error has occured");
		return;
	}
	m_mainWindow = w;
	D2D1_FACTORY_OPTIONS option;

#ifdef _DEBUG
	option.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	m_renderDebugInfo = true;
#else
	option.debugLevel = D2D1_DEBUG_LEVEL_NONE;
	m_renderDebugInfo = false;
#endif // _DEBUG

	// only create a factory if there is none
	HRESULT hr = S_OK;
	if (m_pD2DFactory == nullptr)
		//hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory), nullptr, (void**) & m_pD2DFactory);
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, option, &m_pD2DFactory);
	else
		m_pD2DFactory->AddRef();
	
	if (hr != S_OK) {
		Logger::err(L"Couldn't create D2D1 Factory");
		return;
	}

	// create HWND render target
	auto clientRect = m_mainWindow->getClientSize();
	hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_mainWindow->m_hwnd, D2D1::SizeU(clientRect.width, clientRect.height)), &m_hwndRendertarget);
	if (hr != S_OK) {
		Logger::err(L"Could not create a HWND render target");
		return;
	}

	hr= m_hwndRendertarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &m_debugBrush);
	if (hr!= S_OK) {
		Logger::err(L"Could not create a solid colour brush");
		return;
	}

	// create some text rendering
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_writeFactory);
	if (hr != S_OK) {
		Logger::err(L"Couldn't create write factory");
		return;
	}
	
	wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
	GetSystemDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);
	hr = m_writeFactory->CreateTextFormat(L"Consolas", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20, localeName, &m_debugtextformat);
	if (hr != S_OK) {
		Logger::err(L"Couldn't craete text format");
		return;
	}

	m_displayViewSize = clientRect;
}



RenderHandler::Direct2DContext::~Direct2DContext() {
	SafeRelease(&m_debugBrush);
	SafeRelease(&m_pD2DFactory);
	SafeRelease(&m_hwndRendertarget);
	SafeRelease(&m_writeFactory);
}

void RenderHandler::Direct2DContext::render() {
	if (m_hwndRendertarget == nullptr)
		return;

	beginDraw();
		
	if (RenderCallbackFunction != nullptr)
		RenderCallbackFunction(this->m_hwndRendertarget);

	// always render debug info at the end
	
	m_hwndRendertarget->SetTransform(D2D1::Matrix3x2F::Identity());
		
	if (m_renderDebugInfo)
		renderText();

	endDraw();
}

ID2D1Factory* RenderHandler::Direct2DContext::getFactory() const {
	return m_pD2DFactory;
}

ID2D1HwndRenderTarget* RenderHandler::Direct2DContext::getRenderTarget() const {
	return m_hwndRendertarget;
}

D2D1::Matrix3x2F RenderHandler::Direct2DContext::getScaleMatrix() const {
	return m_scaleMatrix;
}

ID2D1SolidColorBrush* RenderHandler::Direct2DContext::getDebugBrush() const {
	return m_debugBrush;
}

void RenderHandler::Direct2DContext::resize(Rect2D<UINT> r) {
	if (m_hwndRendertarget == nullptr)
		return;

	D2D1_SIZE_U d;
	d.height = r.height;
	d.width = r.width;
	m_hwndRendertarget->Resize(d);
}

void RenderHandler::Direct2DContext::clearCanvas() {
	beginDraw();
	m_hwndRendertarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
	endDraw();
}

void RenderHandler::Direct2DContext::beginDraw() {
	if (m_hwndRendertarget == nullptr)
		return;
	if (m_isRenderInProgress == 0)
		m_hwndRendertarget->BeginDraw();
	m_isRenderInProgress++;
}

void RenderHandler::Direct2DContext::endDraw() {
	if (m_hwndRendertarget == nullptr)
		return;
	m_isRenderInProgress--;
	if (m_isRenderInProgress == 0)
		m_hwndRendertarget->EndDraw();
}

void RenderHandler::Direct2DContext::renderText() {
	if (m_writeFactory == nullptr)
		return;

	beginDraw();
	IDWriteTextLayout* textLayout;
	auto r = m_hwndRendertarget->GetPixelSize();
	m_writeFactory->CreateTextLayout(m_debugString.c_str(), m_debugString.size(), m_debugtextformat, (float)r.width, (float)r.height, &textLayout);
	if (textLayout == nullptr)
		return;
	m_hwndRendertarget->DrawTextLayout(D2D1::Point2F(), textLayout, m_debugBrush);
	textLayout->Release();
	endDraw();
}

void RenderHandler::Direct2DContext::setRenderDebugText(bool b) {
	m_renderDebugInfo = true;
}

void RenderHandler::Direct2DContext::setRenderDebugTextMaxSize(size_t size) {
	m_maxDebugStringSize = size;
}

void RenderHandler::Direct2DContext::setMatrixTranslationOffset(Point2D<float> m) {
	m_matrixTranslationOffset = m;

	m_transformationMatrix = D2D1::Matrix3x2F::Translation(m);
}

void RenderHandler::Direct2DContext::addMatrixTranslationOffset(Point2D<float> m) {
	m_matrixTranslationOffset += m;

	m_transformationMatrix = m_transformationMatrix * D2D1::Matrix3x2F::Translation(m);
}

void RenderHandler::Direct2DContext::setMatrixScaleOffset(float f, Point2D<float> center) {
	m_matrixScaleOffset = f;
	m_matrixScaleOffsetCenter = center;

	// calc the new scale matrix
	m_scaleMatrix = D2D1::Matrix3x2F::Scale({ f, f }, center);
}

void RenderHandler::Direct2DContext::addMatrixScaleOffset(float f, Point2D<float> center) {
	m_matrixScaleOffset += f;
	m_matrixScaleOffsetCenter = center;
	
	// calc the new scale matrix
	auto mat = D2D1::Matrix3x2F(m_scaleMatrix);
	mat.Invert();
	mat = D2D1::Matrix3x2F::Scale({ 1 + f, 1 + f }, mat.TransformPoint(center));
	m_scaleMatrix = mat * m_scaleMatrix;
}

void RenderHandler::Direct2DContext::setCurrentViewPortMatrixActive() {
	m_hwndRendertarget->SetTransform(m_transformationMatrix * m_scaleMatrix);
}

void RenderHandler::Direct2DContext::setIdentityViewPortMatrixActive() {
	m_hwndRendertarget->SetTransform(D2D1::Matrix3x2F::Identity());
}


void RenderHandler::Direct2DContext::resetMatrixOffsets() {
	m_matrixScaleOffset = 1;
	m_matrixTranslationOffset = { 0 , 0 };

	m_scaleMatrix = D2D1::Matrix3x2F::Identity();
	m_transformationMatrix = D2D1::Matrix3x2F::Identity();

}

Point2D<float> RenderHandler::Direct2DContext::getMatrixTranslationOffset() const {
	return m_matrixTranslationOffset;
}

float RenderHandler::Direct2DContext::getMatrixScaleOffset() const {
	return m_matrixScaleOffset;
}

float RenderHandler::Direct2DContext::getDpi() const {
	return GetDpiForWindow(m_hwndRendertarget->GetHwnd());
}

Rect2D<long> RenderHandler::Direct2DContext::getDisplayViewport() const {
	return m_displayViewSize;
}

Point2D<float> RenderHandler::Direct2DContext::scaleFromViewPortPixelsToDisplayPixels(Point2D<float> f) const {
	D2D1::Matrix3x2F invertedMatrix(m_scaleMatrix);
	invertedMatrix.Invert();
	auto d = invertedMatrix.TransformPoint(f);
	return { d.x, d.y };
}

Rect2D<float> RenderHandler::Direct2DContext::scaleFromViewPortPixelsToDisplayPixels(Rect2D<float> f) const {
	D2D1::Matrix3x2F invertedMatrix(m_scaleMatrix); 
	invertedMatrix.Invert(); 
	auto d = invertedMatrix.TransformPoint(f.upperleft);
	auto d2 = invertedMatrix.TransformPoint({f.upperleft.x + f.width, f.upperleft.y + f.height});
	return { {d.x, d.y}, d2.x - d.x, d2.y - d.y};
}

Point2D<float> RenderHandler::Direct2DContext::scaleFromDisplayPixelsToViewPortPixels(Point2D<float> f) const {
	auto d = m_scaleMatrix.TransformPoint(f);
	return { d.x, d.y };
}

Rect2D<float> RenderHandler::Direct2DContext::scaleFromDisplayPixelsToViewPortPixels(Rect2D<float> f) const {
	auto d = m_scaleMatrix.TransformPoint(f.upperleft);
	auto d2 = m_scaleMatrix.TransformPoint({ f.upperleft.x + f.width, f.upperleft.y + f.height });
	return { {d.x, d.y}, d2.x - d.x, d2.y - d.y };
}

Point2D<float> RenderHandler::Direct2DContext::transformPoint(Point2D<float> f) const {
	auto mat = m_transformationMatrix * m_scaleMatrix;
	return mat.TransformPoint(f);
}

Rect2D<float> RenderHandler::Direct2DContext::transformRect(Rect2D<float> f) const {
	auto mat = m_transformationMatrix * m_scaleMatrix;
	auto d = mat.TransformPoint(f.upperleft);
	auto d2 = mat.TransformPoint({ f.upperleft.x + f.width, f.upperleft.y + f.height });
	return { {d.x, d.y}, d2.x - d.x, d2.y - d.y };
}

Point2D<float> RenderHandler::Direct2DContext::transformPointInv(Point2D<float> f) const {
	D2D1::Matrix3x2F invertedMatrix(m_transformationMatrix * m_scaleMatrix);
	invertedMatrix.Invert();
	return invertedMatrix.TransformPoint(f);
}

Rect2D<float> RenderHandler::Direct2DContext::transformRectInv(Rect2D<float> f) const {
	D2D1::Matrix3x2F invertedMatrix(m_transformationMatrix * m_scaleMatrix);
	invertedMatrix.Invert();
	auto d = invertedMatrix.TransformPoint(f.upperleft);
	auto d2 = invertedMatrix.TransformPoint({ f.upperleft.x + f.width, f.upperleft.y + f.height });
	return { {d.x, d.y}, d2.x - d.x, d2.y - d.y };
}



void RenderHandler::Direct2DContext::addDebugText(const std::wstring& s, bool forceDraw) {
	m_debugString.insert(0, s);
	m_debugString.insert(s.size(), L"\n");

	if (m_debugString.size() > m_maxDebugStringSize) {
		m_debugString.erase(m_maxDebugStringSize, m_debugString.npos);
	}

	InvalidateRect(m_hwndRendertarget->GetHwnd(), nullptr, false);
	UpdateWindow(m_hwndRendertarget->GetHwnd());
}

void RenderHandler::Direct2DContext::addRepaintCallbackFunction(void (*fun)(ID2D1HwndRenderTarget* const)) {
	RenderCallbackFunction = fun;
}

RenderHandler::Bitmap::Bitmap(Direct2DContext* context, Rect2D<unsigned int> size, float dpi) {
	D2D1_BITMAP_PROPERTIES prop;
	prop.dpiX = dpi;
	prop.dpiY = dpi;
	prop.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	prop.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	context->getRenderTarget()->CreateBitmap({size.width, size.height}, prop, &m_bitmap);
}

RenderHandler::Bitmap::Bitmap(Direct2DContext* context, const byte* const f, Rect2D<unsigned int> size, unsigned int stride, float dpi) : Bitmap(context, size, dpi) {
	D2D1_RECT_U rect = { 0, 0, size.width, size.height };
	m_bitmap->CopyFromMemory(&rect, f, stride);
}


RenderHandler::Bitmap::Bitmap(const Bitmap& f) {
	f.m_bitmap->AddRef();
	m_bitmap = f.m_bitmap;
}

RenderHandler::Bitmap& RenderHandler::Bitmap::operator=(const Bitmap& f) {
	f.m_bitmap->AddRef();
	m_bitmap = f.m_bitmap;
	return *this;
}

RenderHandler::Bitmap::Bitmap(Bitmap&& f) {
	m_bitmap = f.m_bitmap;
	f.m_bitmap = nullptr;
}

RenderHandler::Bitmap& RenderHandler::Bitmap::operator=(Bitmap&& f) {
	m_bitmap = f.m_bitmap;
	f.m_bitmap = nullptr;

	return *this;
}

RenderHandler::Bitmap::~Bitmap() {
	if (m_bitmap == nullptr)
		return;

	m_bitmap->Release();
}