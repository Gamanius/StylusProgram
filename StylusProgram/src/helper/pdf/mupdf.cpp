#include "PDFHandler.h"
#include "mupdf/pdf.h"

PDFHandler::MUPDF::MUPDF() {
	ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);

	fz_register_document_handlers(ctx);
}

PDFHandler::MUPDF::~MUPDF() {
	fz_drop_context(ctx);
}

PDFHandler::PDF PDFHandler::MUPDF::loadPDF(const std::wstring& s) {
	auto file = FileHandler::openFile(s);
	auto stream = fz_open_memory(ctx, file.data, file.size);
	auto doc = fz_open_document_with_stream(ctx, ".pdf", stream); 
	fz_drop_stream(ctx, stream);

	
	//auto page = fz_load_page(ctx, doc, 0);
	//auto pRect = fz_bound_page(ctx, page);
	//auto ctm = fz_scale(5, 5);
	//pRect = fz_transform_rect(pRect, ctm);
	//auto bbox = fz_round_rect(pRect);

	////auto pix = fz_new_pixmap_from_page_number(ctx, doc, 1, fz_identity, fz_device_rgb(ctx), 0);
	//auto pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 1);
	//fz_clear_pixmap_with_value(ctx, pix, 0xff);
	//auto dev = fz_new_draw_device(ctx, fz_identity, pix);
	//pdf_run_page_with_usage(ctx, (pdf_page*)page, dev, ctm, "View", nullptr);
	//

	////fz_clear_pixmap_with_value(ctx, pix, 0xff);
	//PDFHandler::PDF pdf;
	//pdf.data = pix->samples;
	//pdf.size = pix->h * pix->w * 4;
	//pdf.dim.width = pix->w;
	//pdf.dim.height = pix->h;
	//pdf.stride = pix->stride;

	//pix->samples = nullptr;
	//fz_drop_document(ctx, doc);
	//fz_drop_pixmap(ctx, pix);
	//fz_drop_page(ctx, page);
	//fz_drop_device(ctx ,dev);

	//return pdf;
	//

	auto pdf = PDF(this, doc);
	pdf.data = file.data;
	file.data = nullptr;
	pdf.size = file.size;

	return std::move(pdf);
}