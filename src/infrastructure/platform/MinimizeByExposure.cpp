#include "MinimizeByExposure.h"

#include <QtCore/QEvent>

MinimizeByExposure::MinimizeByExposure(QWindow* window, const std::chrono::milliseconds focusGrace)
    : QObject(window)
    , window_(window)
    , focusGrace_(focusGrace)
{
    window_->installEventFilter(this);
}

bool MinimizeByExposure::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != window_)
    {
        return QObject::eventFilter(watched, event);
    }

    if (event->type() == QEvent::FocusOut)
    {
        sinceFocusLost_.start();
    }
    else if (event->type() == QEvent::FocusIn)
    {
        sinceFocusLost_.invalidate();
    }
    else if (event->type() == QEvent::Expose && window_->isVisible() && !window_->isExposed() && HadFocusJustNow())
    {
        sinceFocusLost_.invalidate();
        emit Minimized();
    }

    return QObject::eventFilter(watched, event);
}

bool MinimizeByExposure::HadFocusJustNow() const
{
    if (window_->isActive())
    {
        return true;
    }

    return sinceFocusLost_.isValid() && sinceFocusLost_.durationElapsed() <= focusGrace_;
}
