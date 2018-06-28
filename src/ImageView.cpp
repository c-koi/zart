/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   ImageView.cpp
 * @author Sebastien Fourey
 * @date   July 2010
 * @brief Definition of the methods of the class ImageView.
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
 *
 */
#include "ImageView.h"
#include <QFrame>
#include <QLayout>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QPainter>
#include <QRect>
#include <QThread>
#include <cmath>
#include <iostream>
#include "Common.h"
#include "OverrideCursor.h"

ImageView::ImageView(QWidget * parent) : QWidget(parent)
{
  setAutoFillBackground(false);
  _image = QImage(640, 480, QImage::Format_RGB888);
  _image.fill(0);
  setMinimumSize(320, 200);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  _imagePosition = geometry();
  _scaleFactor = 1.0;
  _zoomOriginal = false;
  _movedKeypointIndex = -1;
}

void ImageView::setImageSize(int width, int height)
{
  _image = _image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void ImageView::setBackgroundColor(QColor color)
{
  _backgroundColor = color;
}

void ImageView::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  QMutexLocker locker(&_imageMutex);
  if (_image.size() == size()) {
    painter.drawImage(0, 0, _image);
    _imagePosition = rect();
    _scaleFactor = 1.0;
    return;
  }
  if (_backgroundColor.isValid()) {
    painter.fillRect(rect(), _backgroundColor);
  }
  QImage scaled;
  const double imageRatio = _image.width() / static_cast<double>(_image.height());
  const double widgetRatio = width() / static_cast<double>(height());
  if (imageRatio > widgetRatio) {
    scaled = _image.scaledToWidth(width());
    _imagePosition = QRect(0, (height() - scaled.height()) / 2, scaled.width(), scaled.height());
    _scaleFactor = scaled.width() / static_cast<double>(_image.width());
    painter.drawImage(_imagePosition.topLeft(), scaled);
  } else {
    scaled = _image.scaledToHeight(height());
    _imagePosition = QRect((width() - scaled.width()) / 2, 0, scaled.width(), scaled.height());
    _scaleFactor = scaled.height() / static_cast<double>(_image.height());
    painter.drawImage(_imagePosition.topLeft(), scaled);
  }
  paintKeypoints(painter);
}

void ImageView::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) {
    int index = keypointUnderMouse(e->pos());
    if (index != -1) {
      _movedKeypointIndex = index;
      _keypointTimestamp.start();
      if (!_keypoints[index].keepOpacityWhenSelected) {
        update();
      }
    }
  }

  if (!_imagePosition.contains(e->pos())) {
    e->accept();
    return;
  }
  *e = mapMousePositionToImage(e);
  emit mousePress(e);
  e->accept();
}

void ImageView::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) {
    if (_movedKeypointIndex != -1) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      _movedKeypointIndex = -1;
      emit keypointPositionsChanged();
    }
    e->accept();
    return;
  }
}

void ImageView::mouseDoubleClickEvent(QMouseEvent * e)
{
  emit mouseDoubleClick(e);
}

void ImageView::mouseMoveEvent(QMouseEvent * e)
{
  if (hasMouseTracking() && (_movedKeypointIndex == -1)) {
    int index = keypointUnderMouse(e->pos());
    OverrideCursor::setPointingHand(index != -1);
  }

  if (e->buttons() & (Qt::LeftButton | Qt::MiddleButton)) {
    if (_movedKeypointIndex != -1 && (_keypointTimestamp.elapsed() > 25)) {
      QPointF p = pointInWidgetToKeypointPosition(e->pos());
      KeypointList::Keypoint & kp = _keypoints[_movedKeypointIndex];
      kp.setPosition(p);
      repaint();
      _keypointTimestamp.restart();
      emit keypointPositionsChanged();
    }
    e->accept();
  }

  if (!_imagePosition.contains(e->pos())) {
    e->ignore();
    return;
  }
  *e = mapMousePositionToImage(e);
  emit mouseMove(e);
  e->accept();
}

void ImageView::resizeEvent(QResizeEvent * event)
{
  emit resized(event->size());
}

