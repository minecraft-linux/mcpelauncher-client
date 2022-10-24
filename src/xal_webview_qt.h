#pragma once

#include "xal_webview.h"
#include <string>
#include <vector>

class XalWebViewQt : public XalWebView {
    std::string findWebView();
    std::string exec_get_stdout(std::string path, std::string title, std::string description);
    std::vector<std::string> buildCommandLine(std::string path, std::string title, std::string description);
    std::vector<const char *> convertToC(const std::vector<std::string> &v);

public:
    virtual std::string show(std::string starturl, std::string endurlprefix) override;
};
