#pragma once

#include <QtWidgets>
#include <vector>


class InputDialog : public QDialog {
	
	Q_OBJECT

public:
	InputDialog(QWidget* parent = nullptr);
	void AddItem(bool& b, std::string text = "");
	void AddItem(float& f, std::string text = "", float min = 0.f, float max = 1.f);
	void AddItem(int& i, std::string text = "", int min = 0, int max = 10);
	void AddItem(std::string& str, std::string text = "");

	bool eventFilter(QObject* object, QEvent* event);
private:
	std::vector<void*> values;
	std::vector<QObject*> widgets;
	int getIndex(QObject* sender);
	void keyPressEvent(QKeyEvent* event);

private slots:
	void boolChanged(bool state);
	void floatChanged(const QString& val);
	void intChanged(const QString& val);
	void strChanged(const QString& val);
};
