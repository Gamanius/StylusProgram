#pragma once

#include "render/RenderHandler.h"
#include "util/FileHandler.h"
#include <mupdf/fitz.h>

#ifndef PDF_HANDLER_H
#define PDF_HANDLER_H

namespace RenderHandler {
	struct Bitmap;
	class Direct2DContext;
}

namespace PDFHandler {
	class MUPDF;
	struct PDF : public FileHandler::File {
		fz_document* m_doc = nullptr;
		MUPDF* m_pdfcontext = nullptr;

		PDF() = default;
		PDF(MUPDF* context, fz_document* doc);
		PDF(const PDF& f);
		PDF& operator=(const PDF& p);
		PDF(PDF&& p);
		PDF& operator=(PDF&& p);
		~PDF();

		RenderHandler::Bitmap createBitmapFromPage(RenderHandler::Direct2DContext* context, unsigned int page, Rect2D<float> rec, float dpi = 72);
		RenderHandler::Bitmap createBitmapFromPage(RenderHandler::Direct2DContext* context, unsigned int page, Rect2D<float> destination, Rect2D<float> source, float dpi = 72);
		Rect2D<float> getPageSize(unsigned int page, float dpi = 72);
		size_t getNumberOfPages();
	};

	class MUPDF {
		fz_context* ctx = nullptr;
	public:
		MUPDF();
		~MUPDF();

		PDF loadPDF(const std::wstring& s);

		friend PDF;
	};
}

#endif // !PDF_HANDLER_H