#include "PDFHandler.h"
#include "mupdf/pdf.h"

PDFHandler::PDF::PDF(MUPDF* context, fz_document* doc) {
	m_doc = doc;
	m_pdfcontext = context;
}

PDFHandler::PDF::PDF(const PDF& f) : File(f) {
	m_doc = f.m_doc;
	m_doc->refs++;

	m_pdfcontext = f.m_pdfcontext;
}

PDFHandler::PDF& PDFHandler::PDF::operator=(const PDF& f) {
	m_doc = f.m_doc;
	m_doc->refs++;

	m_pdfcontext = f.m_pdfcontext;

	File::operator=(f);

	return *this;
}

PDFHandler::PDF::PDF(PDF&& f) : File(std::move(f)) {
	m_doc = f.m_doc;
	f.m_doc = nullptr;

	m_pdfcontext = f.m_pdfcontext;
	f.m_pdfcontext = nullptr;
}

PDFHandler::PDF& PDFHandler::PDF::operator=(PDF&& f) {
	m_doc = f.m_doc;
	f.m_doc = nullptr;

	m_pdfcontext = f.m_pdfcontext;
	f.m_pdfcontext = nullptr;

	File::operator=(std::move(f));

	return *this;
}

PDFHandler::PDF::~PDF() {
	if (m_doc == nullptr)
		return;
	fz_drop_document(m_pdfcontext->ctx, m_doc);
}

RenderHandler::Bitmap PDFHandler::PDF::createBitmapFromPage(RenderHandler::Direct2DContext* context, unsigned int page, Rect2D<float> rec, bool fit, float dpi) {
	// TODO error handling 
	auto& ctx = m_pdfcontext->ctx;

	auto docpage = fz_load_page(ctx, m_doc, page);
	

	auto ctm = fz_identity;
		auto size = getPageSize(page, 72);
		//ctm = fz_scale((rec.width / size.width), (rec.height / size.height));
		//ctm = fz_scale((dpi / 72.0f), (dpi / 72.0f));
		ctm = fz_scale((rec.width / size.width) * (dpi / 72.0f), (rec.height / size.height) * (dpi / 72.0f));
	

	auto fbox = fz_make_rect(0, 0, size.width, size.height);
	auto bbox = fz_round_rect(fz_transform_rect(fbox, ctm));
	auto pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 1);
	fz_clear_pixmap_with_value(ctx, pix, 0xff);
	auto dev = fz_new_draw_device(ctx, fz_identity, pix);

	pdf_run_page_with_usage(ctx, (pdf_page*)docpage, dev, ctm, "View", nullptr); 
	
	RenderHandler::Bitmap butmap(context, pix->samples, {{0, 0}, (unsigned int)bbox.x1, (unsigned int)bbox.y1}, pix->stride, dpi);

	fz_drop_pixmap(ctx, pix);
	fz_drop_page(ctx, docpage);
	fz_drop_device(ctx, dev);

	return std::move(butmap);
}

Rect2D<float> PDFHandler::PDF::getPageSize(unsigned int page, float dpi) {
	auto& ctx = m_pdfcontext->ctx;
	auto docpage = fz_load_page(ctx, m_doc, page);
	auto rect = fz_bound_page(ctx, docpage);

	fz_drop_page(ctx, docpage);
	return { {0, 0}, rect.x1 * (dpi / 72.0f), rect.y1 * (dpi / 72.0f) };
}

size_t PDFHandler::PDF::getNumberOfPages() {
	auto& ctx = m_pdfcontext->ctx;
	return fz_count_pages(ctx, m_doc);
}
