/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   ImageConverter.h
 * @author Sebastien Fourey
 * @date   Jul 2010
 * @brief  Declaration of the class ImageFilter
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
#ifndef _IMAGECONVERTER_H_
#define _IMAGECONVERTER_H_

#include <QMutex>
#include <opencv2/opencv.hpp>
#include "gmic.h"

class QImage;

class ImageConverter {
public:
  enum MergeDirection
  {
    MergeTop,
    MergeLeft,
    MergeBottom,
    MergeRight,
    DuplicateVertical,
    DuplicateHorizontal
  };

  static void convert(const cv::Mat * in, QImage * out);
  static void convert(const QImage & in, cv::Mat ** out);
  static void convert(const cv::Mat * in, cimg_library::CImg<float> & out);
  static void convert(const cimg_library::CImg<float> & in, QImage * out);
  static void merge(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out, QMutex * imageMutex, MergeDirection direction);
  static void mergeTop(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out);
  static void mergeLeft(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out);
  static void mergeBottom(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out, bool shift = false);
  static void mergeRight(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out, bool shift = true);
  static void duplicateVertical(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out);
  static void duplicateHorizontal(cv::Mat * cvImage, const cimg_library::CImg<float> & cimgImage, QImage * out);

private:
  static cv::Mat * _image;
};

#endif // _IMAGECONVERTER_H_
