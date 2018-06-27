/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   PointParameter.cpp
 * @author Sebastien Fourey
 * @date   June 2018
 *
 * @brief  Definition of the class PointParameter
 *
 * This file is part of the ZArt software's source code.
 *
 * Copyright Sebastien Fourey / GREYC Ensicaen (2010-...)
 *
 *                    https://foureys.users.greyc.fr/
 *
 * This software is a computer program whose purpose is to demonstrate
 * the possibilities of the GMIC image processing language by offering the
 * choice of several manipulations on a video stream aquired from a webcam. In
 * other words, ZArt is a GUI for G'MIC real-time manipulations on the output
 * of a webcam.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". See also the directory "Licence" which comes
 * with this source code for the full text of the CeCILL license.
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#include "PointParameter.h"
#include <QApplication>
#include <QColorDialog>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "Common.h"
#include "KeypointList.h"

int PointParameter::_defaultColorNextIndex = 0;
unsigned long PointParameter::_randomSeed = 12345;

PointParameter::PointParameter(QDomNode node, QObject * parent) : AbstractParameter(parent), _node(node), _defaultPosition(0, 0), _position(0, 0), _removable(false)
{
  _label = nullptr;
  _colorLabel = nullptr;
  _labelX = nullptr;
  _labelY = nullptr;
  _spinBoxX = nullptr;
  _spinBoxY = nullptr;
  _removeButton = nullptr;
  _rowCell = nullptr;
  _notificationEnabled = true;
  _connected = false;
  _defaultRemovedStatus = false;
  _radius = KeypointList::Keypoint::DefaultRadius;
  _visible = true;
  _keepOpacityWhenSelected = false;
  setRemoved(false);
  initFromNode(node);
}

PointParameter::~PointParameter()
{
  delete _label;
  delete _rowCell;
}

bool PointParameter::isVisible() const
{
  return _visible;
}

void PointParameter::addTo(QWidget * widget, int row)
{
  if (!_visible) {
    return;
  }
  QGridLayout * grid = dynamic_cast<QGridLayout *>(widget->layout());
  if (!grid) {
    return;
  }
  delete _label;
  delete _rowCell;

  _rowCell = new QWidget(widget);
  _rowCell->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  QHBoxLayout * hbox = new QHBoxLayout(_rowCell);
  hbox->setMargin(0);
  hbox->addWidget(_colorLabel = new QLabel(_rowCell));

  QFontMetrics fm(widget->font());
  QRect r = fm.boundingRect("CLR");
  _colorLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  QPixmap pixmap(r.width(), r.height());
  QPainter painter(&pixmap);
  painter.setBrush(QColor(_color.red(), _color.green(), _color.blue()));
  painter.setPen(Qt::black);
  painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
  _colorLabel->setPixmap(pixmap);

  hbox->addWidget(_labelX = new QLabel("X", _rowCell));
  hbox->addWidget(_spinBoxX = new QDoubleSpinBox(_rowCell));
  hbox->addWidget(_labelY = new QLabel("Y", _rowCell));
  hbox->addWidget(_spinBoxY = new QDoubleSpinBox(_rowCell));
  if (_removable) {
    hbox->addWidget(_removeButton = new QToolButton(_rowCell));
    _removeButton->setCheckable(true);
    _removeButton->setChecked(_removed);
    _removeButton->setIcon(QIcon(":/images/list-remove.png"));
  } else {
    _removeButton = nullptr;
  }
  hbox->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
  _spinBoxX->setRange(-200.0, 300.0);
  _spinBoxY->setRange(-200.0, 300.0);
  _spinBoxX->setValue(_position.x());
  _spinBoxY->setValue(_position.y());

  grid->addWidget(_label = new QLabel(_name, widget), row, 0, 1, 1);
  grid->addWidget(_rowCell, row, 1, 1, 2);

  setRemoved(_removed);
  connectSpinboxes();
}

void PointParameter::addToKeypointList(KeypointList & list) const
{
  if (_removable && _removed) {
    list.add(KeypointList::Keypoint(_color, _removable, _radius, _keepOpacityWhenSelected));
  } else {
    list.add(KeypointList::Keypoint(_position.x(), _position.y(), _color, _removable, _radius, _keepOpacityWhenSelected));
  }
}

