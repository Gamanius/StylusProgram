#include "PDFHandler.h"

PDFHandler::AnnotationHandler::InkStroke::InkStroke(std::vector<Point2D<float>>* p) {
	m_points = p;
}

PDFHandler::AnnotationHandler::InkStroke::~InkStroke() {
	delete m_points;
	//SafeRelease(&m_bezierCurveGeometry);
	if (m_bezierCurveGeometry != nullptr)
		Logger::add(m_bezierCurveGeometry->Release());
	SafeRelease(&m_strokeBrush);
	SafeRelease(&m_strokeStyle);
}

PDFHandler::AnnotationHandler::InkStroke& PDFHandler::AnnotationHandler::InkStroke::operator=(InkStroke&& s) {
	this->m_points = s.m_points;
	s.m_points = nullptr;

	this->m_bezierCurveGeometry = s.m_bezierCurveGeometry;
	s.m_bezierCurveGeometry = nullptr;

	this->m_strokeBrush = s.m_strokeBrush;
	s.m_strokeBrush = nullptr;

	this->m_strokeStyle = s.m_strokeStyle;
	s.m_strokeStyle = nullptr;

	this->m_boundingBox = s.m_boundingBox;
	s.m_boundingBox = { {0, 0}, 0, 0 };

	this->m_strokeWidth = s.m_strokeWidth;
	s.m_strokeWidth = 0;

	return *this;
}

PDFHandler::AnnotationHandler::InkStroke::InkStroke(InkStroke&& s) { 
	this->m_points = s.m_points;
	s.m_points = nullptr;

	this->m_bezierCurveGeometry = s.m_bezierCurveGeometry;
	s.m_bezierCurveGeometry = nullptr;

	this->m_strokeBrush = s.m_strokeBrush;
	s.m_strokeBrush = nullptr;

	this->m_strokeStyle = s.m_strokeStyle;
	s.m_strokeStyle = nullptr;

	this->m_strokeWidth = s.m_strokeWidth;
	s.m_strokeWidth = 0;

	this->m_boundingBox = s.m_boundingBox;
	s.m_boundingBox = { {0, 0}, 0, 0 };
}

PDFHandler::AnnotationHandler::PdfStroke::PdfStroke(fz_context* ctx, pdf_annot* annot, Point2D<float> offset) {
	this->ctx = ctx;
	pdf_keep_annot(ctx, annot);
	m_annot = annot;

	m_boundingBox = Rect2D<float>(pdf_bound_annot(ctx, m_annot));
	m_boundingBox.upperleft += offset;

	auto amountOfStrokes = pdf_annot_ink_list_count(ctx, annot);
	if (amountOfStrokes > 1) {
		Logger::err(L"There is no support for multiple strokes in one annotation yet");
	}

	auto strokeCount = pdf_annot_ink_list_stroke_count(ctx, annot, 0);
	m_points = new std::vector<Point2D<float>>();
	for (size_t k = 0; k < strokeCount; k++) {
		m_points->push_back(Point2D<float>(pdf_annot_ink_list_stroke_vertex(ctx, annot, 0, k)) + offset);
	}

	m_strokeWidth = pdf_annot_border_width(ctx, annot);
}

PDFHandler::AnnotationHandler::PdfStroke::~PdfStroke() {
	pdf_drop_annot(ctx, m_annot);
	delete m_points;
}

PDFHandler::AnnotationHandler::PdfStroke& PDFHandler::AnnotationHandler::PdfStroke::operator=(PdfStroke&& s) {
	this->ctx = s.ctx;
	s.ctx = nullptr;

	this->m_annot = s.m_annot;
	s.m_annot = nullptr;

	this->m_points = s.m_points;
	s.m_points = nullptr;

	this->m_strokeWidth = s.m_strokeWidth;
	s.m_strokeWidth = 0;

	this->m_boundingBox = s.m_boundingBox;
	s.m_boundingBox = Rect2D<float>();

	return *this;
}

PDFHandler::AnnotationHandler::PdfStroke::PdfStroke(PdfStroke&& s) {
	this->ctx = s.ctx;
	s.ctx = nullptr;

	this->m_annot = s.m_annot;
	s.m_annot = nullptr;

	this->m_points = s.m_points;
	s.m_points = nullptr;

	this->m_strokeWidth = s.m_strokeWidth;
	s.m_strokeWidth = 0;

	this->m_boundingBox = s.m_boundingBox;
	s.m_boundingBox = Rect2D<float>();
}


void PDFHandler::AnnotationHandler::InkStroke::setStrokeBrush(ID2D1SolidColorBrush* brush) {
	SafeRelease(&m_strokeBrush);
	brush->AddRef();
	m_strokeBrush = brush;
}

