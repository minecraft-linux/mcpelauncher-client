#pragma once

#include "xal_webview.h"
#include <memory>

class XalWebViewFactory {

public:
  static std::unique_ptr<XalWebView> createXalWebView();
};