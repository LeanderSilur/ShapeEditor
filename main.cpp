#include <iostream>
#include <QtWidgets/QApplication>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ImageViewer.h"
#include "ui_ShapeEditor.h"
#include "InputDialog.h"


int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	cv::Mat im = cv::imread("D:/190725_sequence_colorization/files/w.png");
	VectorGraphic vg;
	vg.LoadPolylines("D:/190725_sequence_colorization/files/simple4_lines.svg");

	QWidget c;
	Ui_ShapeEditor se;
	se.setupUi(&c);

	ImageViewer *iw = new ImageViewer(se.viewerGrp);
	iw->setGraphic(vg);
	iw->setMat(im);

	//se.viewerGrp->layout()->addWidget(iw);
	c.show();


	iw->ConnectUi(se);

	

	return a.exec();
}