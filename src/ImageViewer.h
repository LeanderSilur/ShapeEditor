#pragma once

#include <unordered_map> 

#include <opencv2/core.hpp>
#include <QtWidgets>
#include "ShapeEditor.h"
#include "VectorGraphic.h"
#include "AnimationFrame.h"

class ImageViewer : public QLabel
{
	Q_OBJECT

public:
	float HIGHLIGHT_DISTANCE = 64;
	const float ENDPOINT_SNAPPING_DISTANCE = 16;

	ImageViewer(QWidget* parent = nullptr);

	void AddFrame(Animation::Frame &frame);
	void ConnectUi(ShapeEditor& se);

private:
	// Used in SnapEndpoints(). The distance2 of an endpoint to a
	// potential intersection point.
	float SNAPPING_DISTANCE2 = 4;
	// Used in MergeConnected(). If two linesegments share an enpoint
	// they can be connected if this angle isn't exceeded.
	// PI * 0.5 ^= 90°
	float MIN_MERGE_ANGLE = CV_PI * 100 / 180;

	// Used in polyline smoothing.
	int SMOOTHING_ITERATIONS = 4;
	float SMOOTHING_LAMBDA = 0.5f;

	// Used in polyline simplification. Points closer together
	// than this value will be merged together in Polyline::Simplify()
	float SIMPLIFY_MAX_DIST = 2.0f;

	// Used in selective line smoothing
	float DRAW_SPACING2 = 1.0f;
	float DRAW_SNAP2 = 9.0f;
	float DRAW_SNAP_ENDS2 = 36.0f;
	int DRAW_SMOOTH_ITER = 3;
	float DRAW_SMOOTH_LAMBDA = 0.5f;
	float SMOOTHBRUSH_MAX_DIST = 40.0f;
	int SMOOTHBRUSH_ITER = 2;
	float SMOOTHBRUSH_LAMBDA = 0.5f;

	// Color which is used, when coloring a shape. Helpful, that it transcends a single graphic.
	VE::ColorAreaPtr ActiveColor = std::make_shared<VE::ColorArea>();

	bool FILE_SAVE_LINES = true;
	bool FILE_SAVE_SHAPES = false;

	typedef cv::Point3_<uint8_t> Pixel;

	const cv::Scalar POLYLINE_EXAMINE = cv::Scalar(20, 200, 200);
	const cv::Scalar POLYLINE_SPLIT = cv::Scalar(255, 200, 0);
	const cv::Scalar POLYLINE_CONNECT1 = cv::Scalar(30, 180, 200);
	const cv::Scalar POLYLINE_CONNECT2 = cv::Scalar(30, 255, 30);
	const cv::Scalar POLYLINE_CONNECT_LINE = cv::Scalar(60, 150, 100);
	const cv::Scalar POLYLINE_DELETE = cv::Scalar(255, 0, 0);
	const cv::Scalar POLYLINE_SMOOTH= cv::Scalar(50, 200, 240);

	const Pixel BACKGROUND_COLOR = Pixel(200, 200, 200);

	std::string WORKING_DIRECTORY = "D:/";

	VE::Transform transform;
	std::vector<Animation::Frame> frames;
	int frameIndex = 0;
	cv::Mat display;
	inline Animation::Frame& activeFrame() {
		return frames[frameIndex];
	};
	inline cv::Mat& sourceImage() {
		return frames[frameIndex].getImage();
	};
	inline VectorGraphic& vectorGraphic() {
		return frames[frameIndex].getVectorGraphic();
	};


	enum class InteractionMode {
		Examine,
		Split,
		Connect,
		Delete,
		ShapeColor,
		Draw,
		Erase,
		Grab,
		Smooth,
	};
	QTabWidget* wMainMenu;
	QLabel* lInfoText;
	QLabel* lFrameText;
	std::unordered_map<InteractionMode, QPushButton*> interactionButtons;
	typedef void (ImageViewer::*VoidFunc)();
	VoidFunc interactionDraw = nullptr;
	typedef void (ImageViewer::* QMouseFunc)(QMouseEvent*);
	QMouseFunc interactionRelease = nullptr;

