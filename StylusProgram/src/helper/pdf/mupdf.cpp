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

	auto pdf = PDF(this, doc);
	pdf.data = file.data;
	file.data = nullptr;
	pdf.size = file.size;

	return std::move(pdf);
}

fz_context* PDFHandler::MUPDF::getctx() const {
	return ctx;
}
