#include "webview.h"
#include "../xal_webview_factory.h"

void WebView::showUrl(FakeJni::JLong l, std::shared_ptr<Context> ctx, std::shared_ptr<FakeJni::JString> starturl, std::shared_ptr<FakeJni::JString> endurl, FakeJni::JInt i, FakeJni::JBoolean z, FakeJni::JLong j2) {
    auto webview = XalWebViewFactory::createXalWebView();
    auto finalendurl = webview->show(starturl->asStdString(), endurl->asStdString());
    auto method = WebView::getDescriptor()->getMethod("(JLjava/lang/String;ZLjava/lang/String;)V", "urlOperationSucceeded");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), WebView::getDescriptor().get(), l, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(finalendurl)), false, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>("webkit-noDefault::0::none")));
}