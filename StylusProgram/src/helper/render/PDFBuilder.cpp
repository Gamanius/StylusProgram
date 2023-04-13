#include "pdf/PDFHandler.h"
#include "util/Logger.h"

RenderHandler::PDFBuilder::PDFBuilder(Direct2DContext* context, PDFHandler::PDF* pdf) {
	m_pdf = pdf;
	m_rendercontext = context;

	auto pages = m_pdf->getNumberOfPages();
	m_bitmapbuffer.resize(pages);
	for (size_t i = 0; i < pages; i++) {
		if (i == 0) {
			m_bitmapbuffer[i] = new CachedPDFBitmap();
			m_bitmapbuffer[i]->m_positionandsize = m_pdf->getPageSize(i);
			m_bitmapbuffer[i]->m_scale = 1;
			continue;
		}
		auto prevbitmap = m_bitmapbuffer[i - 1];
		auto bitmap = new CachedPDFBitmap();
		bitmap->m_positionandsize = m_pdf->getPageSize(i);
		bitmap->m_positionandsize.upperleft.y = prevbitmap->m_positionandsize.upperleft.y + prevbitmap->m_positionandsize.height;

		m_bitmapbuffer[i] = bitmap;
	}

	calculateOutOfBoundsPDF();
}

RenderHandler::PDFBuilder::~PDFBuilder() {
	// there should be no ownership of the pdf or the context so we wont delete it here
	for (size_t i = 0; i < m_bitmapbuffer.size(); i++) {
		if (m_bitmapbuffer[i] == nullptr)
			continue;

		delete m_bitmapbuffer[i];
	}
}

void RenderHandler::PDFBuilder::calculateOutOfBoundsPDF() {
	// get the window size
	auto size = m_rendercontext->getDisplayViewport();
	// transform the viewport
	auto transformedrect = m_rendercontext->transformRectInv(Rect2D<float>(size));
	//transformedrect.upperleft *= -1;

	bool firstvisiblepage = false;
	// iterate over all pages
	for (size_t i = 0; i < m_bitmapbuffer.size(); i++) {
		auto pdfbitmap = m_bitmapbuffer[i];
		// calculate if the page is visible
		auto visible = transformedrect.intersects(pdfbitmap->m_positionandsize);

		if (visible && !firstvisiblepage) {
			m_startpagerender = i;
			firstvisiblepage = true;
		}
		else if ((!visible && firstvisiblepage) || (i == m_bitmapbuffer.size() - 1 && visible)) {
			m_endpagerender = i + visible;
			break;
		}
	}
}

void RenderHandler::PDFBuilder::renderBitmap() {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	// let the pdfs start at 0, 0
	for (size_t i = m_startpagerender; i < m_endpagerender; i++) {
		CachedPDFBitmap* bitmap = m_bitmapbuffer[i];
		Rect2D<float> transformedrect = m_rendercontext->scaleFromDisplayPixelsToViewPortPixels(bitmap->m_positionandsize);
		// render the pdf onto a bitmap
		if (bitmap->m_bitmap.m_bitmap == nullptr || !isEqual(bitmap->m_scale, m_rendercontext->getMatrixScaleOffset())) {
			bitmap->m_bitmap.~Bitmap();
			bitmap->m_bitmap = m_pdf->createBitmapFromPage(m_rendercontext, i, transformedrect, true, m_rendercontext->getDpi());
			bitmap->m_scale = m_rendercontext->getMatrixScaleOffset();
		}
	}
}

void RenderHandler::PDFBuilder::renderBitmap(size_t page) {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	CachedPDFBitmap* bitmap = m_bitmapbuffer[page];
	Rect2D<float> transformedrect = m_rendercontext->scaleFromDisplayPixelsToViewPortPixels(bitmap->m_positionandsize);
	// render the pdf onto a bitmap
	if (bitmap->m_bitmap.m_bitmap == nullptr || !isEqual(bitmap->m_scale, m_rendercontext->getMatrixScaleOffset())) {
		bitmap->m_bitmap.~Bitmap();
		bitmap->m_bitmap = m_pdf->createBitmapFromPage(m_rendercontext, page, transformedrect, true, m_rendercontext->getDpi());
		bitmap->m_scale = m_rendercontext->getMatrixScaleOffset();
	}
}

void RenderHandler::PDFBuilder::render() {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	Logger::add("red");
	m_rendercontext->beginDraw();

	// let the pdfs start at 0, 0
	m_rendercontext->setIdentityViewPortMatrixActive(); 
	for (size_t i = m_startpagerender; i < m_endpagerender; i++) {
		CachedPDFBitmap* bitmap = m_bitmapbuffer[i];

		if (bitmap->m_bitmap.m_bitmap == nullptr)
			renderBitmap(i);
		m_rendercontext->getRenderTarget()->DrawBitmap(bitmap->m_bitmap.m_bitmap, m_rendercontext->transformRect(bitmap->m_positionandsize), 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, {0, 0, bitmap->m_bitmap.m_bitmap->GetSize().width, bitmap->m_bitmap.m_bitmap->GetSize().height});
	}

	m_invalid = false;
	m_rendercontext->endDraw();
}

void RenderHandler::PDFBuilder::invalidate() {
	m_invalid = true;
}

bool RenderHandler::PDFBuilder::isInvalid() const {
	return m_invalid;
}
