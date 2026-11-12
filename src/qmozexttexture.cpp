/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozexttexture.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <QDebug>

// QMOZEMBED_DEBUG=1: what arrives from Gecko and what binding it does.
static bool debugTexture()
{
    static const bool on = qEnvironmentVariableIsSet("QMOZEMBED_DEBUG");
    return on;
}

QMozExtTexture::QMozExtTexture()
{
}

QMozExtTexture::~QMozExtTexture()
{
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
    }
}

int QMozExtTexture::textureId() const
{
    return m_textureId;
}

QSize QMozExtTexture::textureSize() const
{
    return m_textureSize;
}

bool QMozExtTexture::hasAlphaChannel() const
{
    return false;
}

bool QMozExtTexture::hasMipmaps() const
{
    return false;
}

QRectF QMozExtTexture::normalizedTextureSubRect() const
{
    return QRectF(0, 0, 1, 1);
}

unsigned QMozExtTexture::target() const
{
    return m_target;
}

void QMozExtTexture::bind()
{
    if (m_textureId != 0) {
        glBindTexture(m_target, m_textureId);
        updateBindOptions();
    }
}

bool QMozExtTexture::updateTexture()
{
    bool changed = false;

    static const PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES
            = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    if (!glEGLImageTargetTexture2DOES) {
        return false;
    }

    // We don't want to keep a pointer to a QMozWindow in the texture as that could be deleted in
    // the main thread ahead of the texture which would be deleted in the render thread so we
    // connect two through a direct signal connection which we can remove when the window is
    // destroyed.
    Q_EMIT getPlatformImage([&](EGLImageKHR image, int width, int height, unsigned target) {
        if (image) {
            changed = true;

            m_textureSize = QSize(width, height);

            // a texture name keeps the target it was first bound to
            if (m_textureId != 0 && target != m_target) {
                glDeleteTextures(1, &m_textureId);
                m_textureId = 0;
            }
            static const bool force2D = qEnvironmentVariableIsSet("QMOZEMBED_TEXTURE_2D");
            if (force2D) {
                target = GL_TEXTURE_2D;
            }
            m_target = target;
            if (m_textureId == 0) {
                glGenTextures(1, &m_textureId);
            }

            glBindTexture(m_target, m_textureId);
            glEGLImageTargetTexture2DOES(m_target, image);
            // A GL_TEXTURE_2D starts with a mipmap filter and repeat wrap, which
            // leave a texture of a size that is no power of two incomplete in
            // OpenGL ES 2.0, and an incomplete texture samples black (external
            // textures start with linear and clamp).
            glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            if (debugTexture()) {
                const GLenum error = glGetError();
                qDebug() << "QMozExtTexture: image" << image << width << "x" << height
                         << "target" << Qt::hex << m_target
                         << "texture" << m_textureId << "gl error" << Qt::hex << error
                         << "egl display" << eglGetCurrentDisplay()
                         << "context" << eglGetCurrentContext();
            }
        }
    });
    if (debugTexture() && !changed) {
        qDebug() << "QMozExtTexture: no image";
    }

    return changed;
}
