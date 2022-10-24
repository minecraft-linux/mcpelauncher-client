#pragma once

#include <string>
#include <vector>

class XalWebView {
public:
    virtual ~XalWebView() {}

    virtual std::string show(std::string starturl, std::string endurlprefix) = 0;
};
