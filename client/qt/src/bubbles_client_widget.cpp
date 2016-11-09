
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
    
	connect(this, SIGNAL(updateSignal()), this, SLOT(update()), Qt::QueuedConnection);
	connect(this, SIGNAL(closeSignal()), this, SLOT(close()), Qt::QueuedConnection);
	
	engine_ptr->setExitFunction([this](){emit closeSignal();});
	engine_ptr->setGuiUpdateFunction([this](){emit updateSignal();});
	
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

void split_color(uint32_t _c, uint32_t &_r, uint32_t &_g, uint32_t &_b){
	_r = _c & 0xff;
	_g = (_c >> 8) & 0xff;
	_b = (_c >> 16) & 0xff;
}

void Widget::start(){
	engine_ptr->moveEvent(pos.x(), pos.y());
	show();
}

void Widget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, antialiased);
    painter.translate(width()/2, height()/2);
	
    painter.setRenderHint(QPainter::Antialiasing, antialiased);
    
	PlotIterator plotit = engine_ptr->plot();
	uint32_t		r;
	uint32_t		g;
	uint32_t		b;
	
	while(not plotit.end()){
		split_color(plotit.rgbColor(), r, g, b);
		painter.setPen(QPen(QColor(r, g, b), 3));
		painter.setBrush(QBrush(QColor(r, g, b)));
		painter.drawEllipse(QRect(plotit.x() - 10, plotit.y() - 10, 20, 20));
		++plotit;
	}
	
	split_color(plotit.myRgbColor(), r, g, b);
	painter.setPen(QPen(QColor(r, g, b), 3));
	painter.setBrush(QBrush(QColor(r, g, b)));
	painter.drawEllipse(QRect(pos.x() - 20, pos.y() - 20, 40, 40));
}

void Widget::mousePressEvent(QMouseEvent *event){
	if (event->button() == Qt::LeftButton) {
        pos = event->pos();
		pos = QPoint(pos.x() - width()/2, pos.y() - height()/2);
        moving = true;
		update();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event){
	if ((event->buttons() & Qt::LeftButton) && moving){
		pos = event->pos();
		pos = QPoint(pos.x() - width()/2, pos.y() - height()/2);
		engine_ptr->moveEvent(pos.x(), pos.y());
		update();
	}
}

void Widget::mouseReleaseEvent(QMouseEvent *event){
	if (event->button() == Qt::LeftButton && moving) {
		pos = event->pos();
		pos = QPoint(pos.x() - width()/2, pos.y() - height()/2);
        moving = false;
		update();
    }
}

void Widget::resizeEvent(QResizeEvent *event){
	QWidget::resizeEvent(event);
}

}//namespace client
}//namespace bubbles