void PDFHandler::AnnotationHandler::InkStroke::setStrokeStyle(ID2D1StrokeStyle* style) {
	SafeRelease(&m_strokeStyle);
	style->AddRef();
	m_strokeStyle = style;
}

PDFHandler::AnnotationHandler::AnnotationHandler(PDFHandler::PDF* pdf, RenderHandler::PDFBuilder* context) {
	m_pdf = pdf;
	m_pdfbuilder = context;
	m_inkstrokes = std::vector<std::list<InkStroke>*>(m_pdf->getNumberOfPages(), nullptr); 
	m_strokeBuilder = new RenderHandler::StrokeBuilder();


	auto ctx = pdf->m_pdfcontext->getctx();
	for (size_t i = 0; i < m_pdf->getNumberOfPages(); i++) {
		// fill the ink strokes vector
		m_inkstrokes[i] = new std::list<InkStroke>();
		// get the annotations from the page
		auto& pdfpage = m_pdf->getPage(i);
		fz_page* page = pdfpage.page;

		auto annot = pdf_first_annot(ctx, (pdf_page*) page);/*
		if (annot == nullptr)
			m_pdfinkannotations.push_back(nullptr);
		else*/
		m_pdfinkannotations.push_back(new std::vector<PdfStroke>());
		
		auto offset = context->getSizeAndPositionOfPage(i).upperleft;

		while (annot) {
			// TODO support for other types of annotattions?
			// for now just support ink annotations
			if (pdf_annot_type(ctx, annot) == PDF_ANNOT_INK) {
				m_pdfinkannotations[i]->push_back(std::move(PdfStroke(ctx, annot, offset)));  
			}
			annot = pdf_next_annot(ctx, annot);
		}
	}

	// create the resources for the ink strokes
	if (context->m_rendercontext->getRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &(m_currentInkBrush)) != S_OK) {
		Logger::err(L"Couldn't create brush");
		return;
	}

	D2D1_STROKE_STYLE_PROPERTIES props;
	props.startCap = D2D1_CAP_STYLE_ROUND;
	props.endCap = D2D1_CAP_STYLE_ROUND;
	props.dashCap = D2D1_CAP_STYLE_ROUND;
	props.dashStyle = D2D1_DASH_STYLE_SOLID;
	props.lineJoin = D2D1_LINE_JOIN_ROUND;
	
	if (context->m_rendercontext->getFactory()->CreateStrokeStyle(props, nullptr, 0, &m_currentLineStyle) != S_OK) {
		Logger::err(L"Couldn't create brush"); 
		return;
	}

	m_strokeBuilder->m_annotationHandler = this;
}


//PDFHandler::AnnotationHandler::AnnotationHandler(const AnnotationHandler& a) {
//	auto ctx = a.m_pdf->m_pdfcontext->getctx();
//	for (size_t i = 0; i < a.m_pdfinkannotations.size(); i++) {
//		auto vec = a.m_pdfinkannotations[i];
//		if (vec == nullptr)
//			continue;
//		for (size_t i = 0; i < vec->size(); i++) {
//			pdf_keep_annot(ctx, vec->at(i));
//		}
//	}
//	m_pdf = a.m_pdf;
//	m_pdfinkannotations = std::vector(a.m_pdfinkannotations);
//}
//
//PDFHandler::AnnotationHandler& PDFHandler::AnnotationHandler::operator=(const AnnotationHandler& a) {
//	auto ctx = a.m_pdf->m_pdfcontext->getctx();
//	for (size_t i = 0; i < a.m_pdfinkannotations.size(); i++) {
//		auto vec = a.m_pdfinkannotations[i];
//		if (vec == nullptr)
//			continue;
//		for (size_t i = 0; i < vec->size(); i++) {
//			pdf_keep_annot(ctx, vec->at(i));
//		}
//	}
//	m_pdf = a.m_pdf;
//	m_pdfinkannotations = std::vector(a.m_pdfinkannotations);
//
//	return *this;
//}

PDFHandler::AnnotationHandler::AnnotationHandler(AnnotationHandler&& a) {
	m_pdf = a.m_pdf;
	m_pdfbuilder = a.m_pdfbuilder;
	m_pdfinkannotations = std::move(a.m_pdfinkannotations);
	m_inkstrokes = std::move(a.m_inkstrokes);

	a.m_pdf = nullptr;
}

