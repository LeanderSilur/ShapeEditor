
#include "imageviewer.h"

#include "Polyline.h"
#include "Polyshape.h"
#include "InputDialog.h"

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
	setMaximumSize(560, 720);

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
	//ShowMat();
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

	QObject::connect(se.bi_examine,
		cl, this, &ImageViewer::ctExamine);
	QObject::connect(se.bi_split,
		cl, this, &ImageViewer::ctSplit);
	QObject::connect(se.bi_connect,
		cl, this, &ImageViewer::ctConnect);
	QObject::connect(se.bi_delete,
		cl, this, &ImageViewer::ctDelete);
	interactionButtons[InteractionMode::Examine] = se.bi_examine;
	interactionButtons[InteractionMode::Split] = se.bi_split;
	interactionButtons[InteractionMode::Connect] = se.bi_connect;
	interactionButtons[InteractionMode::Delete] = se.bi_delete;


}

bool ImageViewer::ClosestLinePoint(VE::Point& closest, VE::PolylinePtr& element)
{
	// Get the closest element and point to the mouse.
	element = nullptr;
	VE::Point mousePos = VEMousePosition();
	transform.applyInv(mousePos);

	float maxDist = HIGHLIGHT_DISTANCE / transform.scale;
	float maxDist2 = maxDist * maxDist;
	float distance2 = maxDist2;

	vectorGraphic.ClosestPolyline(
		display, transform, distance2,
		mousePos, closest, element);

	return element != nullptr;
}

void ImageViewer::DrawHighlight()
{
	VE::Point closestPt;
	VE::PolylinePtr element;

	if (ClosestLinePoint(closestPt, element)) {
		element->Draw(display, transform, true, false);
	}
}

void ImageViewer::DrawHighlightPoints()
{
	VE::Point closestPt;
	VE::PolylinePtr element;

	if (ClosestLinePoint(closestPt, element)) {
		transform.apply(closestPt);

		cv::circle(display, closestPt, 4, cv::Scalar(100, 255, 150), 2, cv::LINE_AA);
		element->Draw(display, transform, true, true);
	}
}

void ImageViewer::DrawConnect()
{
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

	std::cout << (int)mode << " mode\n";

	if (interactionDraw != nullptr)
		(this->*interactionDraw)();

	// Finally, put the cv::Mat (image) on the QLabel.
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
	auto qmouse = QMousePosition();
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

QPoint ImageViewer::QMousePosition()
{
	return mapFromGlobal(QCursor::pos());
}

VE::Point ImageViewer::VEMousePosition()
{
	QPoint p = mapFromGlobal(QCursor::pos());
	return VE::Point(p.x(), p.y());
}

bool ImageViewer::CtrlPressed()
{
	return QGuiApplication::keyboardModifiers() == Qt::ControlModifier;
}

bool ImageViewer::ShiftPressed()
{
	return QGuiApplication::keyboardModifiers() == Qt::ShiftModifier;
}

void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	// TODO differentiate this
	mouseDown = event->pos();

	if (event->button() == Qt::MiddleButton) {
	}

	if (event->button() == Qt::LeftButton) {
	}
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() == Qt::MiddleButton) {
		transform.x += event->pos().x() - mouseDown.x();
		transform.y += event->pos().y() - mouseDown.y();
		mouseDown = event->pos();
	}

	mouseDown = event->pos();
	ShowMat();
}

void ImageViewer::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::MiddleButton) {

	}
	else if (event->button() == Qt::LeftButton) {

	}
}

void ImageViewer::wheelEvent(QWheelEvent* event) {
	// Do a scroll.
	float scrollFactor = 1.2;
	if (static_cast<float>(event->angleDelta().y()) > 0)
		scrollFactor = 1 / scrollFactor;

	transform.scale /= scrollFactor;
	transform.x = event->pos().x() + (transform.x - event->pos().x()) / scrollFactor;
	transform.y = event->pos().y() + (transform.y - event->pos().y()) / scrollFactor;

	ShowMat();
}
void ImageViewer::resizeEvent(QResizeEvent* event)
{
	display = cv::Mat(cv::Size(width(), height()), CV_8UC3);
}
void ImageViewer::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_F) FrameSelected();
	else if (event->key() == Qt::Key_A) FrameAll();
	else if (event->key() == Qt::Key_Home) FrameTrue();

	else if (event->key() == Qt::Key_Q) SnapEndpoints(true);
	else if (event->key() == Qt::Key_W) RemoveOverlaps(true);
	else if (event->key() == Qt::Key_E) MergeConnected(true);
	else if (event->key() == Qt::Key_R) Simplify(true);
	else if (event->key() == Qt::Key_T) Smooth(true);
	else if (event->key() == Qt::Key_L) BasicCleanup(true);

	else if (event->key() == Qt::Key_D) ComputeConnectionStatus(true);
	else if (event->key() == Qt::Key_Delete ||
		event->key() == Qt::Key_Backspace) RemoveUnusedConnections(true);
	else if (event->key() == Qt::Key_S) CalcShapes(true);


	else if (event->key() == Qt::Key_1) interactionButtons[InteractionMode::Examine]->click();
	else if (event->key() == Qt::Key_2) interactionButtons[InteractionMode::Split]->click();
	else if (event->key() == Qt::Key_3) interactionButtons[InteractionMode::Connect]->click();
	else if (event->key() == Qt::Key_4) interactionButtons[InteractionMode::Delete]->click();
}


