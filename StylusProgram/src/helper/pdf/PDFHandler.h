#pragma once

#include "render/RenderHandler.h"
#include "util/FileHandler.h"
#include <mupdf/fitz.h>
#include "util/Logger.h"
#include "mupdf/pdf.h"

#ifndef PDF_HANDLER_H
#define PDF_HANDLER_H

namespace RenderHandler {
	struct Bitmap;
	class Direct2DContext;
	class StrokeBuilder;
	class PDFBuilder;
}

namespace PDFHandler {
	class MUPDF;
	class AnnotationHandler;

	struct PdfPage {
	private:
		fz_context* ctx = nullptr;
	public:
		fz_page* page = nullptr;

		PdfPage(fz_context* ctx, fz_page* page); 
		PdfPage(const PdfPage& p) = delete;
		PdfPage& operator=(const PdfPage& p) = delete;
		PdfPage(PdfPage&& p);
		PdfPage& operator=(PdfPage&& p);
		~PdfPage();

		operator pdf_page*() {
			return (pdf_page*)page;
		}
	};

	struct PDF : public FileHandler::File {
		fz_document* m_doc = nullptr;
		MUPDF* m_pdfcontext = nullptr;
		std::vector<PdfPage> m_pages;

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

		void save(const std::wstring& s);
		// returns a pdfpage
		PdfPage& getPage(size_t page);
		size_t getNumberOfPages();
	};

	class AnnotationHandler {
		struct InkStroke {
			// the points that define a stroke
			std::vector<Point2D<float>>* m_points;
			ID2D1PathGeometry* m_bezierCurveGeometry = nullptr;
			ID2D1SolidColorBrush* m_strokeBrush = nullptr;
			ID2D1StrokeStyle* m_strokeStyle = nullptr;
			float m_strokeWidth = 1.0f;
			Rect2D<float> m_boundingBox;

			InkStroke(std::vector<Point2D<float>>* p);

			~InkStroke();

			InkStroke(const InkStroke& s) = delete; // dont copy that array...or mess with reference count
			InkStroke& operator=(const InkStroke&) = delete;

			InkStroke& operator=(InkStroke&& s);
			InkStroke(InkStroke&& s);

			void setStrokeBrush(ID2D1SolidColorBrush* brush);
			void setStrokeStyle(ID2D1StrokeStyle* style);
		};
		struct PdfStroke {
			fz_context* ctx = nullptr;
			pdf_annot* m_annot = nullptr;
			std::vector<Point2D<float>>* m_points = nullptr;
			float m_strokeWidth = 1.0f;
			Rect2D<float> m_boundingBox;

			//pdf annot are supposed to be only of type PDF_ANNOT_INK
			PdfStroke(fz_context* ctx, pdf_annot* annot);

			~PdfStroke();

			PdfStroke(const PdfStroke& s) = delete; // i just delete the copy constructor bc i'm scared of reference counting
			PdfStroke& operator=(const PdfStroke&) = delete;

			PdfStroke& operator=(PdfStroke&& s);
			PdfStroke(PdfStroke&& s);
		};

		PDF* m_pdf;
		RenderHandler::PDFBuilder* m_pdfbuilder;
		std::vector<std::vector<PdfStroke>*> m_pdfinkannotations;
		std::vector<std::list<InkStroke>*> m_inkstrokes;

		std::map<UINT32, std::tuple<std::vector<Point2D<float>>*, long>> m_dynamicStroke;

		//Render Stuff
		ID2D1SolidColorBrush* m_currentInkBrush = nullptr;
		ID2D1StrokeStyle* m_currentLineStyle = nullptr;
		float m_currentStrokeWidht = 1;
		float m_currentEraserWidht = 10;

		RenderHandler::StrokeBuilder* m_strokeBuilder;

		void strokeEnd(std::vector<Point2D<float>>* points, long page);

	public:
		AnnotationHandler() = default;
		// PDF and PDFBuilder are borrowed
		AnnotationHandler(PDF* pdf, RenderHandler::PDFBuilder* context);
		AnnotationHandler(const AnnotationHandler& a) = delete;
		AnnotationHandler& operator=(const AnnotationHandler& a) = delete;
		AnnotationHandler(AnnotationHandler&& a);
		AnnotationHandler& operator=(AnnotationHandler&& a);
		~AnnotationHandler();


		void startStroke(Point2D<float> p, UINT32 id);
		void addStroke(Point2D<float> p, UINT32 id);
		void endStroke(Point2D<float> p, UINT32 id);
		void eraser(Point2D<float> p);

		// Will put the annotations into the pdf pages
		void bakeAnnotations();

		RenderHandler::StrokeBuilder* getStrokeBuilder() const;

		bool isStrokeinProgress() const;

		RenderHandler::Direct2DContext* getContext() const;

		friend RenderHandler::StrokeBuilder;
	};

	class MUPDF {
		fz_context* ctx = nullptr;
	public:
		MUPDF();
		~MUPDF();

		PDF loadPDF(const std::wstring& s);

		fz_context* getctx() const;
	};
}

#endif // !PDF_HANDLER_H