PDFHandler::AnnotationHandler& PDFHandler::AnnotationHandler::operator=(AnnotationHandler&& a) {
	m_pdf = a.m_pdf;
	m_pdfbuilder = a.m_pdfbuilder;
	m_pdfinkannotations = std::move(a.m_pdfinkannotations);
	m_inkstrokes = std::move(a.m_inkstrokes);

	a.m_pdf = nullptr;

	return *this;
}

PDFHandler::AnnotationHandler::~AnnotationHandler() {
	// dont delete the pdf or the builder because we are borrowing them
	for (size_t i = 0; i < m_pdfinkannotations.size(); i++) {
		if (m_pdfinkannotations[i] != nullptr) {
			delete m_pdfinkannotations[i];
		}
	}

	for (size_t i = 0; i < m_inkstrokes.size(); i++) {
		delete m_inkstrokes[i];
	}

	SafeRelease(&m_currentInkBrush);
	SafeRelease(&m_currentLineStyle);
}

void calcBoundingBox(Rect2D<float>& a, Point2D<float> p) {
	if (a.upperleft.x > p.x) {
		a.width += std::abs(p.x - a.upperleft.x);
		a.upperleft.x = p.x;
	}
	if (a.upperleft.y > p.y) {
		a.height += std::abs(p.y - a.upperleft.y);
		a.upperleft.y = p.y;
	}
	a.width = max(a.width, std::abs(p.x - a.upperleft.x));
	a.height = max(a.height, std::abs(p.y - a.upperleft.y));
}

Rect2D<float> getBoundingBox(std::vector<Point2D<float>>* points) {
	Rect2D<float> r(points->at(0), 0, 0);
	for (size_t i = 0; i < points->size(); i++) {
		calcBoundingBox(r, points->at(i));
	}
	return r;
}

// https://www.quantstart.com/articles/Tridiagonal-Matrix-Algorithm-Thomas-Algorithm-in-C/
void solveTridiagonal(std::vector<float>& a,
					  std::vector<float>& b,
					  std::vector<float>& c,
					  std::vector<Point2D<float>>& d,
					  std::vector<Point2D<float>>& x) {
	for (size_t i = 1; i < d.size(); i++) {
		auto w = a[i - 1] / b[i - 1];
		b[i] = b[i] - w * c[i - 1];
		d[i] = d[i] - w * d[i - 1];
	}

	x[d.size() - 1] = d[d.size() - 1] / b[d.size() - 1];
	for (int i = d.size() - 2; i >= 0; i--) {
		x[i] = (d[i] - c[i] * x[i + 1]) / b[i];
	}
}

//https://towardsdatascience.com/b%C3%A9zier-interpolation-8033e9a262c2
void calcBezierPoints(const std::vector<Point2D<float>>* points, std::vector<Point2D<float>>& a, std::vector<Point2D<float>>& b) {
	//create the vectos
	auto n = points->size() - 1;
	std::vector<Point2D<float>> p(n);

	p[0] = points->at(0) + 2 * points->at(1);
	for (size_t i = 1; i < n - 1; i++) {
		p[i] = 2 * (2 * points->at(i) + points->at(i + 1));
	}
	p[n - 1] = 8 * points->at(n - 1) + points->at(n); 

	// matrix
	std::vector<float> m_a(n - 1, 1);
	std::vector<float> m_b(n, 4);
	std::vector<float> m_c(n - 1, 1);
	m_a[n - 2] = 2;
	m_b[0] = 2;
	m_b[n - 1] = 7;

	solveTridiagonal(m_a, m_b, m_c, p, a);

	b[n - 1] = (a[n - 1] + points->at(n)) / 2.0;
	for (size_t i = 0; i < n - 1; i++) {
		b[i] = 2 * points->at(i + 1) - a[i + 1];
	}
}

ID2D1PathGeometry* createBezierPathGeometry(ID2D1Factory* factory, const std::vector<Point2D<float>>* points, const std::vector<Point2D<float>>& a, const std::vector<Point2D<float>>& b) {
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* pSink = NULL;
	if (factory->CreatePathGeometry(&geo) != S_OK) {
		Logger::err(L"Couldn't create Path Geometry for new Stroke");
		return nullptr;
	}
	geo->Open(&pSink);
	pSink->BeginFigure({ (float)points->at(0).x,(float)points->at(0).y }, D2D1_FIGURE_BEGIN_FILLED);
	D2D1_BEZIER_SEGMENT seg;
	for (size_t i = 0; i < a.size() - 1; i++) {
		seg.point1 = { (float)a[i].x, (float)a[i].y };
		seg.point2 = { (float)b[i].x, (float)b[i].y };
		seg.point3 = { (float)points->at(i + 1).x,(float)points->at(i + 1).y };
		pSink->AddBezier(seg);
	}
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	pSink->Close();

	SafeRelease(&pSink);

	return geo;
}


