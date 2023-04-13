#include "RenderHandler.h"
#include "util/Logger.h"
#include "visvalingam_simplify/visvalingam_algorithm.h"

#undef min
#undef max

void calcBoundingBox(Rect2D<double>& a, Point2D<double> p) {
	if (a.upperleft.x > p.x) {
		a.width += std::abs(p.x - a.upperleft.x);
		a.upperleft.x = p.x;
	}
	if (a.upperleft.y > p.y) {
		a.height += std::abs(p.y - a.upperleft.y);
		a.upperleft.y = p.y;
	}
	a.width  = std::max(a.width,  std::abs(p.x - a.upperleft.x));
	a.height = std::max(a.height, std::abs(p.y - a.upperleft.y));
}

RenderHandler::StrokeBuilder::Stroke::Stroke(Point2D<double> p) {
	m_points = new std::vector<Point2D<double>>();
	m_points->push_back(p);

	m_boundingBox.upperleft.x = p.x;
	m_boundingBox.upperleft.y = p.y;
	m_boundingBox.width = 0;
	m_boundingBox.height= 0;
}

RenderHandler::StrokeBuilder::Stroke::~Stroke() {
	delete m_points;
	SafeRelease(&m_bezierCurveGeometry);
	SafeRelease(&m_strokeBrush);
	SafeRelease(&m_strokeStyle);
}

RenderHandler::StrokeBuilder::Stroke& RenderHandler::StrokeBuilder::Stroke::operator=(Stroke&& s) {
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

	return *this;
}

RenderHandler::StrokeBuilder::Stroke::Stroke(Stroke&& s) {
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
}

void RenderHandler::StrokeBuilder::Stroke::setStrokeBrush(ID2D1Brush* brush) {
	SafeRelease(&m_strokeBrush);
	brush->AddRef();
	m_strokeBrush = brush;
}

void RenderHandler::StrokeBuilder::Stroke::setStrokeStyle(ID2D1StrokeStyle* style) {
	SafeRelease(&m_strokeStyle);
	style->AddRef();
	m_strokeStyle = style;
}

void RenderHandler::StrokeBuilder::startStroke(Point2D<double> p, UINT32 id) {
	Stroke s(p - m_renderContext->getMatrixTranslationOffset());
	s.setStrokeBrush(m_currentInkBrush); 
	s.setStrokeStyle(m_currentLineStyle);

	m_dynamicStroke.insert(std::make_pair(id, std::move(s)));
}

void RenderHandler::StrokeBuilder::addSafeStroke(Point2D<double> p, UINT32 id) {
	if (m_dynamicStroke.find(id) == m_dynamicStroke.end())
		return;
	addStroke(p, id);
}

void RenderHandler::StrokeBuilder::addStroke(Point2D<double> p, UINT32 id) {
	auto& s = m_dynamicStroke.at(id);
	s.m_points->push_back(p - m_renderContext->getMatrixTranslationOffset());

	//update the bounding box
	calcBoundingBox(s.m_boundingBox, p);

	m_dynamicLines.push_back(std::make_tuple(s.m_points->at(s.m_points->size() - 2), s.m_points->at(s.m_points->size() - 1)));
}


double PerpendicularDistance(const Point2D<double> &pt, const Point2D<double> &lineStart, const Point2D<double> &lineEnd)
{
	double dx = lineEnd.x - lineStart.x;
	double dy = lineEnd.y - lineStart.y;

	//Normalise
	double mag = pow(pow(dx,2.0)+pow(dy,2.0),0.5);
	if(mag > 0.0)
	{
		dx /= mag; dy /= mag;
	}

	double pvx = pt.x - lineStart.x;
	double pvy = pt.y - lineStart.y;

	//Get dot product (project pv onto normalized direction)
	double pvdot = dx * pvx + dy * pvy;

	//Scale line direction vector
	double dsx = pvdot * dx;
	double dsy = pvdot * dy;

	//Subtract this from pv
	double ax = pvx - dsx;
	double ay = pvy - dsy;

	return pow(pow(ax,2.0)+pow(ay,2.0),0.5);
}

