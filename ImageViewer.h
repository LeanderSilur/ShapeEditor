#pragma once

#include <opencv2/core.hpp>
#include <QtWidgets>

#include "VectorGraphic.h"

class ImageViewer : public QLabel
{
	Q_OBJECT

public:
	const float HIGHLIGHT_DISTANCE = 256;

	ImageViewer(QWidget* parent = nullptr);

	void setGraphic(VectorGraphic& vg);
	void setMat(cv::Mat mat);

private:
	typedef struct MouseAction {
		bool active = false;
		QPoint startPos;
	};

	//QPoint position;
	//float scaleFactor = 2;
	// If transform.scale is 2, then the image is shown twice as large.
	VE::Transform transform;


	VectorGraphic vectorGraphic;

	void ShowMat();
	void Frame(const VE::Bounds& bounds);
	void FrameSelected();
	void FrameAll();
	void FrameTrue();

	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void wheelEvent(QWheelEvent* event);

	MouseAction grab;
	MouseAction pointPreview;
	MouseAction lineConnect;

	cv::Mat source;
	cv::Mat display;



protected:
	void keyPressEvent(QKeyEvent* event);
	
};