void PointParameter::extractPositionFromKeypointList(KeypointList & list)
{
  Q_ASSERT_X(!list.isEmpty(), __PRETTY_FUNCTION__, "Keypoint list is empty");
  enableNotifications(false);
  KeypointList::Keypoint kp = list.front();
  if (!kp.isNaN()) {
    _position.setX(kp.x);
    _position.setY(kp.y);
    if (_spinBoxX) {
      _spinBoxX->setValue(kp.x);
      _spinBoxY->setValue(kp.y);
    }
  }
  list.pop_front();
  enableNotifications(true);
}

QString PointParameter::textValue() const
{
  if (_removed) {
    return "nan,nan";
  } else {
    return QString("%1,%2").arg(_position.x()).arg(_position.y());
  }
}

void PointParameter::setValue(const QString & value)
{
  QStringList list = value.split(",");
  if (list.size() == 2) {
    bool ok;
    float x = list[0].toFloat(&ok);
    bool xNaN = (list[0].toUpper() == "NAN");
    if (ok && !xNaN) {
      _position.setX(x);
    }
    float y = list[1].toFloat(&ok);
    bool yNaN = (list[1].toUpper() == "NAN");
    if (ok && !yNaN) {
      _position.setY(y);
    }
    _removed = (_removable && xNaN && yNaN);

    if (_spinBoxX) {
      disconnectSpinboxes();
      if (_removeButton) {
        setRemoved(_removed);
        _removeButton->setChecked(_removed);
      }
      if (!_removed) {
        _spinBoxX->setValue(_position.x());
        _spinBoxY->setValue(_position.y());
      }
      connectSpinboxes();
    }
  }
}

void PointParameter::reset()
{
  enableNotifications(false);
  if (_spinBoxX) {
    _spinBoxX->setValue(_defaultPosition.rx());
    _spinBoxY->setValue(_defaultPosition.ry());
  }
  if (_removeButton && _removable) {
    _removeButton->setChecked((_removed = _defaultRemovedStatus));
  }
  enableNotifications(true);
}

// P = point(x,y,removable{(0),1},burst{(0),1},r,g,b,a{negative->keepOpacityWhenSelected},radius,widget_visible{0|(1)})
bool PointParameter::initFromNode(QDomNode node)
{
  QDomNamedNodeMap attributes = node.attributes();
  _name = attributes.namedItem("name").nodeValue();

  bool ok = true;
  _defaultPosition.setX(50.0);
  _defaultPosition.setY(50.0);
  _defaultPosition = _position;
  _color.setRgb(255, 255, 255, 255);
  _removable = false;
  _radius = KeypointList::Keypoint::DefaultRadius;
  _visible = true;
  _keepOpacityWhenSelected = false;

  float x = 50.0;
  float y = 50.0;
  _removed = false;
  bool xNaN = true;
  bool yNaN = true;

  node.attributes();

  bool savedAsRemoved = false;
  QString savedPosition;

  QDomNode item;

  item = attributes.namedItem("position");
  if (!item.isNull()) {
    QString position = item.nodeValue();

    savedPosition = node.toElement().attribute("savedValue");
    if (!savedPosition.isNull()) {
      savedAsRemoved = savedPosition.endsWith(QChar('-'));
      savedPosition.chop(1);
      position = savedPosition;
    }
    QStringList list = position.split(",");
    if (list.size() >= 1) {
      x = list[0].toFloat(&ok);
      xNaN = (list[0].toUpper() == "NAN");
      if (!ok) {
        return false;
      }
      if (xNaN) {
        x = 50.0;
      }
    }
    if (list.size() >= 2) {
      y = list[1].toFloat(&ok);
      yNaN = (list[1].toUpper() == "NAN");
      if (!ok) {
        return false;
      }
      if (yNaN) {
        y = 50.0;
      }
    }
  }
  _defaultPosition.setX(x);
  _defaultPosition.setY(y);
  _removed = _defaultRemovedStatus = (xNaN || yNaN);

  item = attributes.namedItem("removable");
  if (!item.isNull()) {
    int removable = item.nodeValue().toInt(&ok);
    if (!ok) {
      return false;
    }
    switch (removable) {
    case -1:
      _removable = _removed = _defaultRemovedStatus = true;
      break;
    case 0:
      _removable = _removed = false;
      break;
    case 1:
      _removable = true;
      _defaultRemovedStatus = _removed = (xNaN && yNaN);
      break;
    default:
      return false;
    }
  }

  if (!savedPosition.isNull()) {
    _removed = savedAsRemoved;
  }

  item = attributes.namedItem("color");
  if (!item.isNull()) {
    QStringList color = item.nodeValue().split(",");

    if (color.size() >= 1) {
      int red = color[0].toInt(&ok);
      if (!ok) {
        return false;
      }
      _color.setRed(red);
      _color.setGreen(red);
      _color.setBlue(red);
    } else {
      pickColorFromDefaultColormap();
    }

    if (color.size() >= 2) {
      int green = color[1].toInt(&ok);
      if (!ok) {
        return false;
      }
      _color.setGreen(green);
      _color.setBlue(0);
    }

    if (color.size() >= 3) {
      int blue = color[2].toInt(&ok);
      if (!ok) {
        return false;
      }
      _color.setBlue(blue);
    }

    if (color.size() >= 4) {
      int alpha = color[3].toInt(&ok);
      if (!ok) {
        return false;
      }
      if (color[3].trimmed().startsWith("-") || (alpha < 0)) {
        _keepOpacityWhenSelected = true;
      }
      _color.setAlpha(std::abs(alpha));
    }
  } else {
    pickColorFromDefaultColormap();
  }

  item = attributes.namedItem("radius");
  if (!item.isNull()) {
    QString s = item.nodeValue().trimmed();
    float radius;
    if (s.endsWith("%")) {
      s.chop(1);
      radius = -s.toFloat(&ok);
    } else {
      radius = s.toFloat(&ok);
    }
    if (!ok) {
      return false;
    }
    _radius = radius;
  }

  item = attributes.namedItem("visible");
  if (!item.isNull() && item.nodeValue().trimmed() == "0") {
    _visible = false;
  }

  _position = _defaultPosition;
  return true;
}

