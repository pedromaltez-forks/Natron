//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef ROTOGUI_H
#define ROTOGUI_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QObject>
#include <QToolButton>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

class QInputEvent;
class QMouseEvent;
class QToolBar;
class QWidget;
class QIcon;
class QString;
class QToolButton;
class QKeyEvent;
class QPointF;
class ViewerTab;
class QAction;
class RotoItem;
class QUndoCommand;
class NodeGui;
class Bezier;
class RotoDrawableItem;
class BezierCP;
class GuiAppInstance;
struct RotoGuiSharedData;
class RotoContext;

class RotoToolButton
    : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(bool isSelected READ getIsSelected WRITE setIsSelected)
    
public:

    RotoToolButton(QWidget* parent);

    virtual ~RotoToolButton();
    

    void handleSelection();

    bool getIsSelected() const;
    void setIsSelected(bool s);
    
public Q_SLOTS:
    
    void handleLongPress();
    
private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    
    bool isSelected;
    bool wasMouseReleased;
};

class RotoGui
    : public QObject
{
    Q_OBJECT

public:

    enum RotoTypeEnum
    {
        eRotoTypeRotoscoping = 0,
        eRotoTypeRotopainting
    };

    enum RotoRoleEnum
    {
        eRotoRoleSelection = 0,
        eRotoRolePointsEdition,
        eRotoRoleBezierEdition,
        eRotoRolePaintBrush,
        eRotoRoleCloneBrush,
        eRotoRoleEffectBrush,
        eRotoRoleMergeBrush
    };

    enum RotoToolEnum
    {
        eRotoToolSelectAll = 0,
        eRotoToolSelectPoints,
        eRotoToolSelectCurves,
        eRotoToolSelectFeatherPoints,

        eRotoToolAddPoints,
        eRotoToolRemovePoints,
        eRotoToolRemoveFeatherPoints,
        eRotoToolOpenCloseCurve,
        eRotoToolSmoothPoints,
        eRotoToolCuspPoints,

        eRotoToolDrawBezier,
        eRotoToolDrawBSpline,
        eRotoToolDrawEllipse,
        eRotoToolDrawRectangle,
        
        eRotoToolSolidBrush,
        eRotoToolOpenBezier,
        eRotoToolEraserBrush,
        
        eRotoToolClone,
        eRotoToolReveal,
        
        eRotoToolBlur,
        eRotoToolSharpen,
        eRotoToolSmear,
        
        eRotoToolDodge,
        eRotoToolBurn
        
    };

    RotoGui(NodeGui* node,
            ViewerTab* parent,
            const boost::shared_ptr<RotoGuiSharedData> & sharedData);

    ~RotoGui();

    boost::shared_ptr<RotoGuiSharedData> getRotoGuiSharedData() const;
    GuiAppInstance* getApp() const;

    /**
     * @brief Return the horizontal buttons bar for the given role
     **/
    QWidget* getButtonsBar(RotoGui::RotoRoleEnum role) const;

    /**
     * @brief Same as getButtonsBar(getCurrentRole())
     **/
    QWidget* getCurrentButtonsBar() const;

    /**
     * @brief The currently used tool
     **/
    RotoGui::RotoToolEnum getSelectedTool() const;

    void setCurrentTool(RotoGui::RotoToolEnum tool,bool emitSignal);

    QToolBar* getToolBar() const;

    /**
     * @brief The selected role (selection,draw,add points, etc...)
     **/
    RotoGui::RotoRoleEnum getCurrentRole() const;

    void drawOverlays(double scaleX, double scaleY) const;

    bool penDown(double scaleX, double scaleY, Natron::PenType pen, bool isTabletEvent, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QMouseEvent* e);

    bool penDoubleClicked(double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, QMouseEvent* e);

    bool penMotion(double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QInputEvent* e);

    bool penUp(double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QMouseEvent* e);

    bool keyDown(double scaleX, double scaleY, QKeyEvent* e);

    bool keyUp(double scaleX, double scaleY, QKeyEvent* e);

    bool keyRepeat(double scaleX, double scaleY, QKeyEvent* e);
    
    void focusOut();

    bool isStickySelectionEnabled() const;

    /**
     * @brief Set the selection to be the given beziers and the given control points.
     * This can only be called on the main-thread.
     **/
    void setSelection(const std::list<boost::shared_ptr<RotoDrawableItem> > & selectedBeziers,
                      const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & selectedCps);
    void setSelection(const boost::shared_ptr<Bezier> & curve,
                      const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & point);

    void getSelection(std::list<boost::shared_ptr<RotoDrawableItem> >* selectedBeziers,
                      std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >* selectedCps);

    void refreshSelectionBBox();

    void setBuiltBezier(const boost::shared_ptr<Bezier> & curve);

    boost::shared_ptr<Bezier> getBezierBeingBuild() const;

    /**
     * @brief For undo/redo purpose, calling this will do 3 things:
     * Refresh overlays
     * Trigger a new render
     * Trigger an auto-save
     * Never call this upon the *first* redo() call, we do this already in the user event methods.
     **/
    void evaluate(bool redraw);

    void autoSaveAndRedraw();

    void pushUndoCommand(QUndoCommand* cmd);

    QString getNodeName() const;

    /**
     * @brief This pointer is not meant to be stored away
     **/
    RotoContext* getContext();

    /**
     * @brief Calls RotoContext::removeItem but also clears some pointers if they point to
     * this curve. For undo/redo purpose.
     **/
    void removeCurve(const boost::shared_ptr<RotoDrawableItem>& curve);

    bool isFeatherVisible() const;

    void linkPointTo(const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & cp);

    void notifyGuiClosing();
    
Q_SIGNALS:

    /**
     * @brief Emitted when the selected role changes
     **/
    void roleChanged(int previousRole,int newRole);

    void selectedToolChanged(int);

public Q_SLOTS:

    void updateSelectionFromSelectionRectangle(bool onRelease);

    void onSelectionCleared();

    void onToolActionTriggered();

    void onToolActionTriggered(QAction* act);

    void onAutoKeyingButtonClicked(bool);

    void onFeatherLinkButtonClicked(bool);

    void onRippleEditButtonClicked(bool);

    void onStickySelectionButtonClicked(bool);
    
    void onBboxClickButtonClicked(bool);

    void onAddKeyFrameClicked();

    void onRemoveKeyFrameClicked();

    void onCurrentFrameChanged(SequenceTime,int);

    void restoreSelectionFromContext();

    void onRefreshAsked();

    void onCurveLockedChanged(int);

    void onSelectionChanged(int reason);

    void onDisplayFeatherButtonClicked(bool toggled);


    void smoothSelectedCurve();
    void cuspSelectedCurve();
    void removeFeatherForSelectedCurve();
    void lockSelectedCurves();
    
    void onColorWheelButtonClicked();
    void onDialogCurrentColorChanged(const QColor& color);
    
    void onPressureOpacityClicked(bool isDown);
    void onPressureSizeClicked(bool isDown);
    void onPressureHardnessClicked(bool isDown);
    void onBuildupClicked(bool isDown);
    
    void onResetCloneTransformClicked();
    
private:
    

    void showMenuForCurve(const boost::shared_ptr<Bezier> & curve);

    void showMenuForControlPoint(const boost::shared_ptr<Bezier> & curve,
                                 const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & cp);


    /**
     *@brief Moves of the given pixel the selected control points.
     * This takes into account the zoom factor.
     **/
    void moveSelectedCpsWithKeyArrows(int x,int y);

    void onToolActionTriggeredInternal(QAction* action,bool emitSignal);


    QAction* createToolAction(QToolButton* toolGroup,
                              const QIcon & icon,
                              const QString & text,
                              const QString & tooltip,
                              const QKeySequence & shortcut,
                              RotoGui::RotoToolEnum tool);
    struct RotoGuiPrivate;
    boost::scoped_ptr<RotoGuiPrivate> _imp;
};

#endif // ROTOGUI_H