void ImageView::zoomOriginal()
{
  _zoomOriginal = true;
  setMinimumSize(_image.size());
  resize(_image.size());
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ImageView::zoomFitBest()
{
  _zoomOriginal = false;
  QFrame * frame = dynamic_cast<QFrame *>(parent());
  if (frame) {
    setMinimumSize(320, 200);
    QRect rect = frame->layout()->contentsRect();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    resize(rect.width(), rect.height());
  }
}

void ImageView::checkSize()
{
  if (!_zoomOriginal)
    return;
  if (size() != _image.size()) {
    setMinimumSize(_image.size());
    resize(_image.size());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  }
}

QMouseEvent ImageView::mapMousePositionToImage(QMouseEvent * e)
{
  int x = (e->pos().x() - _imagePosition.left()) / _scaleFactor;
  int y = (e->pos().y() - _imagePosition.top()) / _scaleFactor;
  return QMouseEvent(e->type(), QPoint(x, y), e->button(), e->buttons(), e->modifiers());
}

void ImageView::keyPressEvent(QKeyEvent * e)
{
  if (e->key() == Qt::Key_Space) {
    e->accept();
    emit spaceBarPressed();
  }
  if (e->key() == Qt::Key_Escape) {
    emit escapePressed();
    e->accept();
  }
}

void ImageView::closeEvent(QCloseEvent * e)
{
  emit aboutToClose();
  e->accept();
}

void ImageView::setKeypoints(const KeypointList & keypoints)
{
  _keypoints = keypoints;
  setMouseTracking(_keypoints.size());
  update();
}

KeypointList ImageView::keypoints() const
{
  return _keypoints;
}

QRect ImageView::imagePosition()
{
  return _imagePosition;
}

void ImageView::paintKeypoints(QPainter & painter)
{
  QPen pen;
  pen.setColor(Qt::black);
  pen.setWidth(2);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(pen);

  QRect visibleRect = rect() & _imagePosition;
  KeypointList::reverse_iterator it = _keypoints.rbegin();
  int index = _keypoints.size() - 1;
  while (it != _keypoints.rend()) {
    if (!it->isNaN()) {
      const KeypointList::Keypoint & kp = *it;
      const int & radius = kp.actualRadiusFromPreviewSize(_imagePosition.size());
      QPoint visibleCenter = keypointToVisiblePointInWidget(kp);
      QPoint realCenter = keypointToPointInWidget(kp);
      QRect r(visibleCenter.x() - radius, visibleCenter.y() - radius, 2 * radius, 2 * radius);
      QColor brushColor = kp.color;
      if ((index == _movedKeypointIndex) && !kp.keepOpacityWhenSelected) {
        brushColor.setAlpha(255);
      }
      if (visibleRect.contains(realCenter, false)) {
        painter.setBrush(brushColor);
        pen.setStyle(Qt::SolidLine);
      } else {
        painter.setBrush(brushColor.darker(150));
        pen.setStyle(Qt::DotLine);
      }
      pen.setColor(QColor(0, 0, 0, brushColor.alpha()));
      painter.setPen(pen);
      painter.drawEllipse(r);
    }
    ++it;
    --index;
  }
}

int ImageView::roundedDistance(const QPoint & p1, const QPoint & p2)
{
  const float dx = p1.x() - p2.x();
  const float dy = p1.y() - p2.y();
  return (int)roundf(sqrtf(dx * dx + dy * dy));
}

int ImageView::keypointUnderMouse(const QPoint & p)
{
  KeypointList::iterator it = _keypoints.begin();
  int index = 0;
  while (it != _keypoints.end()) {
    if (!it->isNaN()) {
      const KeypointList::Keypoint & kp = *it;
      QPoint center = keypointToVisiblePointInWidget(kp);
      if (roundedDistance(center, p) <= (kp.actualRadiusFromPreviewSize(_imagePosition.size()) + 2)) {
        return index;
      }
    }
    ++it;
    ++index;
  }
  return -1;
}

QPoint ImageView::keypointToPointInWidget(const KeypointList::Keypoint & kp) const
{
  return QPoint(roundf(_imagePosition.left() + (_imagePosition.width() - 1) * (kp.x / 100.0f)), roundf(_imagePosition.top() + (_imagePosition.height() - 1) * (kp.y / 100.0f)));
}

QPoint ImageView::keypointToVisiblePointInWidget(const KeypointList::Keypoint & kp) const
{
  QPoint p = keypointToPointInWidget(kp);
  p.rx() = std::max(std::max(_imagePosition.left(), 0), std::min(p.x(), std::min(rect().left() + rect().width(), _imagePosition.left() + _imagePosition.width())));
  p.ry() = std::max(std::max(_imagePosition.top(), 0), std::min(p.y(), std::min(rect().top() + rect().height(), _imagePosition.top() + _imagePosition.height())));
  return p;
}

QPointF ImageView::pointInWidgetToKeypointPosition(const QPoint & p) const
{
  QPointF result(100.0 * (p.x() - _imagePosition.left()) / (float)(_imagePosition.width() - 1), 100.0 * (p.y() - _imagePosition.top()) / (float)(_imagePosition.height() - 1));
  result.rx() = std::min(300.0, std::max(-200.0, result.x()));
  result.ry() = std::min(300.0, std::max(-200.0, result.y()));
  return result;
}
