/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   KeypointList.h
 * @author Sebastien Fourey
 * @date   June 2018
 * @brief  Declaration of the class KeypointList
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
#ifndef _ZART_KEYPOINTLIST_H_
#define _ZART_KEYPOINTLIST_H_

#include <QColor>
#include <QPointF>
#include <QSize>
#include <cmath>
#include <deque>

class KeypointList {
public:
  struct Keypoint {
    float x;
    float y;
    QColor color; /* A negative alpha means "Keep opacity while moved" */
    bool removable;
    float radius; /* A negative value is a percentage of the preview diagonal */
    bool keepOpacityWhenSelected;

    Keypoint(float x, float y, QColor color, bool removable, float radius, bool keepOpacityWhenSelected);
    Keypoint(QPointF point, QColor color, bool removable, float radius, bool keepOpacityWhenSelected);
    Keypoint(QColor color, bool removable, float radius, bool keepOpacityWhenSelected);
    bool isNaN() const;
    Keypoint & setNaN();
    inline void setPosition(float x, float y);
    inline void setPosition(const QPointF & p);
    static const int DefaultRadius = 6;

    inline int actualRadiusFromPreviewSize(const QSize & size) const;
  };

  KeypointList();
  void add(const Keypoint & keypoint);
  bool isEmpty();
  void clear();
  QPointF position(int n) const;
  QColor color(int n) const;
  bool isRemovable(int n) const;

  typedef std::deque<Keypoint>::iterator iterator;
  typedef std::deque<Keypoint>::const_iterator const_iterator;
  typedef std::deque<Keypoint>::reverse_iterator reverse_iterator;
  typedef std::deque<Keypoint>::const_reverse_iterator const_reverse_iterator;
  typedef std::deque<Keypoint>::size_type size_type;
  typedef std::deque<Keypoint>::reference reference;
  typedef std::deque<Keypoint>::const_reference const_reference;

  size_type size() const { return _keypoints.size(); }
  reference operator[](size_type index) { return _keypoints[index]; }
  const_reference operator[](size_type index) const { return _keypoints[index]; }
  void pop_front() { _keypoints.pop_front(); }
  const Keypoint & front() { return _keypoints.front(); }
  const Keypoint & front() const { return _keypoints.front(); }
  iterator begin() { return _keypoints.begin(); }
  iterator end() { return _keypoints.end(); }
  const_iterator cbegin() const { return _keypoints.cbegin(); }
  const_iterator cend() const { return _keypoints.cend(); }
  reverse_iterator rbegin() { return _keypoints.rbegin(); }
  reverse_iterator rend() { return _keypoints.rend(); }
  const_reverse_iterator crbegin() const { return _keypoints.crbegin(); }
  const_reverse_iterator crend() const { return _keypoints.crend(); }

private:
  std::deque<Keypoint> _keypoints;
};

void KeypointList::Keypoint::setPosition(float x, float y)
{
  KeypointList::Keypoint::x = x;
  KeypointList::Keypoint::y = y;
}

void KeypointList::Keypoint::setPosition(const QPointF & point)
{
  KeypointList::Keypoint::x = (float)point.x();
  KeypointList::Keypoint::y = (float)point.y();
}

int KeypointList::Keypoint::actualRadiusFromPreviewSize(const QSize & size) const
{
  if (radius >= 0) {
    return (int)roundf(radius);
  } else {
    return (int)roundf(-radius * (sqrtf(size.width() * size.width() + size.height() * size.height())) / 100.0f);
  }
}

#endif // _ZART_KEYPOINTLIST_H_
