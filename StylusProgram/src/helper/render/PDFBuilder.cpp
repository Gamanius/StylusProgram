#include "pdf/PDFHandler.h"
#include "util/Logger.h"

RenderHandler::PDFBuilder::PDFBuilder(Direct2DContext* context, PDFHandler::PDF* pdf) {
	m_pdf = pdf;
	m_rendercontext = context;

	// resize the buffer to the number of pages
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
		bitmap->m_positionandsize.upperleft.y = prevbitmap->m_positionandsize.upperleft.y + prevbitmap->m_positionandsize.height + padding;

		m_bitmapbuffer[i] = bitmap;
	}

	calculateOutOfBoundsPDF();
	createPreviewBitmaps(m_previewScale);
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
		auto& pdfbitmap = m_bitmapbuffer[i];
		// calculate if the page is visible
		auto visible = transformedrect.intersects(pdfbitmap->m_positionandsize);

		if (visible) {
			pdfbitmap->m_intersectionWithViewPort = transformedrect.intersection(pdfbitmap->m_positionandsize);
		}
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

void RenderHandler::PDFBuilder::createPreviewBitmaps(float scale) {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	size_t startIndex = 0;
	if (m_previewPages < m_startpagerender)
		startIndex = m_startpagerender - m_previewPages;
	size_t endIndex = m_endpagerender + m_previewPages;

	for (size_t i = 0; i < m_pdf->getNumberOfPages(); i++) {
		auto& bitmap = m_bitmapbuffer[i];
		// remove the previous bitmaps for memory reasons
		if (i < startIndex || i > endIndex) {
			bitmap->m_previewscale = 0;
			bitmap->m_previewbitmap.~Bitmap(); 
			continue;
		}

		if (isEqual(scale, bitmap->m_previewscale))
			continue;

		bitmap->m_previewbitmap = m_pdf->createBitmapFromPage(m_rendercontext, i, bitmap->m_positionandsize, m_rendercontext->getDpi() * scale);
		bitmap->m_previewscale = scale;
	}
}

void RenderHandler::PDFBuilder::renderBitmap() {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	// let the pdfs start at 0, 0
	for (size_t i = m_startpagerender; i < m_endpagerender; i++) {
		renderBitmap(i);
	}
}

void RenderHandler::PDFBuilder::renderBitmap(size_t page, bool viewportintersection) {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	// dont render the page if the current scale is smaller then the preview scale
	if (m_rendercontext->getMatrixScaleOffset() < m_previewScale)
		return;

	CachedPDFBitmap* bitmap = m_bitmapbuffer[page];
	Rect2D<float> transformedrect = m_rendercontext->scaleFromDisplayPixelsToViewPortPixels(bitmap->m_positionandsize);
	// render the pdf onto a bitmap
	//if (bitmap->m_bitmap.m_bitmap == nullptr || !isEqual(bitmap->m_scale, m_rendercontext->getMatrixScaleOffset())) {
	bitmap->m_bitmap.~Bitmap();
	if (viewportintersection)
		bitmap->m_bitmap = m_pdf->createBitmapFromPage(m_rendercontext, page, m_rendercontext->scaleFromDisplayPixelsToViewPortPixels(bitmap->m_intersectionWithViewPort), { bitmap->m_intersectionWithViewPort.upperleft - bitmap->m_positionandsize.upperleft, bitmap->m_intersectionWithViewPort.width, bitmap->m_intersectionWithViewPort.height }, m_rendercontext->getDpi());
	else
		bitmap->m_bitmap = m_pdf->createBitmapFromPage(m_rendercontext, page, transformedrect, m_rendercontext->getDpi());
	bitmap->m_scale = m_rendercontext->getMatrixScaleOffset();
	//}
}


void RenderHandler::PDFBuilder::render() {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;
	
	m_rendercontext->beginDraw();

	// let the pdfs start at 0, 0
	m_rendercontext->setCurrentViewPortMatrixActive(); 
	for (size_t i = m_startpagerender; i < m_endpagerender; i++) {
		CachedPDFBitmap* pdf = m_bitmapbuffer[i];
		auto& bitmap = pdf->m_bitmap;
		auto& prevbitmap = pdf->m_previewbitmap;

		if (bitmap.m_bitmap == nullptr)
			renderBitmap(i);
		if (prevbitmap.m_bitmap == nullptr)
			createPreviewBitmaps();

		m_rendercontext->getRenderTarget()->DrawBitmap(prevbitmap.m_bitmap, pdf->m_positionandsize, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, {0, 0, prevbitmap.m_bitmap->GetSize().width, prevbitmap.m_bitmap->GetSize().height});
		if (m_rendercontext->getMatrixScaleOffset() > m_previewScale)
			m_rendercontext->getRenderTarget()->DrawBitmap(bitmap.m_bitmap, pdf->m_intersectionWithViewPort, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, {0, 0, bitmap.m_bitmap->GetSize().width, bitmap.m_bitmap->GetSize().height});
	}

	m_invalid = false;
	m_rendercontext->endDraw();
}

void RenderHandler::PDFBuilder::renderpreview() {
	if (m_pdf == nullptr || m_rendercontext == nullptr)
		return;

	m_rendercontext->beginDraw();

	// let the pdfs start at 0, 0
	m_rendercontext->setCurrentViewPortMatrixActive();
	for (size_t i = m_startpagerender; i < m_endpagerender; i++) {
		CachedPDFBitmap* pdf = m_bitmapbuffer[i];
		auto& bitmap = pdf->m_bitmap;
		auto& prevbitmap = pdf->m_previewbitmap;

		if (prevbitmap.m_bitmap == nullptr)
			createPreviewBitmaps();

		m_rendercontext->getRenderTarget()->DrawBitmap(prevbitmap.m_bitmap, pdf->m_positionandsize, 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, { 0, 0, prevbitmap.m_bitmap->GetSize().width, prevbitmap.m_bitmap->GetSize().height });
	}

	m_invalid = false;
	m_rendercontext->endDraw();
}

size_t RenderHandler::PDFBuilder::getCurrentPage() const {
	return size_t();
}

Rect2D<float> RenderHandler::PDFBuilder::getSizeAndPositionOfPage(size_t page) const {
	return m_bitmapbuffer[page]->m_positionandsize;
}

void RenderHandler::PDFBuilder::invalidate() {
	m_invalid = true;
}

bool RenderHandler::PDFBuilder::isInvalid() const {
	return m_invalid;
}

std::tuple<size_t, size_t> RenderHandler::PDFBuilder::getVisibleStartAndEndPage() const {
	return std::tuple<size_t, size_t>(m_startpagerender, m_endpagerender);
}
