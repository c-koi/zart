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
#include "WebcamSource.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QList>
#include <QSettings>
#include <QSplashScreen>
#include <QStatusBar>
#include <QStringList>
#include <algorithm>
#include <set>
#include "Common.h"
using namespace std;

#if CV_MAJOR_VERSION >= 3
#define ZART_CV_CAP_PROP_FRAME_WIDTH cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH
#define ZART_CV_CAP_PROP_FRAME_HEIGHT cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT
#else
#define ZART_CV_CAP_PROP_FRAME_WIDTH CV_CAP_PROP_FRAME_WIDTH
#define ZART_CV_CAP_PROP_FRAME_HEIGHT CV_CAP_PROP_FRAME_HEIGHT
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
#if defined(_IS_UNIX_)
  int i = 0;
  for (i = 0; i <= 10; ++i) {
    QFile file(QString("/dev/video%1").arg(i));
    if (file.open(QFile::ReadOnly)) {
      file.close();
      _webcamList.push_back(i);
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
  return _webcamList;
}

int WebcamSource::getFirstUnusedWebcam()
{
  if (!_webcamList.size()) {
    getWebcamList();
  }
  QList<int>::iterator it = _webcamList.begin();
  while (it != _webcamList.end()) {
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
#if defined(_IS_UNIX_)
  QString os = osName();
#else
  QString os("NotUnix");
#endif

  QSettings settings;

  QList<int>::const_iterator it = camList.begin();
  QList<std::set<QSize, QSizeCompare>> resolutions;
  int iCam = 0;
  while (it != camList.end()) {
    resolutions.push_back(std::set<QSize, QSizeCompare>());
    cv::VideoCapture capture;
#if defined(_IS_UNIX_)
    bool canOpenFile = false;
    {
      QFile file(QString("/dev/video%1").arg(*it));
      if (file.open(QFile::ReadOnly)) {
        canOpenFile = true;
        file.close();
      }
    }
    if (os == "Fedora" && *it % 2) { // Fedora seems to create two devices per webcam!
      canOpenFile = false;
    }
#else
    const bool canOpenFile = true;
#endif
    if (canOpenFile && capture.open(*it)) {
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

        int ratioWidth[] = {16, 11, 8, 5, 4, 0};
        int ratioHeight[] = {9, 9, 5, 4, 3, 0};
        int widths[] = {100, 120, 140, 160, 180, 200, 220, 240, 280, 300, 320, 360, 400, 600, 640, 700, 720, 800, 900, 1100, 1200, 1300, 1500, 1800, 2000, 2100, 0};
        //        int ratioWidth[] = {4, 16, 0};
        //        int ratioHeight[] = {3, 9, 0};
        //        int widths[] = {640, 800, 1024, 0};

        for (int i = 0; widths[i]; ++i) {
          int w = widths[i];
          for (int ratio = 0; ratioWidth[ratio]; ++ratio) {
            int h = w * ratioHeight[ratio] / ratioWidth[ratio];
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
        splashScreen->showMessage(QString("Webcam #%1 is busy (default to 640x480 resolution)...").arg(iCam + 1), Qt::AlignBottom);
        qApp->processEvents();
      }
      if (statusBar) {
        statusBar->showMessage(QString("Webcam #%1 is busy (default to 640x480 resolution)...").arg(iCam + 1), Qt::AlignBottom);
        qApp->processEvents();
      }
      resolutions.back().insert(QSize(640, 480));
    }
    ++it;
    ++iCam;
  }

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
    if (_webcamResolutions[iCam].isEmpty()) {
      _webcamResolutions[iCam].push_back(QSize(640, 480));
      resolutionsList << QString("640x480");
    }
    settings.setValue(QString("WebcamSource/ResolutionsListForCam%1").arg(camList[iCam]), resolutionsList);
    ++itCam;
    ++iCam;
  }
}

const QList<QSize> & WebcamSource::webcamResolutions(int index)
{
  return _webcamResolutions[index];
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
  QFile file("/etc/os-release");
  if (!file.open(QIODevice::ReadOnly)) {
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
}
