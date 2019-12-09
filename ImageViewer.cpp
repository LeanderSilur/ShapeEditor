
#include "imageviewer.h"

#include "Polyline.h"
#include "Polyshape.h"
#include "InputDialog.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem>



ImageViewer::ImageViewer(QWidget* parent)
	: QLabel(nullptr)
{
	parent->layout()->addWidget(this);

	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setMinimumSize(200, 200);
	setMaximumSize(560, 720);

	setMouseTracking(true);
	setAcceptDrops(true);
	
	// width(), height()
	display = cv::Mat(cv::Size(width(), height()), CV_8UC3);
	
	transform.Reset();

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
	Animation::Frame frame(s, VectorGraphic());
	frames.push_back(frame);

}

void ImageViewer::AddFrame(Animation::Frame& frame)
{
	frames.push_back(frame);
}

void ImageViewer::ConnectUi(Ui_ShapeEditor& se)
{
	auto cl = &QPushButton::clicked;

	QObject::connect(se.bFileLoad,
		cl, this, &ImageViewer::FileLoad);
	QObject::connect(se.bFileSaveLines,
		cl, this, &ImageViewer::FileSave);
	QObject::connect(se.bFramePrev,
		cl, this, &ImageViewer::PrevFrame);
	QObject::connect(se.bFrameNext,
		cl, this, &ImageViewer::NextFrame);

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



	interactionButtons[InteractionMode::Examine] = se.bi_examine;
	interactionButtons[InteractionMode::Split] = se.bi_split;
	interactionButtons[InteractionMode::Connect] = se.bi_connect;
	interactionButtons[InteractionMode::Delete] = se.bi_delete;
	interactionButtons[InteractionMode::ShapeColor] = se.bi_shapeColor;
	interactionButtons[InteractionMode::ShapeDelete] = se.bi_shapeDelete;

	QObject::connect(interactionButtons[InteractionMode::Examine],
		cl, this, &ImageViewer::ctExamine);
	QObject::connect(interactionButtons[InteractionMode::Split],
		cl, this, &ImageViewer::ctSplit);
	QObject::connect(interactionButtons[InteractionMode::Connect],
		cl, this, &ImageViewer::ctConnect);
	QObject::connect(interactionButtons[InteractionMode::Delete],
		cl, this, &ImageViewer::ctDelete);
	QObject::connect(interactionButtons[InteractionMode::ShapeColor],
		cl, this, &ImageViewer::ctShapeColor);
	QObject::connect(interactionButtons[InteractionMode::ShapeDelete],
		cl, this, &ImageViewer::ctShapeDelete);



	interactionButtons[InteractionMode::Examine]->click();
}

bool ImageViewer::ClosestLinePoint(VE::Point& closest, VE::PolylinePtr& element)
{
	// Get the closest element and point to the mouse.
	element = nullptr;
	std::cout << "Getting closest.\n";
	VE::Point mousePos = VEMousePosition();
	transform.applyInv(mousePos);

	std::cout << "mousepos.\n";
	float maxDist = HIGHLIGHT_DISTANCE / transform.scale;
	float maxDist2 = maxDist * maxDist;
	float distance2 = maxDist2;

	std::cout << "closest.\n";

	vectorGraphic().ClosestPolyline(
		display, transform, distance2,
		mousePos, closest, element);

	std::cout << "ret.\n";
	return element != nullptr;
}

void ImageViewer::DrawHighlight(const cv::Scalar & color)
{
	VE::Point closestPt;
	VE::PolylinePtr element;

	if (ClosestLinePoint(closestPt, element)) {
		element->Draw(display, transform, &color, false);
	}
}

void ImageViewer::DrawHighlightPoints(const cv::Scalar & color)
{
	std::cout << "draw highlgih points\n";
	VE::Point closestPt;
	VE::PolylinePtr element;

	if (ClosestLinePoint(closestPt, element)) {
		std::cout << "got closest\n";
		transform.apply(closestPt);
		std::cout << "    transformed\n";

		cv::circle(display, closestPt, 4, cv::Scalar(100, 255, 150), 2, cv::LINE_AA);
		std::cout << "    circle\n";
		element->Draw(display, transform, &color, true);
		std::cout << "    el-Draw()\n";
	}
}

