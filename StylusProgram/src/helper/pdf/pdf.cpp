#include "PDFHandler.h"
#include "mupdf/pdf.h"
#include "util/Logger.h"

const pdf_write_options default_write_options = {
	0,  /* do_incremental */
	0,  /* do_pretty */
	0,  /* do_ascii */
	1,  /* do_compress */
	1,  /* do_compress_images */
	0,  /* do_compress_fonts */
	0,  /* do_decompress */
	0,  /* do_garbage */
	0,  /* do_linear */
	0,  /* do_clean */
	0,  /* do_sanitize */
	0,  /* do_appearance */
	0,  /* do_encrypt */
	0,  /* dont_regenerate_id */
	~0, /* permissions */
	"", /* opwd_utf8[128] */
	"", /* upwd_utf8[128] */
};

PDFHandler::PDF::PDF(MUPDF* context, fz_document* doc) {
	m_doc = doc;
	m_pdfcontext = context;
	
	for (size_t i = 0; i < getNumberOfPages(); i++) {
		m_pages.push_back(PdfPage(context->getctx() ,fz_load_page(context->getctx(), doc, i)));
	}
}

/*/
PDFHandler::PDF::PDF(const PDF& f) : File(f) {
	m_doc = f.m_doc;
	fz_keep_document(f.m_pdfcontext->getctx(), m_doc);

	m_pdfcontext = f.m_pdfcontext;
}

PDFHandler::PDF& PDFHandler::PDF::operator=(const PDF& f) {
	m_doc = f.m_doc;
	m_doc->refs++;

	m_pdfcontext = f.m_pdfcontext;

	File::operator=(f);

	return *this;
}*/

PDFHandler::PDF::PDF(PDF&& f) : File(std::move(f)) {
	m_doc = f.m_doc;
	f.m_doc = nullptr;

	m_pages = std::move(f.m_pages);

	m_pdfcontext = f.m_pdfcontext;
	f.m_pdfcontext = nullptr;
}

PDFHandler::PDF& PDFHandler::PDF::operator=(PDF&& f) {
	m_doc = f.m_doc;
	f.m_doc = nullptr;

	m_pages = std::move(f.m_pages);

	m_pdfcontext = f.m_pdfcontext;
	f.m_pdfcontext = nullptr;

	File::operator=(std::move(f));

	return *this;
}

PDFHandler::PDF::~PDF() {
	if (m_doc == nullptr)
		return;
	fz_drop_document(m_pdfcontext->getctx(), m_doc);
}

RenderHandler::Bitmap PDFHandler::PDF::createBitmapFromPage(RenderHandler::Direct2DContext* context, unsigned int page, Rect2D<float> rec, float dpi) {
	// TODO error handling 
	auto ctx = m_pdfcontext->getctx();

	auto& pdfpage = getPage(page);
	auto docpage = pdfpage.page;
	
	auto ctm = fz_identity;
	auto size = getPageSize(page, 72);
	ctm = fz_scale((rec.width / size.width) * (dpi / 72.0f), (rec.height / size.height) * (dpi / 72.0f));
	
	auto fbox = fz_make_rect(0, 0, size.width, size.height);
	auto bbox = fz_round_rect(fz_transform_rect(fbox, ctm));
	auto pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 1);
	fz_clear_pixmap_with_value(ctx, pix, 0xff);
	auto dev = fz_new_draw_device(ctx, fz_identity, pix);

	pdf_run_page_with_usage(ctx, (pdf_page*)docpage, dev, ctm, "View", nullptr); 
	
	RenderHandler::Bitmap butmap(context, pix->samples, {{0, 0}, (unsigned int)bbox.x1, (unsigned int)bbox.y1}, pix->stride, dpi);
	
	fz_drop_pixmap(ctx, pix);
	fz_drop_device(ctx, dev);

	return std::move(butmap);
}

RenderHandler::Bitmap PDFHandler::PDF::createBitmapFromPage(RenderHandler::Direct2DContext* context, unsigned int page, Rect2D<float> destination, Rect2D<float> source, float dpi) {
	auto ctx = m_pdfcontext->getctx();
	// load page
	auto& pdfpage = getPage(page);
	auto docpage = pdfpage.page;

	// create the matrix
	auto scale = fz_identity;
	scale = fz_scale((destination.width / source.width) * (dpi / 72.0f), (destination.height / source.height) * (dpi / 72.0f));
	auto transform = fz_translate(-source.upperleft.x, -source.upperleft.y);

	auto fbox = fz_make_rect(0, 0, source.width, source.height);
	auto bbox = fz_round_rect(fz_transform_rect(fbox, scale));
	auto pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 1);
	fz_clear_pixmap_with_value(ctx, pix, 0xff);
	auto dev = fz_new_draw_device(ctx, fz_identity, pix);
	pdf_run_page_with_usage(ctx, (pdf_page*)docpage, dev, fz_concat(transform, scale), "View", nullptr);


	RenderHandler::Bitmap butmap(context, pix->samples, { {0, 0}, (unsigned int)bbox.x1, (unsigned int)bbox.y1 }, pix->stride, dpi);

	fz_drop_pixmap(ctx, pix);
	fz_drop_device(ctx, dev);

	return std::move(butmap);
}

Rect2D<float> PDFHandler::PDF::getPageSize(unsigned int page, float dpi) {
	auto ctx = m_pdfcontext->getctx();
	auto docpage = fz_load_page(ctx, m_doc, page);
	auto rect = fz_bound_page(ctx, docpage);

	fz_drop_page(ctx, docpage);
	return { {0, 0}, rect.x1 * (dpi / 72.0f), rect.y1 * (dpi / 72.0f) };
}

void PDFHandler::PDF::save(const std::wstring& s) {
	auto ctx = m_pdfcontext->getctx();
	fz_buffer* buf = fz_new_buffer(ctx, 0);
	fz_output* output = fz_new_output_with_buffer(ctx, buf);
	pdf_write_document(ctx, (pdf_document*)m_doc, output, &default_write_options);
	FileHandler::saveFile(s, buf->data, buf->len);

	fz_close_output(ctx, output);
	fz_drop_output(ctx, output);
	fz_drop_buffer(ctx, buf);
}

PDFHandler::PdfPage& PDFHandler::PDF::getPage(size_t page) {
	return m_pages[page];
}

size_t PDFHandler::PDF::getNumberOfPages() {
	auto ctx = m_pdfcontext->getctx();
	return fz_count_pages(ctx, m_doc);
}
