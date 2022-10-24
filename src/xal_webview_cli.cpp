#include "xal_webview_cli.h"
#include <iostream>

std::string XalWebViewCli::show(std::string starturl, std::string endurlprefix) {
    std::cout << "Please open \"" << starturl << "\" in your Webbrowser and sign in\n";
    std::cout << "Paste the final url starting with\"" << endurlprefix << "\" to finish sign in:\n";
    std::string result;
    std::getline(std::cin, result);
    return result;
}