void RamerDouglasPeucker(const std::vector<Point2D<double>>& pointList, double epsilon, std::vector<Point2D<double>>& out) {
	if (pointList.size() < 2)
		throw std::invalid_argument("Not enough points to simplify");

	// Find the point with the maximum distance from line between start and end
	double dmax = 0.0;
	size_t index = 0;
	size_t end = pointList.size() - 1;
	for (size_t i = 1; i < end; i++) {
		double d = PerpendicularDistance(pointList[i], pointList[0], pointList[end]);
		if (d > dmax) {
			index = i;
			dmax = d;
		}
	}

	// If max distance is greater than epsilon, recursively simplify
	if (dmax > epsilon) {
		// Recursive call
		std::vector<Point2D<double>> recResults1;
		std::vector<Point2D<double>> recResults2;
		std::vector<Point2D<double>> firstLine(pointList.begin(), pointList.begin() + index + 1);
		std::vector<Point2D<double>> lastLine(pointList.begin() + index, pointList.end());
		RamerDouglasPeucker(firstLine, epsilon, recResults1);
		RamerDouglasPeucker(lastLine, epsilon, recResults2);

		// Build the result list
		out.assign(recResults1.begin(), recResults1.end() - 1);
		out.insert(out.end(), recResults2.begin(), recResults2.end());
		if (out.size() < 2)
			throw std::runtime_error("Problem assembling output");
	}
	else {
		//Just return start and end points
		out.clear();
		out.push_back(pointList[0]);
		out.push_back(pointList[end]);
	}
}