	// Get the closest line and point. ptIdx will be -1 if no close points are found.
	VectorGraphic::CPParams ClosestLinePositionView(const VE::Point& target, bool snapEndpoints,
		const VectorGraphic::CPParams::M& method = VectorGraphic::CPParams::M::Point);
	VectorGraphic::CPParams ClosestLinePosition(const VE::Point& target, const VectorGraphic::CPParams::M& method, float maxDist2, float snapEndpointDist2 = 0.0);

	// Draw Functions
	void DrawHighlight(const cv::Scalar & color);
	void DrawHighlightPoints(const cv::Scalar & color, bool splitSegments = false);
	void DrawExamine();
	void DrawSplit();
	void DrawConnect();
	void DrawDelete();
	void DrawDraw();
	void DrawErase();
	void DrawGrab();
	void DrawSmooth();


	// Copy Paste
	std::vector<VE::PolylinePtr> copyPasteBuffer;
	void Copy(bool replaceBuffer = true);
	void Paste();

	// Mouse Release Functions
	void ReleaseSplit(QMouseEvent* event);
	void ReleaseConnect(QMouseEvent* event);
	void ReleaseDelete(QMouseEvent* event);
	void ReleaseShapeColor(QMouseEvent* event);
	void ReleaseShapeDelete(QMouseEvent* event);
	void ReleaseDraw(QMouseEvent* event);
	void ReleaseErase(QMouseEvent* event);
	void ReleaseGrab(QMouseEvent* event);
	void ReleaseSmooth(QMouseEvent* event);


	void ShowMat();
	void Frame(const VE::Bounds& bounds);
	void FrameSelected();
	void FrameAll();
	void FrameTrue();


	inline QPoint QMousePosition();
	inline VE::Point VEMousePosition();
	bool CtrlPressed();
	bool ShiftPressed();
	bool AltPressed();

	QPoint mousePressPos;
	QPoint mousePrevPos; // for dragging the view
	bool lmbHold = false;
	InteractionMode mode = InteractionMode::Examine;

	std::vector<VE::Point> drawPoints;

	void WorkingDirSave();
	void WorkingDirRead();

protected:
	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void dragEnterEvent(QDragEnterEvent* e);
	void dragMoveEvent(QDragMoveEvent* e);
	void dropEvent(QDropEvent* e);
	void wheelEvent(QWheelEvent* event);
	void resizeEvent(QResizeEvent* event);

public slots:
	void keyPressEvent(QKeyEvent* event);
	void SnapEndpoints(bool checked);
	void RemoveOverlaps(bool checked);
	void MergeConnected(bool checked);
	void Simplify(bool checked);
	void Smooth(bool checked);

	void BasicCleanup(bool checked);

	void ComputeConnectionStatus(bool checked);
	void RemoveUnusedConnections(bool checked);
	void CalcShapes(bool checked);
	void ClearShapes(bool checked);

	void ReloadFrameOrig(bool checked);
	void ReloadFrameEdit(bool checked);
	void NextFrame(bool checked);
	void PrevFrame(bool checked);

	// Change modes
	void changeToMode(InteractionMode mode, VoidFunc interactionDraw, QMouseFunc interactionRelease);
	void ctExamine(bool checked);
	void ctSplit(bool checked);
	void ctConnect(bool checked);
	void ctDelete(bool checked);
	void ctDraw(bool checked);
	void ctErase(bool checked);
	void ctGrab(bool checked);
	void ctSmooth(bool checked);
	void ctShapeColor(bool checked);

	// Change Snapping Distance
	void setHighlightDistance(double value);
	void setHighlightDistance();

	void FileLoad(bool checked);
	void FileSave(bool checked);
	void FileSetDirectory(bool checked);
};