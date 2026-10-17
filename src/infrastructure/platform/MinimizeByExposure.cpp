#include "MinimizeByExposure.h"

#include <QtCore/QEvent>

MinimizeByExposure::MinimizeByExposure(QWindow* window)
    : QObject(window)
    , window_(window)
{
    window_->installEventFilter(this);
}

bool MinimizeByExposure::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == window_ && event->type() == QEvent::Expose && window_->isVisible() && !window_->isExposed())
    {
        emit Minimized();
    }

    return QObject::eventFilter(watched, event);
}
