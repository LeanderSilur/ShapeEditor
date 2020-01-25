#pragma once
#include "ui_ShapeEditor.h"
#include <iostream>

class ShapeEditor : public QWidget
{
	Q_OBJECT

public:
	explicit ShapeEditor() {
		ui.setupUi(this);
	}

	Ui::ShapeEditor& getUi() {
		return ui;
	};

signals:
	void keyPress(QKeyEvent* event);

private:
	Ui::ShapeEditor ui;
	void keyPressEvent(QKeyEvent* event) {
		keyPress(event);
	};
};