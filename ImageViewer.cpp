
#include "imageviewer.h"
#include "Polyline.h"
#include "Polyshape.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>


ImageViewer::ImageViewer(QWidget* parent)
	: QLabel(nullptr)
{
	parent->layout()->addWidget(this);

	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setMinimumSize(200, 200);
	setMaximumSize(720, 300);

	setMouseTracking(true);
	
	// width(), height()
	display = cv::Mat(cv::Size(width(), height()), CV_8UC3);

	 //todo resizing
	cv::Mat s = cv::Mat(cv::Size(width(), height()), CV_8UC3);
	struct RGB {
		uchar blue;
		uchar green;
		uchar red;
	};

	for (uint y = 0; y < s.rows; y++)
		for (uint x = 0; x < s.cols; x++)
		{
			RGB& rgb = s.ptr<RGB>(y)[x];
			if ((y % 100 < 50) == (x % 100 < 50))
				rgb.green = 190;
			else
				rgb.green = 200;
			rgb.green += ((x + y) % 2) * 20;
			rgb.blue = rgb.red = rgb.green;
		}
	transform.scale = 1;
	transform.x = 0;
	transform.y = 0;
	setMat(s);
	ShowMat();
}

void ImageViewer::setGraphic(VectorGraphic& vg)
{
	vectorGraphic.Polylines = vg.Polylines;
	vectorGraphic.Polyshapes = vg.Polyshapes;
}

void ImageViewer::setMat(cv::Mat img)
{
	if (img.empty()) {
		std::cout << "Error: Image is empty.";
		return;
	}
	cv::cvtColor(img, img, CV_BGR2RGB);
	this->source = img;

}

void ImageViewer::ConnectUi(Ui_ShapeEditor& se)
{
	auto cl = &QPushButton::clicked;

	QObject::connect(se.bsnap,
		cl, this, &ImageViewer::SnapEndpoints);
	QObject::connect(se.boverlap,
		cl, this, &ImageViewer::RemoveOverlaps);
	QObject::connect(se.bmergeConnections,
		cl, this, &ImageViewer::MergeConnected);
	QObject::connect(se.bsimplify,
		cl, this, &ImageViewer::Simplify);
	QObject::connect(se.bsmooth,
		cl, this, &ImageViewer::Smooth);
	QObject::connect(se.bbasicCleanup,
		cl, this, &ImageViewer::BasicCleanup);


	QObject::connect(se.bupdateStat,
		cl, this, &ImageViewer::ComputeConnectionStatus);
	QObject::connect(se.bremoveUnused,
		cl, this, &ImageViewer::RemoveUnusedConnections);
	QObject::connect(se.bcalcShapes,
		cl, this, &ImageViewer::CalcShapes);

	
}

void ImageViewer::ShowMat()
{
	typedef cv::Point3_<uint8_t> Pixel;
	const float visibility = 0.6;
	Pixel baseColor(255, 255, 255);
	baseColor *= (1 - visibility);

	// Clear the mat.
	Pixel* px_display = display.ptr<Pixel>(0, 0);
	Pixel* end = px_display + display.cols * display.rows;
	for (; px_display != end; px_display++)
		*px_display = Pixel(80, 90, 100);

	// Reset the display mats pointer.
	px_display = display.ptr<Pixel>(0, 0);
	Pixel* px_source = source.ptr<Pixel>(0, 0);

	const int & xpos = transform.x;
	const int & ypos = transform.y;

	int y0 = std::max(0, ypos);
	int y1 = std::min(display.rows, ypos + static_cast<int>(std::floorf(source.rows * transform.scale)));
	int x0 = std::max(0, xpos);
	int x1 = std::min(display.cols, xpos + static_cast<int>(std::floorf(source.cols * transform.scale)));

	if (y0 >= y1 || x0 >= x1)
		return;

	for (int y = y0; y < y1; y++)
	{
		for (int x = x0; x < x1; x++)
		{
			int offsetDisp = y * display.cols + x;
			int offsetSource = std::floor((y - ypos) / transform.scale) * source.cols + std::floor((x - xpos) / transform.scale);
			*(px_display + offsetDisp) = baseColor + *(px_source + offsetSource) * visibility;
		}
	}

	// Draw the vector elements.
	vectorGraphic.Draw(display, transform);


	if (mode == InteractionMode::Examine) {
		// highlight closest
		VE::Point result;
		VE::PolylinePtr element;
		QPoint p = MousePosition();
		VE::Point mousePos(p.x(), p.y());
		transform.applyInv(mousePos);
		
		float distance = HIGHLIGHT_DISTANCE / transform.scale;
		vectorGraphic.ClosestPolyline(
			display, transform, distance,
			mousePos, result, element);
		if (element != nullptr) {
			transform.apply(result);
			if (distance < VE::FMAX) {
				cv::circle(display, result, 4, cv::Scalar(100, 255, 150), 2, cv::LINE_AA);
				element->Draw(display, transform, true);
			}

			auto it = std::find(vectorGraphic.Polylines.begin(), vectorGraphic.Polylines.end(), element);
			int elIndex = std::distance(vectorGraphic.Polylines.begin(), it);
			VE::Point res(result);
			transform.applyInv(res);

		}
		
	}


	QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGB888);

	this->setPixmap(QPixmap::fromImage(qimg));
}