void PointParameter::enableNotifications(bool on)
{
  _notificationEnabled = on;
}

void PointParameter::onSpinBoxChanged()
{
  _position = QPointF(_spinBoxX->value(), _spinBoxY->value());
  emit valueChanged();
}

void PointParameter::setRemoved(bool on)
{
  _removed = on;
  if (_spinBoxX) {
    _spinBoxX->setDisabled(on);
    _spinBoxY->setDisabled(on);
    _labelX->setDisabled(on);
    _labelY->setDisabled(on);
    if (_removeButton) {
      _removeButton->setIcon(on ? QIcon(":/images/list-add.png") : QIcon(":/images/list-remove.png"));
    }
  }
}

void PointParameter::saveValueInDOM()
{
  _node.toElement().setAttribute("savedValue", textValue() + (_removed ? "-" : "+"));
}

void PointParameter::resetDefaultColorIndex()
{
  _defaultColorNextIndex = 0;
  _randomSeed = 12345;
}

void PointParameter::onRemoveButtonToggled(bool on)
{
  setRemoved(on);
  emit valueChanged();
}

int PointParameter::randomChannel()
{
  int value = (_randomSeed / 65536) % 256;
  _randomSeed = _randomSeed * 1103515245 + 12345;
  return value;
}

void PointParameter::connectSpinboxes()
{
  if (_connected || !_spinBoxX) {
    return;
  }
  connect(_spinBoxX, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));
  connect(_spinBoxY, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxChanged()));
  if (_removable && _removeButton) {
    connect(_removeButton, SIGNAL(toggled(bool)), this, SLOT(onRemoveButtonToggled(bool)));
  }
  _connected = true;
}

void PointParameter::disconnectSpinboxes()
{
  if (!_connected || !_spinBoxX) {
    return;
  }
  _spinBoxX->disconnect(this);
  _spinBoxY->disconnect(this);
  if (_removable && _removeButton) {
    _removeButton->disconnect(this);
  }
  _connected = false;
}

void PointParameter::pickColorFromDefaultColormap()
{
  switch (_defaultColorNextIndex) {
  case 0:
    _color.setRgb(255, 255, 255, 255);
    break;
  case 1:
    _color = Qt::red;
    break;
  case 2:
    _color = Qt::green;
    break;
  case 3:
    _color.setRgb(64, 64, 255, 255);
    break;
  case 4:
    _color = Qt::cyan;
    break;
  case 5:
    _color = Qt::magenta;
    break;
  case 6:
    _color = Qt::yellow;
    break;
  default:
    _color.setRgb(randomChannel(), randomChannel(), randomChannel());
  }
  ++_defaultColorNextIndex;
}
