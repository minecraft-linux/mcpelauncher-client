#pragma once

#include "xal_webview.h"

class XalWebViewCli : public XalWebView {
public:
    virtual std::string show(std::string starturl, std::string endurlprefix) override;
};
