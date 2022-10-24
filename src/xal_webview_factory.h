#pragma once

#include <memory>
#include "xal_webview.h"

class XalWebViewFactory {
public:
    static std::unique_ptr<XalWebView> createXalWebView();
};
