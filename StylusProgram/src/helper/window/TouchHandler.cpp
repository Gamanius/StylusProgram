#include "WindowHandler.h"

WindowHandler::TouchHandler::TouchHandler(RenderHandler::Direct2DContext* context) {
	m_renderContext = context;
}

void WindowHandler::TouchHandler::startTouchGesture(POINTER_INFO fingerPosition) {
	if (m_firstFingerActive && m_secondFingerActive)
		return;

	if (!m_firstFingerActive) {
		m_initialFirstFinger = fingerPosition;
		m_lastFirstFinger = fingerPosition;

		m_initialOffset = m_renderContext->getMatrixTranslationOffset() - m_renderContext->scaleFromViewPortPixelsToDisplayPixels({0, 0});
		m_firstFingerActive = true;

		return;
	}

	if (!m_secondFingerActive) {
		m_initialSecondFinger = fingerPosition;
		m_lastSecondFinger = fingerPosition;

		m_renderContext->setMatrixTranslationOffset(m_renderContext->scaleFromViewPortPixelsToDisplayPixels(m_lastFirstFinger.pos - m_initialFirstFinger.pos) + m_initialOffset); 
		m_initialOffset = m_renderContext->getMatrixTranslationOffset();
		m_initialFirstFinger = m_lastFirstFinger;

		m_initialScale = m_renderContext->getMatrixScaleOffset();
		m_secondFingerActive = true;

		return;
	}
}

void WindowHandler::TouchHandler::updateTouchGesture(POINTER_INFO fingerPosition) {
	if (fingerPosition.id == m_initialFirstFinger.id) {
		m_lastFirstFinger = fingerPosition;
	}
	else if (fingerPosition.id == m_initialSecondFinger.id) {
		m_lastSecondFinger = fingerPosition;
	}

	// first finger 
	if (!m_secondFingerActive) {
		m_renderContext->setMatrixTranslationOffset(m_renderContext->scaleFromViewPortPixelsToDisplayPixels(m_lastFirstFinger.pos - m_initialFirstFinger.pos) + m_initialOffset);
		m_lastFirstFinger = fingerPosition;
		return;
	}

	// two finger gesture
	// calculate the scaling factor
	double distance = (m_lastFirstFinger.pos - m_lastSecondFinger.pos).distance();
	double initialDistance = (m_initialFirstFinger.pos - m_initialSecondFinger.pos).distance();
	double scale = distance / initialDistance;
	// calculate the center
	Point2D<double> center = (m_lastFirstFinger.pos + m_lastSecondFinger.pos) / 2.0f;
	Point2D<double> initialCenter = (m_initialFirstFinger.pos + m_initialSecondFinger.pos) / 2.0f;
	// set the matrix scale offset
	m_renderContext->setMatrixScaleOffset(m_initialScale * scale, initialCenter);
	// calculate the new scaled points and the translation
	Point2D<double> transformedInitialCenter = m_renderContext->scaleFromViewPortPixelsToDisplayPixels(initialCenter);
	Point2D<double> transformedCenter = m_renderContext->scaleFromViewPortPixelsToDisplayPixels(center);
	Point2D<double> translation = (transformedCenter - transformedInitialCenter); 
	m_renderContext->setMatrixTranslationOffset(m_initialOffset + translation);
}

void WindowHandler::TouchHandler::stopTouchGestureOfFinger(POINTER_INFO finger) {
	// first finger
	if (finger.id == m_initialFirstFinger.id) {
		if (m_secondFingerActive) {
			m_secondFingerActive = false;
			m_initialFirstFinger = m_lastSecondFinger;
			m_initialOffset = m_renderContext->getMatrixTranslationOffset() - m_renderContext->scaleFromViewPortPixelsToDisplayPixels({ 0, 0 });
		}
		else {
			m_firstFingerActive = false;
		}
	}
	// second finger
	else if (finger.id == m_initialSecondFinger.id) {
		m_secondFingerActive = false;
		m_initialFirstFinger = m_lastFirstFinger;
		m_initialOffset = m_renderContext->getMatrixTranslationOffset() - m_renderContext->scaleFromViewPortPixelsToDisplayPixels({ 0, 0 });
		return;
	}
}

bool WindowHandler::TouchHandler::isGestureInProgress() const {
	return m_firstFingerActive || m_secondFingerActive;
}

bool WindowHandler::TouchHandler::isZoomGestureInProgress() const {
	return m_firstFingerActive && m_secondFingerActive;
}

bool WindowHandler::TouchHandler::isPanGestureInProgress() const {
	return m_firstFingerActive;
}
