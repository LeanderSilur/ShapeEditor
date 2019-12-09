#include <iostream>
#include <QtWidgets/QApplication>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ImageViewer.h"
#include "ui_ShapeEditor.h"
#include "InputDialog.h"

#include <filesystem>

int main(int argc, char* argv[])
{
	std::cout << std::filesystem::current_path();
	QApplication a(argc, argv);


	QWidget c;
	Ui_ShapeEditor se;
	se.setupUi(&c);

	ImageViewer *iw = new ImageViewer(se.viewerGrp);

	c.show();


	iw->ConnectUi(se);

	

	return a.exec();
}