void ImageViewer::DrawExamine()
{
	std::cout << "eaxmining\n";
	DrawHighlightPoints(POLYLINE_EXAMINE);
	std::cout << "finished examining\n";
}

void ImageViewer::DrawSplit()
{
	DrawHighlightPoints(POLYLINE_SPLIT);
}

bool GetClosestEndPoint(const float& maxDist2, VE::Bounds &bounds, VectorGraphic& vg, VE::Point& pos)
{
	float distance2 = maxDist2;
	VE::Point origin = pos;
	vg.ClosestEndPoint(bounds, distance2, origin, pos);

	return distance2 < maxDist2;
}

void ImageViewer::DrawConnect()
{
	VE::Bounds bounds(0, 0, display.cols, display.rows);
	transform.applyInv(bounds);

	float maxDist = HIGHLIGHT_DISTANCE / transform.scale,
		maxDist2_1 = maxDist * maxDist,
		maxDist2_2 = maxDist2_1;

	VE::Point pos1 = VE::Point(mousePressPos.x(), mousePressPos.y());
	VE::Point pos2 = VEMousePosition();

	transform.applyInv(pos1);
	transform.applyInv(pos2);

	VE::Point pos2_orig = pos2;

	bool pos1_valid = GetClosestEndPoint(maxDist2_1, bounds, vectorGraphic(), pos1);
	if (lmbHold && !pos1_valid)
		return;
	bool pos2_valid = GetClosestEndPoint(maxDist2_2, bounds, vectorGraphic(), pos2);
	if (lmbHold && pos1 == pos2) {
		pos2_valid = false;
		pos2 = pos2_orig;
	}

	transform.apply(pos1);
	transform.apply(pos2);

	if (lmbHold && pos1_valid) {
		cv::line(display, pos1, pos2, POLYLINE_CONNECT_LINE, 2, cv::LINE_AA);
		cv::circle(display, pos1, 4, POLYLINE_CONNECT1, cv::FILLED);
	}
	if (pos2_valid) {
		cv::Scalar pos2Color = POLYLINE_CONNECT1;
		if (lmbHold)
			pos2Color = POLYLINE_CONNECT2;
		cv::circle(display, pos2, 6, pos2Color, cv::FILLED);
	}
}

void ImageViewer::DrawDelete()
{
	DrawHighlight(POLYLINE_DELETE);
}

void ImageViewer::ReleaseSplit(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	VE::Point closestPt;
	VE::PolylinePtr element;

	if (ClosestLinePoint(closestPt, element)) {
		vectorGraphic().Split(element, closestPt);
		ShowMat();
	}
}

void ImageViewer::ReleaseConnect(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	VE::Bounds bounds(0, 0, display.cols, display.rows);
	transform.applyInv(bounds);

	float maxDist = HIGHLIGHT_DISTANCE / transform.scale,
		maxDist2_1 = maxDist * maxDist,
		maxDist2_2 = maxDist2_1;

	VE::Point pos1 = VE::Point(mousePressPos.x(), mousePressPos.y());
	VE::Point pos2 = VEMousePosition();

	transform.applyInv(pos1);
	transform.applyInv(pos2);

	bool pos1_valid = GetClosestEndPoint(maxDist2_1, bounds, vectorGraphic(), pos1);
	bool pos2_valid = GetClosestEndPoint(maxDist2_2, bounds, vectorGraphic(), pos2);
	if (!pos1_valid || !pos2_valid || pos1 == pos2) {
		std::cout << "Connect failed.\n";
	}
	else {
		std::vector<VE::Point> points = { pos1, pos2 };
		vectorGraphic().AddPolyline(points);
	}
	ShowMat();
}

void ImageViewer::ReleaseDelete(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	VE::Point closestPt;
	VE::PolylinePtr element;
	if (ClosestLinePoint(closestPt, element)) {
		vectorGraphic().Delete(element);
		ShowMat();
	}
}

