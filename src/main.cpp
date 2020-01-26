#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ImageViewer.h"
#include "ShapeEditor.h"
#include "InputDialog.h"

#include <filesystem>
#include "PtTree.h"

#include "AnimationFrame.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	ShapeEditor se;
	ImageViewer *iw = new ImageViewer(se.getUi().viewerGrp);
	iw->ConnectUi(se);


	Animation::Frame frame("D:/190725_sequence_colorization/files/demo/file.png", "D:/190725_sequence_colorization/files/demo/file.png.svg");
	iw->AddFrame(frame);
	iw->NextFrame(true);
	iw->RemoveOverlaps(true);
	se.show();



	

	return a.exec();
}