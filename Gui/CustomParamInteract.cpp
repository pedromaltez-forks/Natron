//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "CustomParamInteract.h"

#include <QThread>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QByteArray>

#include "Gui/KnobGui.h"
#include "Gui/FromQtEnums.h"

#include "Engine/OfxOverlayInteract.h"
#include "Engine/Knob.h"
#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"


using namespace Natron;

struct CustomParamInteractPrivate
{
    KnobGui* knob;
    OFX::Host::Param::Instance* ofxParam;
    boost::shared_ptr<OfxParamOverlayInteract> entryPoint;
    QSize preferredSize;
    double par;
    GLuint savedTexture;
    
    CustomParamInteractPrivate(KnobGui* knob,
                               void* ofxParamHandle,
                               const boost::shared_ptr<OfxParamOverlayInteract> & entryPoint)
        : knob(knob)
          , ofxParam(0)
          , entryPoint(entryPoint)
          , preferredSize()
          , par(0)
          , savedTexture(0)
    {
        assert(entryPoint && ofxParamHandle);
        ofxParam = reinterpret_cast<OFX::Host::Param::Instance*>(ofxParamHandle);
        assert( ofxParam->verifyMagic() );

        par = entryPoint->getProperties().getIntProperty(kOfxParamPropInteractSizeAspect);
        int pW,pH;
        entryPoint->getPreferredSize(pW, pH);
        preferredSize.setWidth(pW);
        preferredSize.setHeight(pH);
    }
};

CustomParamInteract::CustomParamInteract(KnobGui* knob,
                                         void* ofxParamHandle,
                                         const boost::shared_ptr<OfxParamOverlayInteract> & entryPoint,
                                         QWidget* parent)
    : QGLWidget(parent)
      , _imp( new CustomParamInteractPrivate(knob,ofxParamHandle,entryPoint) )
{
    double minW,minH;

    entryPoint->getMinimumSize(minW, minH);
    setMinimumSize(minW, minH);
}

CustomParamInteract::~CustomParamInteract()
{
}

void
CustomParamInteract::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    glCheckError();

    /*
     http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParametersInteracts
     The GL_PROJECTION matrix will be an orthographic 2D view with -0.5,-0.5 at the bottom left and viewport width-0.5, viewport height-0.5 at the top right.

     The GL_MODELVIEW matrix will be the identity matrix.
     */
    {
        GLProtectAttrib a(GL_TRANSFORM_BIT);
        GLProtectMatrix p(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-0.5, width() - 0.5, -0.5, height() - 0.5, 1, -1);
        GLProtectMatrix m(GL_MODELVIEW);
        glLoadIdentity();

        /*A parameter's interact draw function will have full responsibility for drawing the interact, including clearing the background and swapping buffers.*/
        OfxPointD scale;
        scale.x = scale.y = 1.;
        int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
        _imp->entryPoint->drawAction(time, scale);
        glCheckError();
    } // GLProtectAttrib a(GL_TRANSFORM_BIT);
}

void
CustomParamInteract::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

void
CustomParamInteract::resizeGL(int w,
                              int h)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );
    if (h == 0) {
        h = 1;
    }
    glViewport (0, 0, w, h);
    _imp->entryPoint->setSize(w, h);
}

QSize
CustomParamInteract::sizeHint() const
{
    return _imp->preferredSize;
}

void
CustomParamInteract::swapOpenGLBuffers()
{
    swapBuffers();
}

void
CustomParamInteract::redraw()
{
    updateGL();
}

void
CustomParamInteract::getViewportSize(double &w,
                                     double &h) const
{
    w = width();
    h = height();
}

void
CustomParamInteract::getPixelScale(double & xScale,
                                   double & yScale) const
{
    xScale = 1.;
    yScale = 1.;
}

void
CustomParamInteract::getBackgroundColour(double &r,
                                         double &g,
                                         double &b) const
{
    r = 0;
    g = 0;
    b = 0;
}

void
CustomParamInteract::saveOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&_imp->savedTexture);
    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack();
    glPushClientAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glCheckProjectionStack();
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glCheckModelviewStack();
    glPushMatrix();

    // set defaults to work around OFX plugin bugs
    glEnable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it
}

void
CustomParamInteract::restoreOpenGLContext()
{
    assert(QThread::currentThread() == qApp->thread());

    glBindTexture(GL_TEXTURE_2D, _imp->savedTexture);
    //glActiveTexture(_imp->activeTexture);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

void
CustomParamInteract::mousePressEvent(QMouseEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxPointD pos;
    OfxPointI viewportPos;
    pos.x = e->x();
    pos.y = e->y();
    viewportPos.y = e->x();
    viewportPos.y = e->y();
    OfxStatus stat = _imp->entryPoint->penDownAction(time, scale, pos, viewportPos, /*pressure=*/1.);
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

void
CustomParamInteract::mouseMoveEvent(QMouseEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxPointD pos;
    OfxPointI viewportPos;
    pos.x = e->x();
    pos.y = e->y();
    viewportPos.y = e->x();
    viewportPos.y = e->y();
    OfxStatus stat = _imp->entryPoint->penMotionAction(time, scale, pos, viewportPos, /*pressure=*/1.);
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

void
CustomParamInteract::mouseReleaseEvent(QMouseEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxPointD pos;
    OfxPointI viewportPos;
    pos.x = e->x();
    pos.y = e->y();
    viewportPos.y = e->x();
    viewportPos.y = e->y();
    OfxStatus stat = _imp->entryPoint->penUpAction(time, scale, pos, viewportPos, /*pressure=*/1.);
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

void
CustomParamInteract::focusInEvent(QFocusEvent* /*e*/)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxStatus stat = _imp->entryPoint->gainFocusAction(time, scale);
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

void
CustomParamInteract::focusOutEvent(QFocusEvent* /*e*/)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    OfxStatus stat = _imp->entryPoint->loseFocusAction(time, scale);
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

void
CustomParamInteract::keyPressEvent(QKeyEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    QByteArray keyStr;
    OfxStatus stat;
    if ( e->isAutoRepeat() ) {
        stat = _imp->entryPoint->keyRepeatAction( time, scale,(int)QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), keyStr.data() );
    } else {
        stat = _imp->entryPoint->keyDownAction( time, scale, (int)QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), keyStr.data() );
    }
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

void
CustomParamInteract::keyReleaseEvent(QKeyEvent* e)
{
    OfxPointD scale;

    scale.x = scale.y = 1.;
    int time = _imp->knob->getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    QByteArray keyStr;
    OfxStatus stat = _imp->entryPoint->keyUpAction( time, scale, (int)QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), keyStr.data() );
    if (stat == kOfxStatOK) {
        updateGL();
    }
}