// LMB		=> Color/Create
// Ctrl LMB	=> Set Color
// RMB		=> Pick Color
void ImageViewer::ReleaseShapeColor(QMouseEvent* event)
{
	VE::Point pt = VEMousePosition();
	transform.applyInv(pt);

	if (event->button() == Qt::LeftButton) {
		bool result = true;
		if (CtrlPressed() || ShiftPressed()) {
			if (CtrlPressed())
				vectorGraphic().ActiveColor = std::make_shared<VE::ColorArea>(*vectorGraphic().ActiveColor);
			auto& col = vectorGraphic().ActiveColor->Color;
			int r = col[0],
				g = col[1],
				b = col[2];

			InputDialog d;
			d.AddItem(vectorGraphic().ActiveColor->Name, "Name");
			d.AddItem(r, "R", 0, 255);
			d.AddItem(g, "G", 0, 255);
			d.AddItem(b, "B", 0, 255);

			result = d.exec();
			col[0] = r;
			col[1] = g;
			col[2] = b;
		}
		if (result) {
			vectorGraphic().ColorShape(pt);
			ShowMat();
		}
	}
	if (event->button() == Qt::RightButton) {
		vectorGraphic().PickColor(pt);
	}
}

void ImageViewer::ReleaseShapeDelete(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	VE::Point pt(event->x(), event->y());
	transform.applyInv(pt);

	if (vectorGraphic().DeleteShape(pt))
		ShowMat();
}

void ImageViewer::ShowMat()
{
	std::cout << "gonna show\n";
	typedef cv::Point3_<uint8_t> Pixel;
	const float visibility = 0.20;
	Pixel baseColor(80, 80, 80);
	baseColor *= (1 - visibility);

	// Clear the mat.
	Pixel* px_display = display.ptr<Pixel>(0, 0);
	Pixel* end = px_display + display.cols * display.rows;
	for (; px_display != end; px_display++)
		*px_display = Pixel(80, 90, 100);

	// Reset the display mats pointer.
	px_display = display.ptr<Pixel>(0, 0);
	Pixel* px_source = sourceImage().ptr<Pixel>(0, 0);

	const int & xpos = transform.x;
	const int & ypos = transform.y;

	int y0 = std::max(0, ypos);
	int y1 = std::min(display.rows, ypos + static_cast<int>(std::floorf(sourceImage().rows * transform.scale)));
	int x0 = std::max(0, xpos);
	int x1 = std::min(display.cols, xpos + static_cast<int>(std::floorf(sourceImage().cols * transform.scale)));

	if (y0 >= y1 || x0 >= x1)
		return;

	for (int y = y0; y < y1; y++)
	{
		for (int x = x0; x < x1; x++)
		{
			int offsetDisp = y * display.cols + x;
			int offsetSource = std::floor((y - ypos) / transform.scale) * sourceImage().cols + std::floor((x - xpos) / transform.scale);
			*(px_display + offsetDisp) = baseColor + *(px_source + offsetSource) * visibility;
		}
	}

	// Draw the vector elements.
	vectorGraphic().Draw(display, transform);

	std::cout << "here2\n";

	if (interactionDraw != nullptr) {
		std::cout << "not a nullptr\n";
		(this->*interactionDraw)();
	}
	std::cout << "interactionDraw\n";

	// Finally, put the cv::Mat (image) on the QLabel.
	QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGB888);
	this->setPixmap(QPixmap::fromImage(qimg));
	std::cout << "done\n";
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
	vectorGraphic().ClosestPolyline(
		display, transform, distance,
		mousePos, result, element);
	if (element != nullptr) {
		Frame(element->getBounds());
	}
}

void ImageViewer::FrameAll()
{
	if (sourceImage().empty())
		return;
	VE::Bounds bounds;
	bounds.x0 = 0;
	bounds.y0 = 0;
	bounds.x1 = sourceImage().cols;
	bounds.y1 = sourceImage().rows;
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
	if (event->button() == Qt::LeftButton) {
		mousePressPos = event->pos();
		mousePrevPos = event->pos();
		lmbHold = true;
	}
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
	// Movement.
	if (event->buttons() == Qt::MiddleButton) {
		transform.x += event->pos().x() - mousePrevPos.x();
		transform.y += event->pos().y() - mousePrevPos.y();
		mousePrevPos = event->pos();
	}

	mousePrevPos = event->pos();
	ShowMat();
}

void ImageViewer::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
		lmbHold = false;

	if (interactionRelease != nullptr)
		(this->*interactionRelease)(event);
}

