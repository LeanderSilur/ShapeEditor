#include <iostream>
#include <QtWidgets/QApplication>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ImageViewer.h"
#include "ui_ShapeEditor.h"
#include "InputDialog.h"

#include <filesystem>
#include "PtTree.h"

int main(int argc, char* argv[])
{
	/*
	PtTree tree;
	std::vector<VE::Point> points = {
		VE::Point(0, 0),
		VE::Point(1, 1),
		VE::Point(2, 2),
		VE::Point(3, 3),
		VE::Point(4, 2),
		VE::Point(5, 1),
		VE::Point(6, 0)
	};
	tree.setPoints(points);

	float dist2 = 0;
	int i = tree.nearest(VE::Point(2, 2), dist2);

	return 0;
	*/
	 
	QApplication a(argc, argv);


	QWidget c;
	Ui_ShapeEditor se;
	se.setupUi(&c);

	ImageViewer *iw = new ImageViewer(se.viewerGrp);

	c.show();


	iw->ConnectUi(se);

	

	return a.exec();
}