/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   KeypointList.cpp
 * @author Sebastien Fourey
 * @date   June 2018
 * @brief  Definition of the class KeypointList
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
#include "KeypointList.h"
#include <cmath>
#include <cstring>

KeypointList::KeypointList() {}

void KeypointList::add(const KeypointList::Keypoint & keypoint)
{
  _keypoints.push_back(keypoint);
}

bool KeypointList::isEmpty()
{
  return _keypoints.empty();
}

void KeypointList::clear()
{
  _keypoints.clear();
}

QPointF KeypointList::position(int n) const
{
  const Keypoint & kp = _keypoints[n];
  return QPointF(kp.x, kp.y);
}

QColor KeypointList::color(int n) const
{
  return _keypoints[n].color;
}

bool KeypointList::isRemovable(int n) const
{
  return _keypoints[n].removable;
}

KeypointList::Keypoint::Keypoint(float x, float y, QColor color, bool removable, float radius, bool keepOpacityWhenSelected)
    : x(x), y(y), color(color), removable(removable), radius(radius), keepOpacityWhenSelected(keepOpacityWhenSelected)
{
}

KeypointList::Keypoint::Keypoint(QPointF point, QColor color, bool removable, float radius, bool keepOpacityWhenSelected)
    : x((float)point.x()), y((float)point.y()), color(color), removable(removable), radius(radius), keepOpacityWhenSelected(keepOpacityWhenSelected)
{
}

KeypointList::Keypoint::Keypoint(QColor color, bool removable, float radius, bool keepOpacityWhenSelected)
    : color(color), removable(removable), radius(radius), keepOpacityWhenSelected(keepOpacityWhenSelected)
{
  setNaN();
}

bool KeypointList::Keypoint::isNaN() const
{
  if (sizeof(float) == 4) {
    unsigned int ix, iy;
    std::memcpy(&ix, &x, sizeof(float));
    std::memcpy(&iy, &y, sizeof(float));
    return ((ix & 0x7fffffff) > 0x7f800000) || ((iy & 0x7fffffff) > 0x7f800000);
  }
#ifdef isnan
  return (isnan(x) || isnan(y));
#else
  return !(x == x) || !(y == y);
#endif
}

KeypointList::Keypoint & KeypointList::Keypoint::setNaN()
{
#ifdef NAN
  x = y = (float)NAN;
#else
  const double nanValue = -std::sqrt(-1.0);
  x = y = (float)nanValue;
#endif
  return *this;
}
