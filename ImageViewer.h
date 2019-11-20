#pragma once

#include <opencv2/core.hpp>
#include <QtWidgets>

#include "VectorGraphic.h"

class ImageViewer : public QLabel
{
	typedef struct MouseAction {
		bool active = false;
		QPoint startPos;
	};

	Q_OBJECT

public:

	ImageViewer(QWidget *parent = nullptr);

	void setGraphic(VectorGraphic & vg) { vectorGraphic = VectorGraphic(vg); }
	void setMat(cv::Mat mat);

private:

	QPoint position;
	double scaleFactor = 2;


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