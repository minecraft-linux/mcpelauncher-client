#include "xal_webview_factory.h"

#include <stdexcept>

#if defined(XAL_WEBVIEW_USE_QT)
#include "xal_webview_qt.h"
std::unique_ptr<XalWebView> XalWebViewFactory::createXalWebView() {
    return std::unique_ptr<XalWebView>(new XalWebViewQt());
}
#elif defined(XAL_WEBVIEW_USE_CLI)
#include "xal_webview_cli.h"
std::unique_ptr<XalWebView> XalWebViewFactory::createXalWebView() {
    return std::unique_ptr<XalWebView>(new XalWebViewCli());
}
#else
std::unique_ptr<XalWebView> XalWebViewFactory::createXalWebView() {
    throw std::runtime_error("No XalWebView implementation available");
}
#endif
