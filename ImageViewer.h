#pragma once

#include <unordered_map> 

#include <opencv2/core.hpp>
#include <QtWidgets>

#include "VectorGraphic.h"
#include "ui_ShapeEditor.h"

class ImageViewer : public QLabel
{
	Q_OBJECT

public:
	const float HIGHLIGHT_DISTANCE = 64;

	ImageViewer(QWidget* parent = nullptr);

	void setGraphic(VectorGraphic& vg);
	void setMat(cv::Mat mat);

	void ConnectUi(Ui_ShapeEditor& se);

private:
	bool FILE_SAVE_LINES = true;
	bool FILE_SAVE_SHAPES = false;

	const cv::Scalar POLYLINE_EXAMINE = cv::Scalar(20, 200, 200);
	const cv::Scalar POLYLINE_SPLIT = cv::Scalar(255, 200, 0);
	const cv::Scalar POLYLINE_CONNECT1 = cv::Scalar(30, 180, 200);
	const cv::Scalar POLYLINE_CONNECT2 = cv::Scalar(30, 255, 30);
	const cv::Scalar POLYLINE_CONNECT_LINE = cv::Scalar(60, 150, 100);
	const cv::Scalar POLYLINE_DELETE = cv::Scalar(255, 0, 0);
	VE::Transform transform;
	VectorGraphic vectorGraphic;

	enum class InteractionMode {
		Examine,
		Split,
		Connect,
		Delete
	};
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

	void ShowMat();
	void Frame(const VE::Bounds& bounds);
	void FrameSelected();
	void FrameAll();
	void FrameTrue();


	inline QPoint QMousePosition();
	inline VE::Point VEMousePosition();
	bool CtrlPressed();
	bool ShiftPressed();

	QPoint mousePressPos;
	QPoint mousePrevPos;
	bool lmbHold = false;
	InteractionMode mode = InteractionMode::Examine;

	cv::Mat source;
	cv::Mat display;


protected:
	void keyPressEvent(QKeyEvent* event);
	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void wheelEvent(QWheelEvent* event);
	void resizeEvent(QResizeEvent* event);

public slots:
	void SnapEndpoints(bool checked);
	void RemoveOverlaps(bool checked);
	void MergeConnected(bool checked);
	void Simplify(bool checked);
	void Smooth(bool checked);

	void BasicCleanup(bool checked);

	void ComputeConnectionStatus(bool checked);
	void RemoveUnusedConnections(bool checked);
	void CalcShapes(bool checked);
	
	// change modes 
	void ctExamine(bool checked);
	void ctSplit(bool checked);
	void ctConnect(bool checked);
	void ctDelete(bool checked);

	void FileLoad(bool checked);
	void FileSave(bool checked);
};