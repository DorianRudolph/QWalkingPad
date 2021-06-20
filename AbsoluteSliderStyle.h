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