void PDFHandler::AnnotationHandler::strokeEnd(std::vector<Point2D<float>>* points, long page) {
	InkStroke newStroke(points);
	newStroke.m_boundingBox = getBoundingBox(points);

	newStroke.setStrokeBrush(m_currentInkBrush);
	newStroke.setStrokeStyle(m_currentLineStyle);
	newStroke.m_strokeWidth = m_currentStrokeWidht;

	// create bezier stuff
	std::vector<Point2D<float>> ai(points->size() - 1);
	std::vector<Point2D<float>> bi(points->size() - 1);
	calcBezierPoints(points, ai, bi);
	newStroke.m_bezierCurveGeometry = createBezierPathGeometry(m_pdfbuilder->m_rendercontext->getFactory(), points, ai, bi);

	// put the new stroke into the m_inkstroke vector
	m_inkstrokes[page]->push_back(std::move(newStroke));
}

void PDFHandler::AnnotationHandler::startStroke(Point2D<float> p, UINT32 id) {
	m_dynamicStroke[id] = std::make_tuple(new std::vector<Point2D<float>>(), -1);
	for (size_t i = 0; i < m_pdf->getNumberOfPages(); i++) {
		if (m_pdfbuilder->getSizeAndPositionOfPage(i).intersects(p)) {
			std::get<0>(m_dynamicStroke[id])->push_back(p);
			std::get<1>(m_dynamicStroke[id]) = i;
			break;
		}
	}
}

void PDFHandler::AnnotationHandler::addStroke(Point2D<float> p, UINT32 id) {
	if (m_dynamicStroke.find(id) == m_dynamicStroke.end())
		return;

	auto& ref = m_dynamicStroke[id];
	// check if the list is empty
	if (std::get<0>(ref)->size() == 0) {
		// iterate through all pages
		for (size_t i = 0; i < m_pdf->getNumberOfPages(); i++) { 
			// if the points is on a pdf page add the point to the list
			if (m_pdfbuilder->getSizeAndPositionOfPage(i).intersects(p)) { 
				std::get<0>(m_dynamicStroke[id])->push_back(p);
				std::get<1>(m_dynamicStroke[id]) = i;
				break;
			}
		}
		return;
	}

	// everything is good
	if (m_pdfbuilder->getSizeAndPositionOfPage(std::get<1>(ref)).intersects(p)) {
		m_strokeBuilder->addDynamicLine(std::get<0>(ref)->back(), p);
		std::get<0>(ref)->push_back(p); 
		return;
	}

	// point is not a pdf anymore 
	// we dont need to delete the old vector because it will be deleted by the InkStroke object that 
	// is created in the strokeEnd function
	strokeEnd(std::get<0>(ref), std::get<1>(ref));
	m_dynamicStroke[id] = std::make_tuple<std::vector<Point2D<float>>*, long>(new std::vector<Point2D<float>>(), -1);
} 

void PDFHandler::AnnotationHandler::endStroke(Point2D<float> p, UINT32 id) {
	if (m_dynamicStroke.find(id) == m_dynamicStroke.end())
		return;

	auto& ref = m_dynamicStroke[id];

	// check if the list is empty
	if (std::get<0>(ref)->size() <= 2) {
		delete std::get<0>(ref);
		m_dynamicStroke.erase(id);
		return;
	}

	// check if the last point should be thrown away or not
	if (m_pdfbuilder->getSizeAndPositionOfPage(std::get<1>(ref)).intersects(p)) {
		// add point to list
		m_strokeBuilder->addDynamicLine(std::get<0>(ref)->back(), p);
		std::get<0>(ref)->push_back(p);
	}

	// we dont need to delete the old vector because it will be deleted by the InkStroke object that 
	// is created in the strokeEnd function
	strokeEnd(std::get<0>(ref), std::get<1>(ref));
	m_dynamicStroke.erase(id);
} 

