#ifndef BUBBLES_CLIENT_WIDGET_HPP
#define BUBBLES_CLIENT_WIDGET_HPP

#include <QWidget>
#include "client/engine/bubbles_client_engine.hpp"


namespace bubbles{
namespace client{

class Engine;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(Engine::PointerT &_engine_ptr, QWidget *parent = 0);

    void setFloatBased(bool floatBased);
    void setAntialiased(bool antialiased);

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
	
	void start();
signals:
	void updateSignal();
	void closeSignal();
protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    
private:
    bool				antialiased;
	QPoint				pos;
	bool				moving;
	Engine::PointerT	&engine_ptr;
};

}//namespace client
}//namespace bubbles
#endif