// https://www.quantstart.com/articles/Tridiagonal-Matrix-Algorithm-Thomas-Algorithm-in-C/
void solveTridiagonal(std::vector<double>& a,
					  std::vector<double>& b,
					  std::vector<double>& c,
					  std::vector<Point2D<double>>& d,
					  std::vector<Point2D<double>>& x) {
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
void calcBezierPoints(const std::vector<Point2D<double>>& points, std::vector<Point2D<double>>& a, std::vector<Point2D<double>>& b) {
	//create the vectos
	auto n = points.size() - 1;
	std::vector<Point2D<double>> p(n);

	p[0] = points[0] + 2 * points[1];
	for (size_t i = 1; i < n - 1; i++) {
		p[i] = 2 * (2 * points[i] + points[i + 1]);
	}
	p[n - 1] = 8 * points[n - 1] + points[n];

	// matrix
	std::vector<double> m_a(n - 1, 1);
	std::vector<double> m_b(n, 4);
	std::vector<double> m_c(n - 1, 1);
	m_a[n - 2] = 2;
	m_b[0] = 2;
	m_b[n - 1] = 7;

	solveTridiagonal(m_a, m_b, m_c, p, a);

	b[n - 1] = (a[n - 1] + points[n]) / 2.0;
	for (size_t i = 0; i < n - 1; i++) {
		b[i] = 2 * points[i + 1] - a[i + 1];
	}
}

ID2D1PathGeometry* createBezierPathGeometry(ID2D1Factory* factory, const std::vector<Point2D<double>>& points, const std::vector<Point2D<double>>& a, const std::vector<Point2D<double>>& b) {
	ID2D1PathGeometry* geo = NULL;
	ID2D1GeometrySink* pSink = NULL;
	if (factory->CreatePathGeometry(&geo) != S_OK) {
		Logger::err(L"Couldn't create Path Geometry for new Stroke");
		return nullptr;
	}
	geo->Open(&pSink);
	pSink->BeginFigure({ (float)points.at(0).x,(float)points.at(0).y }, D2D1_FIGURE_BEGIN_FILLED);
	D2D1_BEZIER_SEGMENT seg;
	for (size_t i = 0; i < a.size() - 1; i++) {
		seg.point1 = { (float)a[i].x, (float)a[i].y };
		seg.point2 = { (float)b[i].x, (float)b[i].y };
		seg.point3 = { (float)points.at(i + 1).x,(float)points.at(i + 1).y };
		pSink->AddBezier(seg);
	}
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	pSink->Close(); 
	
	SafeRelease(&pSink);

	return geo;
}

void RenderHandler::StrokeBuilder::endStroke(Point2D<double> p, UINT32 id) {
	if (m_dynamicStroke.find(id) == m_dynamicStroke.end())
		return;
	auto& s = m_dynamicStroke.at(id);
	s.m_points->push_back(p - m_renderContext->getMatrixTranslationOffset());

	if (s.m_points->size() <= 2) {
		s.m_points->push_back(p - m_renderContext->getMatrixTranslationOffset());
	}

	// simplify points
	//simp.simplify(1, simplifiedPoints);
	
	//update the bounding box
	calcBoundingBox(s.m_boundingBox, p);

	// create bezier stuff
	std::vector<Point2D<double>> ai(s.m_points->size() - 1);
	std::vector<Point2D<double>> bi(s.m_points->size() - 1);
	calcBezierPoints(*s.m_points, ai, bi);
	s.m_bezierCurveGeometry = createBezierPathGeometry(m_renderContext->m_pD2DFactory, *s.m_points, ai, bi);
	
	m_longTimeStroke.push_back(std::move(s));
	m_dynamicStroke.erase(id);

	if (!isStrokeInProgress())
		renderAllStrokes();
}

Point2D<double> RenderHandler::StrokeBuilder::getLastStroke(UINT32 id) {
	if (m_dynamicStroke.find(id) == m_dynamicStroke.end())
		return { 0, 0 };
	auto& s = m_dynamicStroke.at(id);
	return s.m_points->at(s.m_points->size() - 1);
}

void RenderHandler::StrokeBuilder::renderAllStrokes() {
	if (m_renderContext == nullptr)
		return;
	m_renderContext->beginDraw();
	m_renderContext->setCurrentViewPortMatrixActive();
	for (size_t i = 0; i < m_longTimeStroke.size(); i++) {
		auto& stroke = m_longTimeStroke[i];
		m_renderContext->m_hwndRendertarget->DrawGeometry(stroke.m_bezierCurveGeometry, stroke.m_strokeBrush, m_currentStrokeWidht, stroke.m_strokeStyle);
		//for (size_t i = 0; i < stroke.m_points->size() - 2; i++) {
		//	m_renderContext->m_hwndRendertarget->DrawLine(stroke.m_points->at(i), stroke.m_points->at(i+1), stroke.m_strokeBrush, 3, stroke.m_strokeStyle);
		//}
		//D2D1_RECT_F rect;
		//rect.left = m_longTimeStroke.at(i).m_boundingBox.upperleft.x;
		//rect.right = m_longTimeStroke.at(i).m_boundingBox.upperleft.x + m_longTimeStroke.at(i).m_boundingBox.width;
		//rect.bottom = m_longTimeStroke.at(i).m_boundingBox.height + m_longTimeStroke.at(i).m_boundingBox.upperleft.y;
		//rect.top = m_longTimeStroke.at(i).m_boundingBox.upperleft.y;
		//target->DrawRectangle(rect, m_renderContext->m_debugBrush);
	}
	m_renderContext->endDraw();
}

void RenderHandler::StrokeBuilder::renderDynamicLines() {
	if (m_dynamicLines.size() == 0)
		return;
	m_renderContext->beginDraw();
	m_renderContext->setCurrentViewPortMatrixActive();
	auto hwndtarget = m_renderContext->m_hwndRendertarget;
	for (auto& l : m_dynamicLines) {
		hwndtarget->DrawLine(std::get<0>(l), std::get<1>(l), m_currentInkBrush, m_currentStrokeWidht);
	}
	m_dynamicLines.clear();
	m_renderContext->endDraw();
}

void RenderHandler::StrokeBuilder::setRenderContext(Direct2DContext* c) {
	if (c->m_hwndRendertarget == nullptr) {
		Logger::err(L"Render Context doesn't have a valid Render Target");
		return;
	}
	if (c->m_hwndRendertarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &(m_currentInkBrush)) != S_OK) {
		Logger::err(L"Couldn't create brush");
		return;
	}
	D2D1_STROKE_STYLE_PROPERTIES props;
	props.startCap = D2D1_CAP_STYLE_ROUND;
	props.endCap = D2D1_CAP_STYLE_ROUND;
	props.dashCap = D2D1_CAP_STYLE_ROUND;
	props.dashStyle = D2D1_DASH_STYLE_SOLID;
	props.lineJoin = D2D1_LINE_JOIN_ROUND;
	float f = 1;

	if (c->m_pD2DFactory->CreateStrokeStyle(props, nullptr, 0, &m_currentLineStyle) != S_OK) {
		Logger::err(L"Couldn't create brush");
		return;
	}
	m_renderContext = c;
}

bool RenderHandler::StrokeBuilder::isStrokeInProgress() const {
	return m_dynamicStroke.size() != 0;
}

RenderHandler::StrokeBuilder::~StrokeBuilder() {
	SafeRelease(&m_currentInkBrush);
	SafeRelease(&m_currentLineStyle);
} 
