// QWalkingPad - Simple desktop application for controlling the Kingsmith WalkingPad over BLE
// Copyright (C) 2021 Dorian Rudolph
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <QProxyStyle>

class AbsoluteSliderStyle : public QProxyStyle {
public:
  using QProxyStyle::QProxyStyle;

  int styleHint(QStyle::StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                QStyleHintReturn *returnData = 0) const {
    if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
      return (Qt::LeftButton);
    if (hint == QStyle::SH_Slider_PageSetButtons)
      return (Qt::MiddleButton | Qt::RightButton);
    return QProxyStyle::styleHint(hint, option, widget, returnData);
  }
};

