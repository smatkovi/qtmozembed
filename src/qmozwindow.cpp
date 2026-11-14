/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (c) 2015 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozwindow.h"

#include "qmozcontext.h"
#include "qmozwindow_p.h"

#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteWindow.h"

using namespace mozilla::embedlite;

QMozWindow::QMozWindow(const QSize &size, QObject *parent)
    : QObject(parent)
    , d(new QMozWindowPrivate(*this, size))
{
    Q_ASSERT_X(!size.isEmpty(),
               "QMozWindow::QMozWindow",
               QString("Window size is empty, width = %1 and height = %2").arg(size.width()).arg(size.height()).toUtf8().constData());
}

QMozWindow::~QMozWindow()
{
    Q_ASSERT(!d->mReserved);
}

void QMozWindow::setNativeWindow(quintptr window)
{
    d->mNativeWindow = window;
    if (d->mWindow) {
        d->mWindow->SetNativeWindow(window);
    }
}

void QMozWindow::reserve()
{
    if (!d->mWindow) {
        d->mWindow = QMozContext::instance()->GetApp()->CreateWindow(d->mSize.width(), d->mSize.height(), d.data());
        d->mReserved = true;
        if (d->mNativeWindow) {
            d->mWindow->SetNativeWindow(d->mNativeWindow);
        }
    }
}

void QMozWindow::release()
{
    if (d->mWindow) {
        QMozContext::instance()->GetApp()->DestroyWindow(d->mWindow);
        d->mWindow = nullptr;
    }
}

bool QMozWindow::isReserved() const
{
    return d->mReserved;
}

void QMozWindow::setSize(const QSize &size)
{
    d->setSize(size);
}

QSize QMozWindow::size() const
{
    return d->mSize;
}

void QMozWindow::setContentOrientation(Qt::ScreenOrientation orientation)
{
    d->setContentOrientation(orientation);
}

void QMozWindow::setPrimaryOrientation(Qt::ScreenOrientation orientation)
{
    d->setPrimaryOrientation(orientation);
}

Qt::ScreenOrientation QMozWindow::contentOrientation() const
{
    return d->mOrientation;
}

Qt::ScreenOrientation QMozWindow::pendingOrientation() const
{
    return d->mPendingOrientation;
}

Qt::ScreenOrientation QMozWindow::primaryOrientation() const
{
    return d->mPrimaryOrientation;
}

void QMozWindow::getPlatformImage(const std::function<void(void *image, int width, int height, unsigned target)> &callback)
{
    // ESR 153 EmbedLite only offers the synchronous descriptor form; the handle is
    // borrowed for the duration of the callback, so the consumer must import it there.
    // The target is the GL texture target Gecko made the image for, GL_TEXTURE_2D
    // for an image of a GL texture (Mesa), GL_TEXTURE_EXTERNAL_OES otherwise.
    d->mWindow->WithPlatformImage([&callback](const mozilla::embedlite::PlatformImageDescriptor &desc) {
        const unsigned target =
            desc.textureTarget == mozilla::embedlite::PlatformImageTextureTarget::Texture2D
                ? 0x0DE1  /* GL_TEXTURE_2D */
                : 0x8D65; /* GL_TEXTURE_EXTERNAL_OES */
        callback(desc.handle, desc.width, desc.height, target);
    });
}

void QMozWindow::clearPlatformImage()
{
    d->mWindow->ClearPlatformImage();
}

void QMozWindow::suspendRendering()
{
    d->mWindow->SuspendRendering();
}

void QMozWindow::resumeRendering()
{
    d->mWindow->ResumeRendering();
}

void QMozWindow::scheduleUpdate()
{
    d->mWindow->ScheduleUpdate();
}

bool QMozWindow::setReadyToPaint(bool ready)
{
    return d->setReadyToPaint(ready);
}

bool QMozWindow::readyToPaint() const
{
    return d->PreRender();
}

bool QMozWindow::isCompositorCreated()
{
    return d->mCompositorCreated;
}

void QMozWindow::timerEvent(QTimerEvent *event)
{
    d->timerEvent(event);
}
