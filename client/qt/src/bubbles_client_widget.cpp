
#include "bubbles_client_widget.hpp"

#include <QPainter>
#include <QMouseEvent>

#include <stdlib.h>

namespace bubbles{
namespace client{

Widget::Widget(Engine::PointerT &_engine_ptr, QWidget *parent)
    : QWidget(parent), engine_ptr(_engine_ptr)
{
    antialiased = true;
    
    setBackgroundRole(QPalette::Base);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	moving = false;
}

void Widget::setAntialiased(bool antialiased)
{
    this->antialiased = antialiased;
    update();
}

QSize Widget::minimumSizeHint() const
{
    return QSize(1280, 720);
}

QSize Widget::sizeHint() const
{
    return QSize(1280, 720);
}

void Widget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, antialiased);
    //painter.translate();
	
    painter.setRenderHint(QPainter::Antialiasing, antialiased);
    
	for(int i = 0; i < width(); i += 80){
		for(int j = 0; j < height(); j += 80){
			painter.setPen(QPen(QColor(i % 255, j % 255, 100), 3));
			painter.setBrush(QBrush(QColor(i % 255, j % 255,100)));
			//painter.drawEllipse(QRect(pos.x() - 20, pos.y() - 20, 40, 40));
			painter.drawEllipse(QRect(pos.x() + i - 20, pos.y() + j  - 20, 40, 40));
		}
	}
}

void Widget::mousePressEvent(QMouseEvent *event){
	if (event->button() == Qt::LeftButton) {
        pos = event->pos();
        moving = true;
		update();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event){
	if ((event->buttons() & Qt::LeftButton) && moving){
		pos = event->pos();
		update();
	}
}

void Widget::mouseReleaseEvent(QMouseEvent *event){
	if (event->button() == Qt::LeftButton && moving) {
		pos = event->pos();
        moving = false;
		update();
    }
}

void Widget::resizeEvent(QResizeEvent *event){
	QWidget::resizeEvent(event);
}

}//namespace client
}//namespace bubbles
