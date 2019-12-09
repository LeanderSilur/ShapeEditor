#include "InputDialog.h"

#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <iostream>


class QToggleButton : public QPushButton{
private:
	void keyPressEvent(QKeyEvent* event) {
		if ((event->key() >= 49 && event->key() <= 90) ||
			event->key() == 32)
			toggle();
	}
};

InputDialog::InputDialog(QWidget* parent)
	: QDialog(parent)
{
	QGridLayout* layout = new QGridLayout();
	this->setLayout(layout);
	setWindowFlags(Qt::FramelessWindowHint);


	this->setStyleSheet("\
QDialog {\
	background-color:#aaa;\
	border:1px solid black;\
}\
QDialog * { color:#fe9; }\
\
QPushButton {\
	background-color:#777;\
	border-radius:7px;\
	border:1px solid #000;\
	padding:5px;\
}\
QPushButton:checked {\
	color:#fe5;\
	background-color: #886;\
}\
\
QPushButton, QLineEdit {\
	border:1px solid black;\
}\
QLineEdit { background-color:#666; }\
QPushButton:focus, QLineEdit:focus {\
	border:1px solid #fc0;\
}\
");

}

void InputDialog::AddItem(bool& b, std::string text)
{
	QToggleButton* btn = new QToggleButton();
	static_cast<QGridLayout*>(layout())->addWidget(btn, widgets.size(), 1);
	btn->setText(QString(text.c_str()));
	btn->setCheckable(true);
	btn->setChecked(b);
	widgets.push_back(btn);
	values.push_back(&b);

	QObject::connect(btn, &QToggleButton::clicked,
		this, &InputDialog::boolChanged);
	btn->installEventFilter(this);
}

void InputDialog::AddItem(float& f, std::string text, float min, float max)
{
	QLineEdit* edit = new QLineEdit();
	static_cast<QGridLayout*>(layout())->addWidget(new QLabel(text.c_str()), widgets.size(), 0);
	static_cast<QGridLayout*>(layout())->addWidget(edit, widgets.size(), 1);
	QDoubleValidator* validator = new QDoubleValidator(min, max, 4);
	edit->setValidator(validator);
	edit->setText(QString::number(f));
	widgets.push_back(edit);
	values.push_back(&f);

	QObject::connect(edit, &QLineEdit::textEdited,
		this, &InputDialog::floatChanged);
	edit->installEventFilter(this);

}

void InputDialog::AddItem(int& i, std::string text, int min, int max)
{
	QLineEdit* edit = new QLineEdit();
	static_cast<QGridLayout*>(layout())->addWidget(new QLabel(text.c_str()), widgets.size(), 0);
	static_cast<QGridLayout*>(layout())->addWidget(edit, widgets.size(), 1);
	QIntValidator* validator = new QIntValidator(min, max);
	edit->setValidator(validator);
	edit->setText(QString::number(i));
	widgets.push_back(edit);
	values.push_back(&i);

	QObject::connect(edit, &QLineEdit::textEdited,
		this, &InputDialog::intChanged);
	edit->installEventFilter(this);

}

void InputDialog::AddItem(std::string& str, std::string text)
{
	QLineEdit* edit = new QLineEdit();
	static_cast<QGridLayout*>(layout())->addWidget(new QLabel(text.c_str()), widgets.size(), 0);
	static_cast<QGridLayout*>(layout())->addWidget(edit, widgets.size(), 1);
	edit->setText(str.c_str());
	widgets.push_back(edit);
	values.push_back(&str);

	QObject::connect(edit, &QLineEdit::textEdited,
		this, &InputDialog::strChanged);
	edit->installEventFilter(this);
}

bool InputDialog::eventFilter(QObject* object, QEvent* event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Enter ||
			keyEvent->key() == Qt::Key_Return ||
			keyEvent->key() == Qt::Key_Escape) {
			this->keyPressEvent(keyEvent);
			return true;
		}
	}
	return false;
}

int InputDialog::getIndex(QObject* sender)
{
	auto it = std::find(widgets.begin(), widgets.end(), sender);
	int index = std::distance(widgets.begin(), it);
	return index;
}

void InputDialog::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Enter ||
		event->key() == Qt::Key_Return)
	{
		std::cout << "accept\n";
		this->accept();
	}
	else if (event->key() == Qt::Key_Escape) {
		std::cout << "reject\n";
		this->reject();
	}
	else
	{
		//myLabelText->setText("You Pressed Other Key");
	}
}

void InputDialog::boolChanged(bool state)
{
	int index = getIndex(QObject::sender());
	*((bool*)values[index]) = state;
}


void InputDialog::floatChanged(const QString& val)
{
	int index = getIndex(QObject::sender());
	*((float*)values[index]) = val.toFloat();
}

void InputDialog::intChanged(const QString& val)
{
	int index = getIndex(QObject::sender());
	*((int*)values[index]) = val.toInt();
}

void InputDialog::strChanged(const QString& val)
{
	int index = getIndex(QObject::sender());
	*((std::string*)values[index]) = val.toStdString();
}
