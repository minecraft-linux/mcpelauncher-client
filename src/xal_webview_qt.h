#pragma once

#include "xal_webview.h"

class XalWebViewQt : public XalWebView {
    std::string findWebView();
    std::string exec_get_stdout(const char* command);
public:
    virtual std::string show(std::string starturl, std::string endurlprefix) override;
};