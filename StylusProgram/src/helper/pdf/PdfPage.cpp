#include "PDFHandler.h"

PDFHandler::PdfPage::PdfPage(fz_context* ctx, fz_page* page) : ctx(ctx), page(page) {}

PDFHandler::PdfPage::PdfPage(PdfPage&& p) {
    this->ctx = p.ctx;
    this->page = p.page;

    p.ctx = nullptr;
    p.page = nullptr;
}

PDFHandler::PdfPage& PDFHandler::PdfPage::operator=(PdfPage&& p) {
    this->ctx = p.ctx;
    this->page = p.page;

    p.ctx = nullptr;
    p.page = nullptr;
    return *this;
}

PDFHandler::PdfPage::~PdfPage() {
    if (page == nullptr)
        return;
    // dont delete ctx because it is not owned by this object
    fz_drop_page(ctx, page);
}
