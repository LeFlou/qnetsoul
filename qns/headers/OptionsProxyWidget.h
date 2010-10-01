/*
  Copyright 2010 Dally Richard
  This file is part of QNetSoul.
  QNetSoul is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  QNetSoul is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with QNetSoul.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPTIONS_PROXY_WIDGET_H_
#define OPTIONS_PROXY_WIDGET_H_

#include <QDir>
#include <QWidget>
#include <QNetworkProxy>
#include "AbstractOptions.h"

class   OptionsProxyWidget : public QWidget, public AbstractOptions
{
  Q_OBJECT

    public:
  OptionsProxyWidget(QWidget* parent = 0);
  ~OptionsProxyWidget(void);

  void enableProxy(void);
  void readOptions(QSettings& settings);
  void writeOptions(QSettings& settings);
  void updateOptions(void);
  void saveOptions(void);
  void setProxy(void);
  bool validFields(void) const;

 private:
  bool    _useProxy;
  QString _proxy;
  QString _proxyPort;
  QString _proxyLogin;
  QString _proxyPassword;
};

#endif
