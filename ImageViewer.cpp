
#include "imageviewer.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>



ImageViewer::ImageViewer(QWidget * parent)
{
	if (parent == nullptr)
		this->setFixedSize(720, 720);

	setMouseTracking(true);

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
	setMat(s);
	ShowMat();
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

void ImageViewer::ShowMat()
{
	typedef cv::Point3_<uint8_t> Pixel;

	// Clear the mat.
	Pixel* px_display = display.ptr<Pixel>(0, 0);
	Pixel* end = px_display + display.cols * display.rows;
	for (; px_display != end; px_display++)
		*px_display = Pixel(100, 100, 100);

	// Reset the display mats pointer.
	px_display = display.ptr<Pixel>(0, 0);
	Pixel* px_source = source.ptr<Pixel>(0, 0);

	const int & xpos = position.x();
	const int & ypos = position.y();

	int y0 = std::max(0, ypos);
	int y1 = std::min(display.rows, static_cast<int>(ypos + std::floor(source.rows / scaleFactor)));
	int x0 = std::max(0, xpos);
	int x1 = std::min(display.cols, xpos + static_cast<int>(std::floor(source.cols / scaleFactor)));

	if (y0 >= y1 || x0 >= x1)
		return;


	for (int y = y0; y < y1; y++)
	{
		for (int x = x0; x < x1; x++)
		{
			int offsetDisp = y * display.cols + x;
			int offsetSource = std::floor((y - ypos) * scaleFactor) * source.cols + std::floor((x - xpos) * scaleFactor);
			*(px_display + offsetDisp) = *(px_source + offsetSource);
		}
	}

	// Draw the vector elements.
	VE::Transform2D t;
	t.scale = 1/scaleFactor;
	t.x = xpos;
	t.y = ypos;

	vectorGraphic.Draw(display, t);
	if (pointPreview.active) {
		// highlight closest
		VE::Point result;
		std::shared_ptr<VE::VectorElement> element;
		VE::Point mousePos(pointPreview.startPos.x(),pointPreview.startPos.y());
		VE::transformInv(mousePos,t);
		
		double distance = 10000;
		vectorGraphic.ClosestPoint(
			display, t, distance,
			mousePos, result, element);
		VE::transform(result, t);
		if (distance < 10000) {
			cv::circle(display, result, 4, cv::Scalar(100, 255, 150), 2, cv::LINE_AA);
			element->Draw(display, t, true);
		}
	}


	QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGB888);

	this->setPixmap(QPixmap::fromImage(qimg));
}

void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::MiddleButton && !lineConnect.active) {
		grab.active = true;
		grab.startPos = event->pos();
	}

	if (event->button() == Qt::LeftButton && !grab.active) {
		lineConnect.active = true;
		lineConnect.startPos = event->pos();
	}
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
	if (grab.active) {
		position += event->pos() - grab.startPos;
		grab.startPos = event->pos();
	}
	
	if (event->buttons() == 0) {
		pointPreview.active = true;
		pointPreview.startPos = event->pos();
	}
	else {
		pointPreview.active = false;
	}
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
		double scrollFactor = 1.2;
		if (static_cast<double>(event->angleDelta().y()) > 0)
			scrollFactor = 1 / scrollFactor;

		scaleFactor *= scrollFactor;
		position = event->pos() + (position - event->pos()) / scrollFactor;
		ShowMat();
	}
}