void ImageViewer::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void ImageViewer::dragMoveEvent(QDragMoveEvent* e)
{
	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void ImageViewer::dropEvent(QDropEvent* e)
{
	foreach(const QUrl & url, e->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		qDebug() << "Dropped file:" << fileName;
		Animation::Frame frame;
		if (frame.Load(fileName.toStdString())) {
			frames.push_back(frame);
			activeFrame = frames.size() - 1;
			ShowMat();
		}
		break;
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

	else if (event->key() == Qt::Key_O
		&& event->modifiers() == Qt::Modifier::CTRL) FileLoad(true);
	else if (event->key() == Qt::Key_S
		&& event->modifiers() == Qt::Modifier::CTRL) FileSave(true);
	else if (event->key() == Qt::Key_Right) NextFrame(true);
	else if (event->key() == Qt::Key_Left) PrevFrame(true);


	else if (event->key() == Qt::Key_1) interactionButtons[InteractionMode::Examine]->click();
	else if (event->key() == Qt::Key_2) interactionButtons[InteractionMode::Split]->click();
	else if (event->key() == Qt::Key_3) interactionButtons[InteractionMode::Connect]->click();
	else if (event->key() == Qt::Key_4) interactionButtons[InteractionMode::Delete]->click();
	else if (event->key() == Qt::Key_5) interactionButtons[InteractionMode::ShapeColor]->click();
	else if (event->key() == Qt::Key_6) interactionButtons[InteractionMode::ShapeDelete]->click();
}


void ImageViewer::SnapEndpoints(bool checked)
{
	vectorGraphic().SnapEndpoints();
	ShowMat();
}

void ImageViewer::RemoveOverlaps(bool checked)
{
	vectorGraphic().RemoveOverlaps();
	ShowMat();
}

void ImageViewer::MergeConnected(bool checked)
{
	bool result = true;

	if (CtrlPressed()) {
		float& angle = vectorGraphic().MIN_MERGE_ANGLE;
		angle *= 180 / CV_PI;

		InputDialog d;
		d.AddItem(angle, "Minimum Angle In Degrees", 0, 180);
		result = d.exec();

		angle /= 180 / CV_PI;
	}

	if (result) {
		vectorGraphic().MergeConnected();
		ShowMat();
	}
	else {
		std::cout << "Cancelled Merge.\n";
	}
}

void ImageViewer::Simplify(bool checked)
{
	bool result = true;

	if (CtrlPressed()) {
		InputDialog d;
		d.AddItem(vectorGraphic().SIMPLIFY_MAX_DIST, "Maximum Merge Distance", 0, 500);
		result = d.exec();
	}

	if (result) {
		for (VE::PolylinePtr& p : vectorGraphic().Polylines) {
			p->Simplify(vectorGraphic().SIMPLIFY_MAX_DIST);
			p->Cleanup();
		}
		ShowMat();
	}
	else {
		std::cout << "Cancelled Simplify.\n";
	}
}

void ImageViewer::Smooth(bool checked)
{
	bool result = true;

	if (CtrlPressed()) {
		InputDialog d;
		d.AddItem(vectorGraphic().SMOOTHING_ITERATIONS, "Iterations", 0, 100);
		d.AddItem(vectorGraphic().SMOOTHING_LAMBDA, "Lambda", 0.f, 1.f);
		result = d.exec();
	}

	if (result) {
		for (VE::PolylinePtr& p : vectorGraphic().Polylines) {
			p->Smooth(vectorGraphic().SMOOTHING_ITERATIONS, vectorGraphic().SMOOTHING_LAMBDA);
			p->Cleanup();
		}
		ShowMat();
	}
	else {
		std::cout << "Cancelled Smooth.\n";
	}
}

void ImageViewer::BasicCleanup(bool checked)
{
	vectorGraphic().SnapEndpoints();
	vectorGraphic().RemoveOverlaps();
	vectorGraphic().MergeConnected();


	for (VE::PolylinePtr& p : vectorGraphic().Polylines) p->Simplify(vectorGraphic().SIMPLIFY_MAX_DIST);
	for (VE::PolylinePtr& p : vectorGraphic().Polylines) p->Smooth(vectorGraphic().SMOOTHING_ITERATIONS, vectorGraphic().SMOOTHING_LAMBDA);
	for (VE::PolylinePtr& p : vectorGraphic().Polylines) p->Cleanup();

	vectorGraphic().ComputeConnectionStatus();
}

void ImageViewer::ComputeConnectionStatus(bool checked)
{
	vectorGraphic().ComputeConnectionStatus();
	ShowMat();
}

void ImageViewer::RemoveUnusedConnections(bool checked)
{
	vectorGraphic().RemoveUnusedConnections();
	ShowMat();
}

void ImageViewer::CalcShapes(bool checked)
{
	vectorGraphic().CalcShapes();
	vectorGraphic().ColorShapesRandom();
	ShowMat();
}

void ImageViewer::NextFrame(bool checked)
{
	std::cout << "nextframe "<< activeFrame;
	if (activeFrame + 1 < frames.size())
		activeFrame++;
	ShowMat();
}

void ImageViewer::PrevFrame(bool checked)
{
	std::cout << "prevframe " << activeFrame;
	if (activeFrame - 1 >= 0)
		activeFrame--;
	ShowMat();
}

void ImageViewer::ctExamine(bool checked)
{
	mode = InteractionMode::Examine;
	interactionDraw = &ImageViewer::DrawExamine;
	interactionRelease = nullptr;
	ShowMat();
}

void ImageViewer::ctSplit(bool checked)
{
	mode = InteractionMode::Split;
	interactionDraw = &ImageViewer::DrawSplit;
	interactionRelease = &ImageViewer::ReleaseSplit;
	ShowMat();
}

void ImageViewer::ctConnect(bool checked)
{
	mode = InteractionMode::Connect;
	interactionDraw = &ImageViewer::DrawConnect;
	interactionRelease = &ImageViewer::ReleaseConnect;
	ShowMat();
}

void ImageViewer::ctDelete(bool checked)
{
	mode = InteractionMode::Delete;
	interactionDraw = &ImageViewer::DrawDelete;
	interactionRelease = &ImageViewer::ReleaseDelete;
	ShowMat();
}

void ImageViewer::ctShapeColor(bool checked)
{
	mode = InteractionMode::ShapeColor;
	interactionDraw = nullptr;
	interactionRelease = &ImageViewer::ReleaseShapeColor;
	ShowMat();
}

void ImageViewer::ctShapeDelete(bool checked)
{
	mode = InteractionMode::ShapeDelete;
	interactionDraw = nullptr;
	interactionRelease = &ImageViewer::ReleaseShapeDelete;
	ShowMat();

}

void ImageViewer::FileLoad(bool checked)
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
		QString("Choose a .svg with matching .png"), "D:/190725_sequence_colorization/files",
		QString("Lineart (*.svg)"));

	decltype(frames) newFrames;
	for (size_t i = 0; i < fileNames.size(); i++)
	{
		std::string fileName = fileNames[i].toStdString();
		Animation::Frame frame;
		if (frame.Load(fileName)) {
			newFrames.push_back(frame);
		}
	}
	if (!newFrames.empty()) {
		activeFrame = 0;
		frames = newFrames;
	}
	
}

void ImageViewer::FileSave(bool checked)
{
	bool result = true;
	if (CtrlPressed()) {
		InputDialog d;
		d.AddItem(FILE_SAVE_LINES, "Lines");
		d.AddItem(FILE_SAVE_SHAPES, "Shapes");
		result = d.exec();
	}
	if (!result) return;


	std::string directoryName = QFileDialog::getExistingDirectory(this, "Select Save Directory",
		"D:/190725_sequence_colorization/files").toStdString();
	
	auto directory = std::filesystem::path(directoryName);
	if (!std::filesystem::is_directory(directory))
		return;

	
	for (auto&frame:frames)
	{
		
		auto path = directory / frame.getName();

		if (FILE_SAVE_LINES) {
			auto pathLine = path.string() + "_lines.svg";
			vectorGraphic().SavePolylines(pathLine, "", cv::Size2i(sourceImage().cols, sourceImage().rows));
		}
		if (FILE_SAVE_SHAPES) {
			auto pathShape = path.string() + "_shapes.svg";
			vectorGraphic().SavePolyshapes(pathShape, "", cv::Size2i(sourceImage().cols, sourceImage().rows));
		}

	}

}
