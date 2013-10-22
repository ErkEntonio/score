/*
Copyright: LaBRI / SCRIME

Author: Jaime Chao (01/10/2013)

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
*/

#include "graphicstimeprocess.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QFinalState>
#include <QDebug>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>

/// @todo Use a namespace ?
const qint32 headerHeight = 20;

GraphicsTimeProcess::GraphicsTimeProcess(const QPointF &position, QGraphicsItem *parent, QGraphicsScene *scene)
  : QGraphicsObject(parent), _scene(scene), _boxEditingBrush(Qt::NoBrush), _boxExecutionBrush(Qt::yellow, Qt::Dense6Pattern)
{
  setFlags(QGraphicsItem::ItemIsSelectable |
           QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);

  //creer les time event de début et de fin

  createStates(position, parent);
  createTransitions();

  _stateMachine.setInitialState(_initialState);
  _stateMachine.start();

  _scene->clearSelection();
  _scene->addItem(this);
  setSelected(true);
}

GraphicsTimeProcess::~GraphicsTimeProcess()
{
  delete _initialState;
  delete _normalState; //will delete all child states
  delete _finalState;
}

void GraphicsTimeProcess::createStates(const QPointF &position, QGraphicsItem *parent)
{
  _initialState = new QState();
  _initialState->assignProperty(this, "pos", position);
  _initialState->assignProperty(this, "mainScenario", parent ? false : true); /// \todo peut mieux faire, utiliser propriété parent
  _initialState->assignProperty(this, "objectName", tr("Box"));
  _initialState->assignProperty(this, "running", false);
  _initialState->assignProperty(this, "paused", false);
  _initialState->assignProperty(this, "stopped", false);
  _initialState->assignProperty(this, "height", 100);
  _initialState->assignProperty(this, "width", 200);
  _stateMachine.addState(_initialState);

  // creating a new top-level state
  _normalState = new QState();

  _editionState = new QState(_normalState);
  _editionState->assignProperty(this,"enabled", true); /// @todo enabled n'est pas la plus approprié car trop radicale
  _editionState->assignProperty(this, "boxBrush", _boxEditingBrush);

  _executionState = new QState(_normalState);
  _executionState->assignProperty(this,"enabled", false);
  _executionState->assignProperty(this, "boxBrush", _boxExecutionBrush);

  _runningState = new QState(_executionState);
  _runningState->assignProperty(this, "running", true);
  _runningState->assignProperty(this, "paused", false);
  _runningState->assignProperty(this, "stopped", false);
  _pausedState = new QState(_executionState);
  _pausedState->assignProperty(this, "running", false);
  _pausedState->assignProperty(this, "paused", true);
  _pausedState->assignProperty(this, "stopped", false);
  _stoppedState = new QState(_executionState);
  _stoppedState->assignProperty(this, "running", false);
  _stoppedState->assignProperty(this, "paused", false);
  _stoppedState->assignProperty(this, "stopped", true);
  _executionState->setInitialState(_runningState);

  _normalState->setInitialState(_editionState);
  _stateMachine.addState(_normalState);

  //normalSizeState = new QState(editingState); //Using parrallel states to add normal and extended
  //extendedSizeState = new QState(editingState);

  _finalState = new QFinalState(); /// @todo gérer le final state et la suppression d'objets graphiques
  _stateMachine.addState(_finalState);
}

void GraphicsTimeProcess::createTransitions()
{
  _initialState->addTransition(_initialState, SIGNAL(propertiesAssigned()), _normalState);
  _editionState->addTransition(this, SIGNAL(playOrPauseButtonClicked()), _runningState);
  _runningState->addTransition(this, SIGNAL(playOrPauseButtonClicked()), _pausedState);
  _runningState->addTransition(this, SIGNAL(stopButtonClicked()), _stoppedState);
  _pausedState->addTransition(this, SIGNAL(playOrPauseButtonClicked()), _runningState);
  _pausedState->addTransition(this, SIGNAL(stopButtonClicked()), _stoppedState);
  _stoppedState->addTransition(_stoppedState, SIGNAL(propertiesAssigned()), _editionState);
  _normalState->addTransition(this, SIGNAL(suppress()), _finalState);
}

QRectF GraphicsTimeProcess::boundingRect() const
{
  return QRectF(0,0,_width,_height);
}

void GraphicsTimeProcess::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(option)
  Q_UNUSED(widget)

  /// Draw the header part
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(Qt::gray));
  painter->drawRect(0,0,_width, headerHeight);

  painter->setPen(Qt::SolidLine);
  painter->setBrush(boxBrush());
  painter->drawText(boundingRect(), Qt::AlignLeft | Qt::AlignTop, objectName());

  /// Draw the bounding rectangle
  painter->drawRect(boundingRect());
}

void GraphicsTimeProcess::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
      if (event->pos().y() <= headerHeight){
          emit headerClicked();
        }
    }
  QGraphicsObject::mouseDoubleClickEvent(event);
}

void GraphicsTimeProcess::setmainScenario(bool arg)
{
  if (_mainScenario != arg) {
      _mainScenario = arg;
      emit mainScenarioChanged(arg);
    }
}

void GraphicsTimeProcess::setwidth(qreal arg)
{
  if (_width != arg) {
      _width = arg;
      emit widthChanged(arg);
    }
}

void GraphicsTimeProcess::setheight(qreal arg)
{
  if (_height != arg) {
      _height = arg;
      emit heightChanged(arg);
    }
}

void GraphicsTimeProcess::setrunning(bool arg)
{
  if (_running != arg) {
      _running = arg;
      emit runningChanged(arg);
    }
}

void GraphicsTimeProcess::setpaused(bool arg)
{
  if (_paused != arg) {
      _paused = arg;
      emit pausedChanged(arg);
    }
}

void GraphicsTimeProcess::setstopped(bool arg)
{
  if (_stopped != arg) {
      _stopped = arg;
      emit stoppedChanged(arg);
    }
}

void GraphicsTimeProcess::setboxBrush(QBrush arg)
{
  if (_boxBrush != arg) {
      _boxBrush = arg;
      emit boxBrushChanged(arg);
    }
}
