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
	const float HIGHLIGHT_DISTANCE = 64;

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

	// Color which is used, when coloring a shape. Helpful, that it transcends a single graphic.
	VE::ColorAreaPtr ActiveColor = std::make_shared<VE::ColorArea>();

	bool FILE_SAVE_LINES = true;
	bool FILE_SAVE_SHAPES = false;

	const cv::Scalar POLYLINE_EXAMINE = cv::Scalar(20, 200, 200);
	const cv::Scalar POLYLINE_SPLIT = cv::Scalar(255, 200, 0);
	const cv::Scalar POLYLINE_CONNECT1 = cv::Scalar(30, 180, 200);
	const cv::Scalar POLYLINE_CONNECT2 = cv::Scalar(30, 255, 30);
	const cv::Scalar POLYLINE_CONNECT_LINE = cv::Scalar(60, 150, 100);
	const cv::Scalar POLYLINE_DELETE = cv::Scalar(255, 0, 0);

	std::string WORKING_DIRECTORY = "D:/190725_sequence_colorization/files/malila/rgb";

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
		ShapeDelete,
	};
	QTabWidget* wMainMenu;
	QLabel* lInfoText;
	QLabel* lFrameText;
	std::unordered_map<InteractionMode, QPushButton*> interactionButtons;
	typedef void (ImageViewer::*VoidFunc)();
	VoidFunc interactionDraw = nullptr;
	typedef void (ImageViewer::* QMouseFunc)(QMouseEvent*);
	QMouseFunc interactionRelease = nullptr;

	bool ClosestLinePoint(VE::Point& closest, VE::PolylinePtr& element);

	// Draw Functions
	void DrawHighlight(const cv::Scalar & color);
	void DrawHighlightPoints(const cv::Scalar & color);
	void DrawExamine();
	void DrawSplit();
	void DrawConnect();
	void DrawDelete();

	// Mouse Release Functions
	void ReleaseSplit(QMouseEvent* event);
	void ReleaseConnect(QMouseEvent* event);
	void ReleaseDelete(QMouseEvent* event);
	void ReleaseShapeColor(QMouseEvent* event);
	void ReleaseShapeDelete(QMouseEvent* event);

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
	QPoint mousePrevPos;
	bool lmbHold = false;
	InteractionMode mode = InteractionMode::Examine;

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
	
	void ReloadFrame(bool checked);
	void NextFrame(bool checked);
	void PrevFrame(bool checked);

	// Change modes (Lines)
	void ctExamine(bool checked);
	void ctSplit(bool checked);
	void ctConnect(bool checked);
	void ctDelete(bool checked);

	// Change modes (Shapes)
	void ctShapeColor(bool checked);
	void ctShapeDelete(bool checked);

	void FileLoad(bool checked);
	void FileSave(bool checked);
	void FileSetDirectory(bool checked);
};