void PDFHandler::AnnotationHandler::eraser(Point2D<float> p) {
	// first check on which page we are
	bool foundpage = false;
	size_t page = 0;
	for (size_t i = 0; i < m_pdf->getNumberOfPages(); i++) {
		if (m_pdfbuilder->getSizeAndPositionOfPage(i).intersects(p)) {
			page = i;
			foundpage = true;
			break;
		}
	}
	
	//return if no page was found
	if (!foundpage)
		return;

	// keep track if any line was removed
	bool removedLine = false;

	// do the custom strokes
	auto it = m_inkstrokes[page]->begin();
	while (it != m_inkstrokes[page]->end()) {
		// check if bounding box intersects
		bool isLineRemoved = false;
		if (it->m_boundingBox.intersects(p)) {
			// iterate through all points that define the stroke
			for (size_t i = 1; i < it->m_points->size(); i++) {
				// check if the distance between a line and the eraser tip is smaller than the widht of the line
				// and the size of the eraser tip
				if (pointToLineDistance(it->m_points->at(i), it->m_points->at(i - 1), p) < m_pdfbuilder->m_rendercontext->DptoPx(m_currentEraserWidht) + it->m_strokeWidth) { 
					it = m_inkstrokes[page]->erase(it);
					removedLine = true;
					isLineRemoved = true;
					break;
				}
			}
		}
		if (!isLineRemoved)
			++it;
	}

	// do the pdf strokes
	auto it2 = m_pdfinkannotations[page]->begin();
	while (it2 != m_pdfinkannotations[page]->end()) {
		bool isLineRemoved = false;
		if (it2->m_boundingBox.intersects(p)) {
			// edge case were m_points == 1
			if (it2->m_points->size() == 1) {
				if (it2->m_points->at(0).distance(p) < m_pdfbuilder->m_rendercontext->DptoPx(m_currentEraserWidht) + it2->m_strokeWidth) {
					pdf_delete_annot(m_pdf->m_pdfcontext->getctx(), m_pdf->getPage(page), it2->m_annot);
					it2 = m_pdfinkannotations[page]->erase(it2);
					removedLine = true;
					isLineRemoved = true;
				}
			}
			else {
				for (size_t i = 1; i < it2->m_points->size(); i++) {
					if (pointToLineDistance(it2->m_points->at(i), it2->m_points->at(i - 1), p) < m_pdfbuilder->m_rendercontext->DptoPx(m_currentEraserWidht) + it2->m_strokeWidth) {
						pdf_delete_annot(m_pdf->m_pdfcontext->getctx(), m_pdf->getPage(page), it2->m_annot);
						it2 = m_pdfinkannotations[page]->erase(it2);
						removedLine = true;
						isLineRemoved = true;
						break;
					}
				}
			}
		}
		if (!isLineRemoved)
			++it2;
	}

	if (removedLine) {
		m_pdfbuilder->m_rendercontext->render();
	}
}

void PDFHandler::AnnotationHandler::bakeAnnotations() {
	auto ctx = m_pdf->m_pdfcontext->getctx();

	for (size_t i = 0; i < m_inkstrokes.size(); i++) {
		auto& page = m_pdf->getPage(i);
		// iterate through all strokes on one page
		auto it = m_inkstrokes[i]->begin();
		while (it != m_inkstrokes[i]->end()) {
			// create annottation
			pdf_annot* annot = nullptr;
			annot = pdf_create_annot(ctx, page, PDF_ANNOT_INK);
			// add stroke and the points to the annotation
			pdf_add_annot_ink_list_stroke(ctx, annot);
			for (size_t j = 0; j < it->m_points->size(); j++) {
				pdf_add_annot_ink_list_stroke_vertex(ctx, annot, it->m_points->at(j) - m_pdfbuilder->getSizeAndPositionOfPage(i).upperleft);
			}
			// set the width
			pdf_set_annot_border_width(ctx, annot, it->m_strokeWidth);
			// and get color
			float color[3];
			color[0] = it->m_strokeBrush->GetColor().r;
			color[1] = it->m_strokeBrush->GetColor().g;
			color[2] = it->m_strokeBrush->GetColor().b;
			pdf_set_annot_color(ctx, annot, 3, (float*)&color);
			//update the annotation
			pdf_update_annot(ctx, annot); 
			// get the offset of the page
			auto offset = m_pdfbuilder->getSizeAndPositionOfPage(i).upperleft;
			// push the annotation into the pdfinkannotation buffer
			m_pdfinkannotations[i]->push_back(std::move(PdfStroke(ctx, annot, offset)));
			// and remove reference from here
			pdf_drop_annot(ctx, annot);
			++it;
		}
	}


	for (size_t i = 0; i < m_inkstrokes.size(); i++) {
		m_inkstrokes[i]->clear();
	}
}

RenderHandler::StrokeBuilder* PDFHandler::AnnotationHandler::getStrokeBuilder() const {
	return m_strokeBuilder;
}

bool PDFHandler::AnnotationHandler::isStrokeinProgress() const {
	return m_dynamicStroke.size() != 0;
}

RenderHandler::Direct2DContext* PDFHandler::AnnotationHandler::getContext() const {
	return m_pdfbuilder->m_rendercontext;
}
