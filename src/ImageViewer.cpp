
#include "imageviewer.h"

#include "Polyline.h"
#include "Polyshape.h"
#include "InputDialog.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>



ImageViewer::ImageViewer(QWidget* parent)
	: QLabel(nullptr)
{
	parent->layout()->addWidget(this);

	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setMinimumSize(200, 200);
	setMaximumSize(1024, 720);

	setMouseTracking(true);
	setAcceptDrops(true);
	
	// width(), height()
	display = cv::Mat(cv::Size(width(), height()), CV_8UC3);
	
	transform.Reset();
	ActiveColor->Color = cv::Scalar(120, 160, 140);

	WorkingDirRead();

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

void ImageViewer::ConnectUi(ShapeEditor& se)
{
	QObject::connect(&se, &ShapeEditor::keyPress, this, &ImageViewer::keyPressEvent);
	auto& seUi = se.getUi();

	auto cl = &QPushButton::clicked;
	wMainMenu = seUi.menuTabWidget;
	lInfoText = seUi.lInfoText;
	lFrameText = seUi.lFrameText;
	
	QObject::connect(seUi.bFileDirectory,
		cl, this, &ImageViewer::FileSetDirectory);
	QObject::connect(seUi.bFileLoad,
		cl, this, &ImageViewer::FileLoad);
	QObject::connect(seUi.bFileSaveLines,
		cl, this, &ImageViewer::FileSave);
	QObject::connect(seUi.bFrameReload,
		cl, this, &ImageViewer::ReloadFrame);
	QObject::connect(seUi.bFramePrev,
		cl, this, &ImageViewer::PrevFrame);
	QObject::connect(seUi.bFrameNext,
		cl, this, &ImageViewer::NextFrame);

	QObject::connect(seUi.bsnap,
		cl, this, &ImageViewer::SnapEndpoints);
	QObject::connect(seUi.boverlap,
		cl, this, &ImageViewer::RemoveOverlaps);
	QObject::connect(seUi.bmergeConnections,
		cl, this, &ImageViewer::MergeConnected);
	QObject::connect(seUi.bsimplify,
		cl, this, &ImageViewer::Simplify);
	QObject::connect(seUi.bsmooth,
		cl, this, &ImageViewer::Smooth);
	QObject::connect(seUi.bbasicCleanup,
		cl, this, &ImageViewer::BasicCleanup);


	QObject::connect(seUi.bupdateStat,
		cl, this, &ImageViewer::ComputeConnectionStatus);
	QObject::connect(seUi.bremoveUnused,
		cl, this, &ImageViewer::RemoveUnusedConnections);
	QObject::connect(seUi.bshapesCalc,
		cl, this, &ImageViewer::CalcShapes);
	QObject::connect(seUi.bshapesClear,
		cl, this, &ImageViewer::ClearShapes);



	interactionButtons[InteractionMode::Examine] = seUi.bi_examine;
	interactionButtons[InteractionMode::Split] = seUi.bi_split;
	interactionButtons[InteractionMode::Connect] = seUi.bi_connect;
	interactionButtons[InteractionMode::Delete] = seUi.bi_delete;
	interactionButtons[InteractionMode::ShapeColor] = seUi.bi_shapeColor;
	interactionButtons[InteractionMode::ShapeDelete] = seUi.bi_shapeDelete;

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

// Get the closest element and point to the target.
// Target point is given in viewport transformed space.
void ImageViewer::ClosestLinePoint(int& ptIdx, const VE::Point& target, VE::Point& closest, VE::PolylinePtr & pl, bool snapEndpoints)
{
	VE::Bounds bounds(0, 0, display.cols, display.rows);
	transform.applyInv(bounds);

	VE::Point tTarget = target;
	transform.applyInv(tTarget);

	const float
		maxDist = HIGHLIGHT_DISTANCE / transform.scale,
		maxDist2 = maxDist * maxDist,
		maxDistEnd = ENDPOINT_SNAPPING_DISTANCE / transform.scale,
		maxDistEnd2 = maxDistEnd * maxDistEnd;
	float dist2 = maxDist2;

	vectorGraphic().ClosestPolylinePoint(
		dist2, tTarget, ptIdx, closest, pl, &bounds, snapEndpoints ? maxDistEnd2 : 0.0);
}

void ImageViewer::DrawHighlight(const cv::Scalar & color)
{
	int ptIdx;
	VE::Point closestPt;
	VE::PolylinePtr pl;
	ClosestLinePoint(ptIdx, VEMousePosition(), closestPt, pl, false);

	if (ptIdx >= 0) {
		pl->Draw(display, transform, &color, false);
	}
}

void ImageViewer::DrawHighlightPoints(const cv::Scalar & color)
{
	int ptIdx;
	VE::Point closestPt;
	VE::PolylinePtr pl;
	ClosestLinePoint(ptIdx, VEMousePosition(), closestPt, pl, false);


	int plIdx = std::distance(vectorGraphic().Polylines.begin(),
		std::find(vectorGraphic().Polylines.begin(), vectorGraphic().Polylines.end(), pl));
	
	if (ptIdx >= 0) {
		std::stringstream stream;
		stream << std::fixed << std::setprecision(3) << "[" << plIdx << ", " << ptIdx << "] " << closestPt.x << ", " << closestPt.y;
		lInfoText->setText(stream.str().c_str());

		transform.apply(closestPt);

		cv::circle(display, closestPt, 4, cv::Scalar(100, 255, 150), 2, cv::LINE_AA);
		pl->Draw(display, transform, &color, true);
	}
}

void ImageViewer::DrawExamine()
{
	DrawHighlightPoints(POLYLINE_EXAMINE);
}

void ImageViewer::DrawSplit()
{
	DrawHighlightPoints(POLYLINE_SPLIT);
}

void ImageViewer::DrawConnect()
{
	VE::Bounds bounds(0, 0, display.cols, display.rows);
	transform.applyInv(bounds);


	VE::Point pos1 = VE::Point(mousePressPos.x(), mousePressPos.y());
	VE::Point pos2 = VEMousePosition();
	int ptIdx1, ptIdx2;
	VE::Point closestPt1, closestPt2;
	VE::PolylinePtr pl1, pl2;

	ClosestLinePoint(ptIdx1, pos1, closestPt1, pl1, true);
	bool pos1_valid = ptIdx1 >= 0;
	if (pos1_valid) transform.apply(closestPt1);

	if (lmbHold && !pos1_valid) return;


	ClosestLinePoint(ptIdx2, pos2, closestPt2, pl2, true);
	bool pos2_valid = ptIdx2 >= 0;
	if (pos2_valid) transform.apply(closestPt2);

	if (lmbHold && (!pos2_valid || closestPt1 == closestPt2))
		closestPt2 = pos2;

	if (lmbHold) {
		cv::line(display, closestPt1, closestPt2, POLYLINE_CONNECT_LINE, 2, cv::LINE_AA);
		if (pos1_valid)
			cv::circle(display, closestPt1, 4, POLYLINE_CONNECT1, cv::FILLED);
	}
	if (pos2_valid) {
		cv::circle(display, closestPt2, 6,
			lmbHold ? POLYLINE_CONNECT2 : POLYLINE_CONNECT1, cv::FILLED);
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

	int ptIdx;
	VE::Point closestPt;
	VE::PolylinePtr pl;
	ClosestLinePoint(ptIdx, VEMousePosition(), closestPt, pl, false);

	if (ptIdx >= 0) {
		vectorGraphic().Split(pl, ptIdx);
		ShowMat();
	}
}

void ImageViewer::ReleaseConnect(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	VE::Bounds bounds(0, 0, display.cols, display.rows);
	transform.applyInv(bounds);


	VE::Point pos1 = VE::Point(mousePressPos.x(), mousePressPos.y());
	VE::Point pos2 = VEMousePosition();
	int ptIdx1, ptIdx2;
	VE::Point closestPt1, closestPt2;
	VE::PolylinePtr pl1, pl2;

	ClosestLinePoint(ptIdx1, pos1, closestPt1, pl1, true);
	ClosestLinePoint(ptIdx2, pos2, closestPt2, pl2, true);
	if (ptIdx1 < 0 || ptIdx2 < 0)
		return;

	if (closestPt1 == closestPt2)
		return;

	// Create new Line Segment.
	std::vector<VE::Point> points = { closestPt1, closestPt2 };
	vectorGraphic().AddPolyline(points);

	if (pl1 == pl2) {
		std::vector<int> indices { ptIdx1, ptIdx2 };
		vectorGraphic().Split(pl1, indices);
	}
	else {
		vectorGraphic().Split(pl1, ptIdx1);
		vectorGraphic().Split(pl2, ptIdx2);
	}
	//ShowMat();
}

void ImageViewer::ReleaseDelete(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton)
		return;

	int ptIdx;
	VE::Point closestPt;
	VE::PolylinePtr pl;
	ClosestLinePoint(ptIdx, VEMousePosition(), closestPt, pl, false);

	if (ptIdx >= 0) {
		vectorGraphic().Delete(pl);
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
		if (AltPressed()) {
			vectorGraphic().PickColor(pt, ActiveColor);
		}
		else {
			bool result = true;
			if (CtrlPressed() || ShiftPressed()) {
				if (CtrlPressed())
					ActiveColor = std::make_shared<VE::ColorArea>(*ActiveColor);
				auto& col = ActiveColor->Color;
				int r = col[0],
					g = col[1],
					b = col[2];

				InputDialog d;
				d.AddItem(ActiveColor->Name, "Name");
				d.AddItem(r, "R", 0, 255);
				d.AddItem(g, "G", 0, 255);
				d.AddItem(b, "B", 0, 255);

				result = d.exec();
				col[0] = r;
				col[1] = g;
				col[2] = b;
			}
			if (result) {
				auto& g = vectorGraphic();
				g.ColorShape(pt, ActiveColor);
				ShowMat();
			}
		}
		lInfoText->setText((ActiveColor->Name + " ["
			+ std::to_string((int)ActiveColor->Color[0]) + ", "
			+ std::to_string((int)ActiveColor->Color[1]) + ", "
			+ std::to_string((int)ActiveColor->Color[2]) + "]").c_str());
	}
	if (event->button() == Qt::RightButton) {
		if (vectorGraphic().DeleteShape(pt))
			ShowMat();
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
	
	cv::Scalar c(255, 255, 255);
	if (vectorGraphic().Polylines.size() >= 28)
	vectorGraphic().Polylines[29]->Draw(display, transform, &c, false);

	if (interactionDraw != nullptr) {
		(this->*interactionDraw)();
	}

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
	int ptIdx;
	VE::Point result;
	VE::PolylinePtr pl;
	ClosestLinePoint(ptIdx, VEMousePosition(), result, pl, false);

	if (ptIdx >= 0) {
		Frame(pl->getBounds());
	}
}

void ImageViewer::FrameAll()
{
	if (vectorGraphic().Polylines.empty()
		|| sourceImage().empty())
		return;
	
	VE::Bounds bounds = vectorGraphic().getBounds();
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

bool ImageViewer::AltPressed()
{
	return QGuiApplication::keyboardModifiers() == Qt::AltModifier;
}

void ImageViewer::WorkingDirSave()
{
	std::ofstream directoryFile;
	directoryFile.open("directory");
	directoryFile << WORKING_DIRECTORY;
	directoryFile.close();
}

void ImageViewer::WorkingDirRead()
{
	if (std::filesystem::exists(std::filesystem::path("directory"))) {
		std::ifstream directoryFile("directory");
		if (directoryFile.is_open())
		{
			WORKING_DIRECTORY = std::string((std::istreambuf_iterator<char>(directoryFile)),
				std::istreambuf_iterator<char>());

			directoryFile.close();
		}
	}
}

void ImageViewer::mousePressEvent(QMouseEvent * event)
{
	// TODO differentiate this
	if (event->button() == Qt::LeftButton) {

		mousePressPos = event->pos();
		mousePrevPos = event->pos();
		lmbHold = true;
	}
	else if (event->button() == Qt::ForwardButton) {
		NextFrame(true);
	}
	else if (event->button() == Qt::BackButton) {
		PrevFrame(true);
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
			frameIndex = frames.size() - 1;
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
	if (event->modifiers() == Qt::Modifier::CTRL &&
		event->key() == Qt::Key_O) FileLoad(true);
	else if (event->modifiers() == Qt::Modifier::CTRL &&
		event->key() == Qt::Key_S) FileSave(true);
	else if (event->modifiers() == Qt::Modifier::CTRL &&
		event->key() == Qt::Key_S) FileSave(true);
	else if (event->modifiers() == Qt::Modifier::ALT && 
		event->key() == Qt::Key_S) ClearShapes(true);
	else if (event->key() == Qt::Key_S) CalcShapes(true);

	else if (event->key() == Qt::Key_R &&
		event->modifiers() == Qt::Modifier::CTRL) ReloadFrame(true);

	else if (event->key() == Qt::Key_F) FrameSelected();
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

	else if (event->key() == Qt::Key_Right) NextFrame(true);
	else if (event->key() == Qt::Key_Left) PrevFrame(true);

	else if (event->modifiers() == Qt::KeypadModifier) {
		if (event->key() == Qt::Key_1)
			wMainMenu->setCurrentIndex(0);
		if (event->key() == Qt::Key_2)
			wMainMenu->setCurrentIndex(1);
		if (event->key() == Qt::Key_3)
			wMainMenu->setCurrentIndex(2);
	}
	else if (event->key() == Qt::Key_1) interactionButtons[InteractionMode::Examine]->click();
	else if (event->key() == Qt::Key_2) interactionButtons[InteractionMode::Split]->click();
	else if (event->key() == Qt::Key_3) interactionButtons[InteractionMode::Connect]->click();
	else if (event->key() == Qt::Key_4) interactionButtons[InteractionMode::Delete]->click();
	else if (event->key() == Qt::Key_5) interactionButtons[InteractionMode::ShapeColor]->click();
	else if (event->key() == Qt::Key_6) interactionButtons[InteractionMode::ShapeDelete]->click();
}


void ImageViewer::SnapEndpoints(bool checked)
{
	bool result = true;

	if (CtrlPressed()) {
		SNAPPING_DISTANCE2 = std::sqrt(SNAPPING_DISTANCE2);
		InputDialog d;
		d.AddItem(SNAPPING_DISTANCE2, "Snapping Distance", 0, 100);
		result = d.exec();
		SNAPPING_DISTANCE2 = SNAPPING_DISTANCE2 * SNAPPING_DISTANCE2;
	}

	if (result) {
		vectorGraphic().SnapEndpoints(SNAPPING_DISTANCE2);
		ShowMat();
	}
	else {
		std::cout << "Cancelled Merge.\n";
	}
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
		float& angle = MIN_MERGE_ANGLE;
		angle *= 180 / CV_PI;

		InputDialog d;
		d.AddItem(angle, "Minimum Angle In Degrees", 0, 180);
		result = d.exec();

		angle /= 180 / CV_PI;
	}

	if (result) {
		vectorGraphic().MergeConnected(MIN_MERGE_ANGLE);
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
		d.AddItem(SIMPLIFY_MAX_DIST, "Maximum Merge Distance", 0, 500);
		result = d.exec();
	}

	if (result) {
		for (VE::PolylinePtr& p : vectorGraphic().Polylines) {
			p->Simplify(SIMPLIFY_MAX_DIST);
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
		d.AddItem(SMOOTHING_ITERATIONS, "Iterations", 0, 100);
		d.AddItem(SMOOTHING_LAMBDA, "Lambda", 0.f, 1.f);
		result = d.exec();
	}

	if (result) {
		for (VE::PolylinePtr& p : vectorGraphic().Polylines) {
			p->Smooth(SMOOTHING_ITERATIONS, SMOOTHING_LAMBDA);
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
	for (VE::PolylinePtr& p : vectorGraphic().Polylines) p->Simplify(SIMPLIFY_MAX_DIST);
	vectorGraphic().MergeConnected(MIN_MERGE_ANGLE);
	for (VE::PolylinePtr& p : vectorGraphic().Polylines) p->Smooth(SMOOTHING_ITERATIONS, SMOOTHING_LAMBDA);
	for (VE::PolylinePtr& p : vectorGraphic().Polylines) p->Cleanup();

	vectorGraphic().ComputeConnectionStatus();
	ShowMat();
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

void ImageViewer::ClearShapes(bool checked)
{
	vectorGraphic().ClearShapes();
	ShowMat();
}

void ImageViewer::ReloadFrame(bool checked)
{
	activeFrame().Reload();
	ShowMat();
}

void ImageViewer::NextFrame(bool checked)
{
	if (frameIndex + 1 < frames.size())
		frameIndex++;

	lFrameText->setText((std::to_string(frameIndex + 1) + " / " + std::to_string((int)frames.size())).c_str());
	ShowMat();
}

void ImageViewer::PrevFrame(bool checked)
{
	if (frameIndex - 1 >= 0)
		frameIndex--;

	lFrameText->setText((std::to_string(frameIndex + 1) + " / " + std::to_string((int)frames.size())).c_str());
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
		QString("Choose a .svg with matching .png"), WORKING_DIRECTORY.c_str(),
		QString("Edited Lineart (*.png.l.svg);;Lineart (*.png.svg)"));

	std::cout << "Loading ";
	decltype(frames) newFrames;
	for (size_t i = 0; i < fileNames.size(); i++)
	{
		std::string fileName = fileNames[i].toStdString();
		Animation::Frame frame;
		if (frame.Load(fileName)) {
			newFrames.push_back(frame);
			std::cout << "|"; 
		}
		else {
			std::cout << ".";
		}
	}
	if (!newFrames.empty()) {
		frameIndex = 0;
		frames = newFrames;
	}
	std::cout << "\n";
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
		WORKING_DIRECTORY.c_str()).toStdString();
	
	auto directory = std::filesystem::path(directoryName);
	if (!std::filesystem::is_directory(directory))
		return;

	
	for (Animation::Frame&frame:frames)
	{

		auto path_l = directory / frame.getEditName();
		auto path_s = directory / frame.getShapeName();

		if (FILE_SAVE_LINES) {
			
			frame.getVectorGraphic().Save(path_l.string(), "", cv::Size2i(sourceImage().cols, sourceImage().rows));
		}
		if (FILE_SAVE_SHAPES) {
			frame.getVectorGraphic().SavePolyshapes(path_s.string(), "", cv::Size2i(sourceImage().cols, sourceImage().rows));
		}

	}

}

void ImageViewer::FileSetDirectory(bool checked)
{
	std::string directoryName = QFileDialog::getExistingDirectory(this, "Select Working Directory",
		WORKING_DIRECTORY.c_str()).toStdString();

	auto directory = std::filesystem::path(directoryName);
	if (std::filesystem::is_directory(directory)) {
		WORKING_DIRECTORY = directoryName;
		WorkingDirSave();
	}
}