void ImageViewer::Frame(const VE::Bounds& bounds)
{
	float x_scale = display.cols / (bounds.x1 - bounds.x0);
	float y_scale = display.rows / (bounds.y1 - bounds.y0);

	transform.scale = std::min(x_scale, y_scale);
	transform.x = -transform.scale * bounds.x0;
	transform.y = -transform.scale * bounds.y0;
	ShowMat();
}

void ImageViewer::FrameSelected()
{
	// highlight closest
	VE::Point result;
	VE::PolylinePtr element;
	auto qmouse = MousePosition();
	VE::Point mousePos(qmouse.x(), qmouse.y());
	transform.applyInv(mousePos);

	float distance = HIGHLIGHT_DISTANCE / transform.scale;
	vectorGraphic.ClosestPolyline(
		display, transform, distance,
		mousePos, result, element);
	if (element != nullptr) {
		Frame(element->getBounds());
	}
}

void ImageViewer::FrameAll()
{
	if (source.empty())
		return;
	VE::Bounds bounds;
	bounds.x0 = 0;
	bounds.y0 = 0;
	bounds.x1 = source.cols;
	bounds.y1 = source.rows;
	Frame(bounds);
}

void ImageViewer::FrameTrue()
{
	transform.scale = 1;
	transform.x = 0;
	transform.y = 0;
}

QPoint ImageViewer::MousePosition()
{
	return mapFromGlobal(QCursor::pos());
}

void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	// TODO differentiate this
	if (event->button() == Qt::MiddleButton) {
		mouseDown = event->pos();
	}

	if (event->button() == Qt::LeftButton) {
		mouseDown = event->pos();
	}
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
	if (event->button() == Qt::MiddleButton) {
		transform.x += event->pos().x() - mouseDown.x();
		transform.y += event->pos().y() - mouseDown.y();
		mouseDown = event->pos();
	}
	

	mouseDown = event->pos();
	ShowMat();
}

void ImageViewer::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::MiddleButton)
		grab.active = false;
}
void ImageViewer::wheelEvent(QWheelEvent* event) {
	if (grab.active == false) {
		// Do a scroll.
		float scrollFactor = 1.2;
		if (static_cast<float>(event->angleDelta().y()) > 0)
			scrollFactor = 1 / scrollFactor;

		transform.scale /= scrollFactor;
		transform.x = event->pos().x() + (transform.x - event->pos().x()) / scrollFactor;
		transform.y = event->pos().y() + (transform.y - event->pos().y()) / scrollFactor;

		ShowMat();
	}
}
void ImageViewer::resizeEvent(QResizeEvent* event)
{
	display = cv::Mat(cv::Size(width(), height()), CV_8UC3);
}
void ImageViewer::keyPressEvent(QKeyEvent* event)
{
	std::cout << "keydown \n";
	if (event->key() == Qt::Key_F) {
		FrameSelected();
	}
	if (event->key() == Qt::Key_A) {
		FrameAll();
	}
	if (event->key() == Qt::Key_Home) {
		FrameTrue();
	}
}


void ImageViewer::SnapEndpoints(bool checked)
{
	vectorGraphic.SnapEndpoints();
	ShowMat();
}

void ImageViewer::RemoveOverlaps(bool checked)
{
}

void ImageViewer::MergeConnected(bool checked)
{
}

void ImageViewer::Simplify(bool checked)
{
}

void ImageViewer::Smooth(bool checked)
{
}

void ImageViewer::BasicCleanup(bool checked)
{
}

void ImageViewer::ComputeConnectionStatus(bool checked)
{
}

void ImageViewer::RemoveUnusedConnections(bool checked)
{
}

void ImageViewer::CalcShapes(bool checked)
{
}
