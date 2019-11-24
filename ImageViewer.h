#pragma once

#include <opencv2/core.hpp>
#include <QtWidgets>

#include "VectorGraphic.h"

class ImageViewer : public QLabel
{
	Q_OBJECT

public:

	ImageViewer(QWidget* parent = nullptr);

	void setGraphic(VectorGraphic& vg);
	void setMat(cv::Mat mat);

private:
	typedef struct MouseAction {
		bool active = false;
		QPoint startPos;
	};

	QPoint position;
	float scaleFactor = 2;


	VectorGraphic vectorGraphic;

	void ShowMat();

	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void wheelEvent(QWheelEvent * event);

	MouseAction grab;
	MouseAction pointPreview;
	MouseAction lineConnect;

	cv::Mat source;
	cv::Mat display;

	
};