void ImageViewer::SnapEndpoints(bool checked)
{
	vectorGraphic.SnapEndpoints();
	ShowMat();
}

void ImageViewer::RemoveOverlaps(bool checked)
{
	vectorGraphic.RemoveOverlaps();
	ShowMat();
}

void ImageViewer::MergeConnected(bool checked)
{
	if (CtrlPressed()) {
		float& angle = vectorGraphic.MIN_MERGE_ANGLE;
		angle *= 180 / CV_PI;

		InputDialog d;
		d.AddItem(angle, "Minimum Angle In Degrees", 0, 180);
		d.exec();

		angle /= 180 / CV_PI;
	}

	vectorGraphic.MergeConnected();
	ShowMat();
}

void ImageViewer::Simplify(bool checked)
{
	if (CtrlPressed()) {
		InputDialog d;
		d.AddItem(vectorGraphic.SIMPLIFY_MAX_DIST, "Maximum Merge Distance", 0, 500);
		d.exec();
	}

	for (VE::PolylinePtr& p : vectorGraphic.Polylines) {
		p->Simplify(vectorGraphic.SIMPLIFY_MAX_DIST);
		p->Cleanup();
	}

	ShowMat();
}

void ImageViewer::Smooth(bool checked)
{
	if (CtrlPressed()) {
		InputDialog d;
		d.AddItem(vectorGraphic.SMOOTHING_ITERATIONS, "Iterations", 0, 100);
		d.AddItem(vectorGraphic.SMOOTHING_LAMBDA, "Lambda", 0.f, 1.f);
		d.exec();
	}

	for (VE::PolylinePtr& p : vectorGraphic.Polylines) {
		p->Smooth(vectorGraphic.SMOOTHING_ITERATIONS, vectorGraphic.SMOOTHING_LAMBDA);
		p->Cleanup();
	}

	ShowMat();
}

void ImageViewer::BasicCleanup(bool checked)
{
	vectorGraphic.SnapEndpoints();
	vectorGraphic.RemoveOverlaps();
	vectorGraphic.MergeConnected();


	for (VE::PolylinePtr& p : vectorGraphic.Polylines) p->Simplify(vectorGraphic.SIMPLIFY_MAX_DIST);
	for (VE::PolylinePtr& p : vectorGraphic.Polylines) p->Smooth(vectorGraphic.SMOOTHING_ITERATIONS, vectorGraphic.SMOOTHING_LAMBDA);
	for (VE::PolylinePtr& p : vectorGraphic.Polylines) p->Cleanup();

	vectorGraphic.ComputeConnectionStatus();
}

void ImageViewer::ComputeConnectionStatus(bool checked)
{
	vectorGraphic.ComputeConnectionStatus();
	ShowMat();
}

void ImageViewer::RemoveUnusedConnections(bool checked)
{
	vectorGraphic.RemoveUnusedConnections();
	ShowMat();
}

void ImageViewer::CalcShapes(bool checked)
{
	vectorGraphic.CalcShapes();
	ShowMat();
}

void ImageViewer::ctExamine(bool checked)
{
	mode = InteractionMode::Examine;
	interactionDraw = &ImageViewer::DrawHighlightPoints;
}

void ImageViewer::ctSplit(bool checked)
{
	mode = InteractionMode::Split;
	interactionDraw = &ImageViewer::DrawHighlightPoints;
}

void ImageViewer::ctConnect(bool checked)
{
	mode = InteractionMode::Connect;
	interactionDraw = &ImageViewer::DrawConnect;
}

void ImageViewer::ctDelete(bool checked)
{
	mode = InteractionMode::Delete;
	interactionDraw = &ImageViewer::DrawHighlight;
}
