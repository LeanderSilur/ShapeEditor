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
	const float HIGHLIGHT_DISTANCE = 256;

	ImageViewer(QWidget* parent = nullptr);

	void setGraphic(VectorGraphic& vg);
	void setMat(cv::Mat mat);

	void ConnectUi(Ui_ShapeEditor& se);

private:
	VE::Transform transform;
	VectorGraphic vectorGraphic;

	enum class InteractionMode {
		Examine,
		Split,
		Connect,
		Delete
	};
	std::unordered_map<InteractionMode, QPushButton*> interactionButtons;
	typedef void (ImageViewer::*DrawFunction)();
	DrawFunction interactionDraw = nullptr;

	bool ClosestLinePoint(VE::Point& closest, VE::PolylinePtr& element);

	// Draw Functions
	void DrawHighlight();
	void DrawHighlightPoints();
	void DrawConnect();

	void ShowMat();
	void Frame(const VE::Bounds& bounds);
	void FrameSelected();
	void FrameAll();
	void FrameTrue();


	inline QPoint QMousePosition();
	inline VE::Point VEMousePosition();
	bool CtrlPressed();
	bool ShiftPressed();

	QPoint mouseDown;
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
};