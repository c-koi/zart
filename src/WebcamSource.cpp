/** -*- mode: c++ ; c-basic-offset: 2 -*-
 * @file   WebcamSource.cpp
 * @author Sebastien Fourey
 * @date   July 2010
 * @brief Definition of methods of the class WebcamGrabber
 *
 * This file is part of the ZArt software's source code.
 *
 * Copyright Sebastien Fourey / GREYC Ensicaen (2010-...)
 *
 *                    https://foureys.users.greyc.fr/
 *
 * This software is a computer program whose purpose is to demonstrate
 * the possibilities of the GMIC image processing language by offering the
 * choice of several manipulations on a video stream acquired from a webcam. In
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
#include "WebcamSource.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QList>
#include <QSettings>
#include <QSplashScreen>
#include <QStatusBar>
#include <QStringList>
#include <QtGlobal>
#include <algorithm>
#include <chrono>
#include <set>
#include <thread>
#include "Common.h"
using namespace std;

#if CV_MAJOR_VERSION >= 3
#define ZART_CV_CAP_PROP_FRAME_WIDTH cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH
#define ZART_CV_CAP_PROP_FRAME_HEIGHT cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT
#else
#define ZART_CV_CAP_PROP_FRAME_WIDTH CV_CAP_PROP_FRAME_WIDTH
#define ZART_CV_CAP_PROP_FRAME_HEIGHT CV_CAP_PROP_FRAME_HEIGHT
#endif

#if (CV_MAJOR_VERSION > 3) || ((CV_MAJOR_VERSION == 3) && (CV_MINOR_VERSION > 4)) || ((CV_MAJOR_VERSION == 3) && (CV_MINOR_VERSION == 4) && (CV_SUBMINOR_VERSION >= 4))
#define CVCAPTURE_HAS_BACKEND_METHOD
#endif

#ifdef HAS_V4L2
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

QVector<QList<QSize>> WebcamSource::_webcamResolutions;
QList<int> WebcamSource::_webcamList;

namespace
{
class QSizeCompare {
public:
  bool operator()(const QSize & a, const QSize & b) const { return ((a.width() < b.width()) || ((a.width() == b.width()) && (a.height() < b.height()))); }
};
} // namespace

WebcamSource::WebcamSource() : _capture(nullptr), _cameraIndex(-1), _captureSize(640, 480) {}

WebcamSource::~WebcamSource()
{
  if (_capture) {
    delete _capture;
    _capture = nullptr;
  }
}

void WebcamSource::capture()
{
  cv::Mat * anImage = new cv::Mat;
  if (_capture && _capture->read(*anImage)) {
    setImage(anImage);
  } else {
    delete anImage;
  }
}

const QList<int> & WebcamSource::getWebcamList()
{
  _webcamList.clear();
#if defined(HAS_V4L2)
  for (int i = 0; i <= 10; ++i) {
    QString filename = QString("/dev/video%1").arg(i);
    if (!QFileInfo(filename).isReadable()) {
      continue;
    }
    int fd = open(filename.toLocal8Bit().constData(), O_RDWR);
    if (fd != -1) {

      bool isCapture = false;
      v4l2_capability cap;
      if (!ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (cap.capabilities & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
          qDebug("[Device %d] is a capture", i);
          isCapture = true;
        }
      }

      bool camera = false;
      v4l2_input input;
      input.index = 0;
      while (!ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
        if (input.type == V4L2_INPUT_TYPE_CAMERA) {
          qDebug("[Device %d] is a camera", i);
          camera = true;
        }
        ++input.index;
      }

      close(fd);
      if (isCapture && camera) {
        _webcamList.push_back(i);
      }
    }
  }
#elif defined(_IS_UNIX_)
  const QString os = osName();
  const bool skipOdd = (os == "Fedora" || os == "FreeBSD");
  for (int i = 0; i <= 10; i += (1 + skipOdd)) {
    QFile file(QString("/dev/video%1").arg(i));
    if (file.open(QFile::ReadOnly)) {
      file.close();
      cv::VideoCapture capture;
      capture.open(i);
      if (captureIsValid(capture, i)) {
        _webcamList.push_back(i);
      }
      capture.release();
    }
  }
#else
  cv::VideoCapture * capture;
  for (int i = 0; i <= 10; ++i) {
    capture = new cv::VideoCapture;
    if (capture && capture->open(i)) {
      capture->release();
      delete capture;
      _webcamList.push_back(i);
    }
  }
#endif
  TSHOW(_webcamList);
  return _webcamList;
}

int WebcamSource::getFirstUnusedWebcam()
{
  if (!_webcamList.size()) {
    getWebcamList();
  }
  QList<int>::iterator it = _webcamList.begin();
  while (it != _webcamList.end()) {
    TRACE << "Looking for first unused webcam : " << *it;
    if (isWebcamUnused(*it)) {
      return *it;
    }
    ++it;
  }
  return -1;
}

bool WebcamSource::isWebcamUnused(int index)
{
  try {
    cv::VideoCapture capture;
#if defined(_IS_UNIX_)
    bool canOpenFile = false;
    {
      QFile file(QString("/dev/video%1").arg(index));
      if (file.open(QFile::ReadOnly)) {
        canOpenFile = true;
        file.close();
      }
    }
#else
    const bool canOpenFile = true;
#endif
    if (canOpenFile && capture.open(index)) {
      cv::Mat cvImage;
      TRACE << "Trying to read an image from webcam " << index;
      if (capture.read(cvImage)) {
        std::cout << "[ZArt] Webcam " << index << " is available (" << cvImage.cols << "x" << cvImage.rows << ")\n";
        return true;
      }
    }
  } catch (const cv::Exception &) {
    TRACE << "DONE (exception) index " << index;
  }
  TRACE << "DONE index " << index;
  std::cout << "[ZArt] Cannot use webcam " << index << std::endl;
  return false;
}

bool WebcamSource::canOpenDeviceFile(int index)
{
#if defined(_IS_UNIX_)
  QFile file(QString("/dev/video%1").arg(index));
  if (file.open(QFile::ReadOnly)) {
    file.close();
    return true;
  }
  return false;
#else
  return true;
#endif
}

const QList<int> & WebcamSource::getCachedWebcamList()
{
  return _webcamList;
}

void WebcamSource::setCameraIndex(int i)
{
  _cameraIndex = i;
}

void WebcamSource::stop()
{
  if (_capture) {
    delete _capture;
    _capture = nullptr;
  }
}

void WebcamSource::start()
{
  if (!_capture && _cameraIndex != -1) {
    _capture = new cv::VideoCapture;
    cv::Mat * capturedImage = new cv::Mat;
    if (_capture && _capture->open(_cameraIndex)) {
      try {
        _capture->read(*capturedImage); // TODO : Check
      } catch (cv::Exception &) {
        capturedImage = nullptr;
      }
    } else {
      delete capturedImage;
      capturedImage = nullptr;
    }
    setImage(capturedImage);
    if (image()) {
      QSize imageSize(image()->cols, image()->rows);
      if (imageSize != _captureSize) {
        _capture->set(ZART_CV_CAP_PROP_FRAME_WIDTH, _captureSize.width());
        _capture->set(ZART_CV_CAP_PROP_FRAME_HEIGHT, _captureSize.height());
        cv::Mat * anImage = new cv::Mat;
        // Read two images!
        _capture->grab();
        // _capture->read(*anImage);
        if (_capture->read(*anImage)) {
          setImage(anImage);
        } else {
          delete anImage;
        }
      }
      setWidth(image()->cols);
      setHeight(image()->rows);
    }
  }
}

void WebcamSource::setCaptureSize(int width, int height)
{
  setCaptureSize(QSize(width, height));
}

void WebcamSource::setCaptureSize(const QSize & size)
{
  _captureSize = size;
  if (_capture) {
    stop();
    start();
  }
}

QSize WebcamSource::captureSize()
{
  return _captureSize;
}

int WebcamSource::cameraIndex()
{
  return _cameraIndex;
}

void WebcamSource::retrieveWebcamResolutions(const QList<int> & camList, QSplashScreen * splashScreen, QStatusBar * statusBar)
{
#if defined(HAS_V4L2)
  retrieveWebcamResolutionsV4L2(camList);
  Q_UNUSED(splashScreen)
  Q_UNUSED(statusBar)
#else
  retrieveWebcamResolutionsOpenCV(camList, splashScreen, statusBar);
#endif
  return;
}

void WebcamSource::retrieveWebcamResolutionsOpenCV(const QList<int> & camList, QSplashScreen * splashScreen, QStatusBar * statusBar)
{
  QSettings settings;
  QList<std::set<QSize, QSizeCompare>> resolutions;
  QList<int>::const_iterator it = camList.begin();
  int iCam = 0;
  while (it != camList.end()) {
    resolutions.push_back(std::set<QSize, QSizeCompare>());
    cv::VideoCapture capture;
    if (canOpenDeviceFile(*it) && capture.open(*it) && captureIsValid(capture, *it)) {
      QStringList resolutionsStrList = settings.value(QString("WebcamSource/ResolutionsListForCam%1").arg(camList[iCam])).toStringList();
      bool settingsAreFine = !resolutionsStrList.isEmpty();
      for (int i = 0; i < resolutionsStrList.size() && settingsAreFine; ++i) {
        QStringList str = resolutionsStrList.at(i).split(QChar('x'));
        int w = str[0].toInt();
        int h = str[1].toInt();
        bool ok1 = capture.set(ZART_CV_CAP_PROP_FRAME_WIDTH, w);
        bool ok2 = capture.set(ZART_CV_CAP_PROP_FRAME_HEIGHT, h);
        if (!ok1 || !ok2) {
          continue;
        }
        cv::Mat tmpA;
        capture.read(tmpA);
        QSize size(static_cast<int>(capture.get(ZART_CV_CAP_PROP_FRAME_WIDTH)), static_cast<int>(capture.get(ZART_CV_CAP_PROP_FRAME_HEIGHT)));
        if (size == QSize(w, h)) {
          resolutions.back().insert(size);
        } else {
          settingsAreFine = false;
        }
      }

      if (!settingsAreFine) {
        if (splashScreen) {
          splashScreen->showMessage(QString("Retrieving webcam #%1 resolutions...\n(almost-brute-force checking!)\nResult will be saved.").arg(iCam + 1), Qt::AlignBottom);
          qApp->processEvents();
        }
        if (statusBar) {
          statusBar->showMessage(QString("Retrieving webcam #%1 resolutions...").arg(iCam + 1), Qt::AlignBottom);
          qApp->processEvents();
        }
        resolutions.back().clear();

        // Default size ?
        {
          cv::Mat tmp;
          capture.read(tmp);
          QSize defaultSize(static_cast<int>(capture.get(ZART_CV_CAP_PROP_FRAME_WIDTH)), static_cast<int>(capture.get(ZART_CV_CAP_PROP_FRAME_HEIGHT)));
          if (defaultSize.isValid() && !defaultSize.isEmpty()) {
            resolutions.back().insert(defaultSize);
          }
        }

        int ratioWidth[] = {4, 16, 0};
        int ratioHeight[] = {3, 9, 0};
        int widths[] = {320, 640, 800, 1024, 1280, 1600, 1920, 0};

        for (int i = 0; widths[i]; ++i) {
          int w = widths[i];
          for (int ratio = 0; ratioWidth[ratio]; ++ratio) {
            int h = w * ratioHeight[ratio] / ratioWidth[ratio];
            QSize requestedSize(w, h);
            try {
              cv::Mat tmp;
              capture.read(tmp);
              capture.set(ZART_CV_CAP_PROP_FRAME_WIDTH, w);
              capture.set(ZART_CV_CAP_PROP_FRAME_HEIGHT, h);
              QSize size(static_cast<int>(capture.get(ZART_CV_CAP_PROP_FRAME_WIDTH)), static_cast<int>(capture.get(ZART_CV_CAP_PROP_FRAME_HEIGHT)));
              if ((splashScreen || statusBar) && resolutions.back().find(size) == resolutions.back().end()) {
                if (splashScreen) {
                  splashScreen->showMessage(QString("Retrieving webcam #%1 resolutions... %2x%3\n(brute-force checking!)\nResult will be saved.").arg(iCam + 1).arg(size.width()).arg(size.height()),
                                            Qt::AlignBottom);
                }
                if (statusBar) {
                  statusBar->showMessage(QString("Retrieving webcam #%1 resolutions... %2x%3").arg(iCam + 1).arg(size.width()).arg(size.height()));
                }
                qApp->processEvents();
              }
              if (size.isValid() && !size.isNull()) {
                resolutions.back().insert(size);
              }
            } catch (cv::Exception &) {
              std::cerr << "Cannot set capture size " << w << "x" << h << std::endl;
            }
          }
        }
      }
    } else {
      if (splashScreen) {
        splashScreen->showMessage(QString("Webcam #%1 cannot be opened of used.").arg(iCam + 1), Qt::AlignBottom);
        qApp->processEvents();
      }
      if (statusBar) {
        statusBar->showMessage(QString("Webcam #%1 cannot be opened or used.").arg(iCam + 1), Qt::AlignBottom);
        qApp->processEvents();
      }
      resolutions.back().clear();
    }
    ++it;
    ++iCam;
  }

  qDebug("Done checking resolutions");

  _webcamResolutions.clear();
  _webcamResolutions.resize(camList.size());
  iCam = 0;
  QList<std::set<QSize, QSizeCompare>>::iterator itCam = resolutions.begin();
  while (itCam != resolutions.end()) {
    QStringList resolutionsList;
    std::set<QSize, QSizeCompare>::iterator itRes = itCam->begin();
    while (itRes != itCam->end()) {
      _webcamResolutions[iCam].push_back(*itRes);
      resolutionsList << QString("%1x%2").arg(itRes->width()).arg(itRes->height());
      ++itRes;
    }
    settings.setValue(QString("WebcamSource/ResolutionsListForCam%1").arg(camList[iCam]), resolutionsList);
    ++itCam;
    ++iCam;
  }
}

void WebcamSource::retrieveWebcamResolutionsV4L2(const QList<int> & camList)
{
  ENTERING;
#if defined(HAS_V4L2)
  QSettings settings;
  QList<std::set<QSize, QSizeCompare>> resolutions;
  QList<int>::const_iterator it = camList.begin();
  int iCam = 0;
  for (; it != camList.end(); ++it, ++iCam) {
    resolutions.push_back(std::set<QSize, QSizeCompare>());
    QString filename = QString("/dev/video%1").arg(*it);
    if (!QFileInfo(filename).isReadable()) {
      continue;
    }
    int fd = open(filename.toLocal8Bit().constData(), O_RDWR);
    if (fd != -1) {
      // Is a camera?
      bool camera = false;
      v4l2_input input;
      input.index = 0;
      while (!ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
        if (input.type == V4L2_INPUT_TYPE_CAMERA) {
          qDebug("[Device %d] is a camera", *it);
          camera = true;
        }
        ++input.index;
      }
      if (camera) {
        v4l2_fmtdesc fmt;
        fmt.index = 0;
        fmt.type = V4L2_CAP_VIDEO_CAPTURE;
        QList<unsigned int> pixelFormats;
        while (!ioctl(fd, VIDIOC_ENUM_FMT, &fmt)) {
          qDebug("[Device %d] pixel format %c%c%c%c", *it, (char)(fmt.pixelformat & 0xFF), (char)((fmt.pixelformat & 0xFF00) >> 8), (char)((fmt.pixelformat & 0xFF0000) >> 16),
                 (char)((fmt.pixelformat & 0xFF000000) >> 24));
          pixelFormats += fmt.pixelformat;
          ++fmt.index;
        }
        for (unsigned int pixelformat : pixelFormats) {
          v4l2_frmsizeenum framesize;
          framesize.index = 0;
          framesize.pixel_format = pixelformat;
          while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &framesize)) {
            if (framesize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
              QSize size(framesize.discrete.width, framesize.discrete.height);
              resolutions.back().insert(size);
            }
            ++framesize.index;
          }
        }
      }
      close(fd);
    }
  }
  qDebug("Done checking resolutions");
  _webcamResolutions.clear();
  _webcamResolutions.resize(camList.size());
  iCam = 0;
  QList<std::set<QSize, QSizeCompare>>::iterator itCam = resolutions.begin();
  while (itCam != resolutions.end()) {
    QStringList resolutionsList;
    std::set<QSize, QSizeCompare>::iterator itRes = itCam->begin();
    while (itRes != itCam->end()) {
      _webcamResolutions[iCam].push_back(*itRes);
      resolutionsList << QString("%1x%2").arg(itRes->width()).arg(itRes->height());
      ++itRes;
    }
    settings.setValue(QString("WebcamSource/ResolutionsListForCam%1").arg(camList[iCam]), resolutionsList);
    ++iCam;
    ++itCam;
  }
#else
  Q_UNUSED(camList)
#endif
}

const QList<QSize> & WebcamSource::webcamResolutions(int index)
{
  static const QList<QSize> empty;
  return index >= 0 ? _webcamResolutions[index] : empty;
}

void WebcamSource::clearSavedSettings()
{
  QSettings settings;
  for (int i = 0; i < 50; ++i) {
    settings.remove(QString("WebcamSource/ResolutionsListForCam%1").arg(i));
    settings.remove(QString("WebcamSource/DefaultResolutionCam%1").arg(i));
  }
}

QString WebcamSource::osName()
{
#ifdef _IS_FREEBSD_
  return "FreeBSD";
#else
  QFile file("/etc/os-release");
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning("Warning: Cannot determine the os name (/etc/os-release file missing?)");
    return "Unknown";
  }
  while (file.bytesAvailable()) {
    QString line = file.readLine().trimmed();
    if (line.startsWith("NAME=")) {
      line.replace("NAME=", "");
      if (line.size() && line[0] == '"') {
        line.remove(0, 1);
      }
      if (line.size() && line[line.size() - 1] == '"') {
        line.chop(0);
      }
      return line;
    }
  }
  return "Unknown";
#endif
}

bool WebcamSource::captureIsValid(const cv::VideoCapture & capture, int index)
{
#if defined(HAS_V4L2)
  // WebcamSource::getWebcamList() already returns valid webcams only
  Q_UNUSED(capture)
  Q_UNUSED(index)
  return true;
#elif defined(CVCAPTURE_HAS_BACKEND_METHOD)
  Q_UNUSED(index)
  return std::string("UNICAP") != capture.getBackendName().c_str();
#else
  Q_UNUSED(capture)
  QString os = osName();
  return (os != "Fedora" && os != "FreeBSD") || !(index % 2);
#endif
}
