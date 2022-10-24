#include "xal_webview_qt.h"
#include <EnvPathUtil.h>
#include <array>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstring>
#include <errno.h>
#include <game_window_manager.h>
#include "util.h"

std::string XalWebViewQt::findWebView() {
    std::string path;
#ifdef MCPELAUNCHER_WEBVIEW_PATH
    if(EnvPathUtil::findInPath("mcpelauncher-webview", path, MCPELAUNCHER_WEBVIEW_PATH, EnvPathUtil::getAppDir().c_str()))
        return path;
#endif
    if(EnvPathUtil::findInPath("mcpelauncher-webview", path))
        return path;
    return std::string();
}

std::string XalWebViewQt::exec_get_stdout(std::string path, std::string title, std::string description) {
    char ret[1024];

    int pipes[3][2];
    static const int PIPE_STDOUT = 0;
    static const int PIPE_STDERR = 1;
    static const int PIPE_STDIN = 2;
    static const int PIPE_READ = 0;
    static const int PIPE_WRITE = 1;

    pipe(pipes[PIPE_STDOUT]);
    pipe(pipes[PIPE_STDERR]);
    pipe(pipes[PIPE_STDIN]);
    int pid;
    if(!(pid = fork())) {
        auto argv = buildCommandLine(path, title, description);
        auto argvc = convertToC(argv);
        dup2(pipes[PIPE_STDOUT][PIPE_WRITE], STDOUT_FILENO);
        dup2(pipes[PIPE_STDERR][PIPE_WRITE], STDERR_FILENO);
        dup2(pipes[PIPE_STDIN][PIPE_READ], STDIN_FILENO);
        close(pipes[PIPE_STDIN][PIPE_WRITE]);
        close(pipes[PIPE_STDOUT][PIPE_WRITE]);
        close(pipes[PIPE_STDERR][PIPE_WRITE]);
        close(pipes[PIPE_STDIN][PIPE_READ]);
        close(pipes[PIPE_STDOUT][PIPE_READ]);
        close(pipes[PIPE_STDERR][PIPE_READ]);
        int r = execv(argvc[0], (char**)argvc.data());
        printf("Show: execv() error %i %s", r, strerror(errno));
        _exit(r);
    } else if(pid != -1) {
        close(pipes[PIPE_STDIN][PIPE_WRITE]);
        close(pipes[PIPE_STDIN][PIPE_READ]);

        close(pipes[PIPE_STDOUT][PIPE_WRITE]);
        close(pipes[PIPE_STDERR][PIPE_WRITE]);

        std::string outputStdOut;
        std::string outputStdErr;
        ssize_t r;
        if((r = read(pipes[PIPE_STDOUT][PIPE_READ], ret, 1024)) > 0)
            outputStdOut += std::string(ret, (size_t)r);
        if((r = read(pipes[PIPE_STDERR][PIPE_READ], ret, 1024)) > 0)
            outputStdErr += std::string(ret, (size_t)r);

        close(pipes[PIPE_STDOUT][PIPE_READ]);
        close(pipes[PIPE_STDERR][PIPE_READ]);

        int status;
        waitpid(pid, &status, 0);
        status = WEXITSTATUS(status);
        if(status == 0) {
            return outputStdOut;
        } else {
            throw std::runtime_error("Process exited with status=" + std::to_string(status) + " stdout:`" + outputStdOut + "` stderr:`" + outputStdErr + "`");
        }
    } else {
        throw std::runtime_error("Fork failed");
    }
}

std::string XalWebViewQt::show(std::string starturl, std::string endurlprefix) {
    try {
        auto webview_path = findWebView();
        if(webview_path.empty()) {
            throw std::runtime_error("mcpelauncher-webview not found");
        }
        auto result = exec_get_stdout(webview_path, starturl, endurlprefix);
        trim(result);
        // valid character list took from https://developers.google.com/maps/documentation/urls/url-encoding#special-characters
        // added space, because it isn't url encoded on account creation
        auto validUrlChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~!*'();:@&=+$,/?%#[] ";
        if((result.rfind(endurlprefix, 0) != 0 || result.find_first_not_of(validUrlChars, 0) != std::string::npos) && !result.empty()) {
            auto iurl = result.find(endurlprefix);
            if(iurl != std::string::npos) {
                auto eurl = result.find_first_not_of(validUrlChars, iurl);
                auto url = result.substr(iurl, eurl);
                GameWindowManager::getManager()->getErrorHandler()->onError("XalWebViewQt", "The Launcher might failed to open the Xboxlive login Window successfully, please report if Minecraft tells you that sign in failed. Please look into the gamelog for more Information: Process returned stdout:`" + result + "`, but expected an url starting with:`" + endurlprefix + "`. Try returning `" + url + "` as endurl. We track this issue here https://github.com/minecraft-linux/mcpelauncher-manifest/issues/444");
                return url;
            }
            throw std::runtime_error("Process returned stdout:`" + result + "`, but expected an url starting with:`" + endurlprefix + "`");
        }
        return result;
    } catch(const std::exception& ex) {
        GameWindowManager::getManager()->getErrorHandler()->onError("XalWebViewQt", std::string("Failed to open Xboxlive login Window. Please look into the gamelog for more Information: ") + ex.what());
        return "";
    }
}

std::vector<std::string> XalWebViewQt::buildCommandLine(std::string path, std::string title, std::string description) {
    std::vector<std::string> cmd;
    cmd.emplace_back(path);
    cmd.emplace_back(title);
    cmd.emplace_back(description);
    return std::move(cmd);
}

std::vector<const char*> XalWebViewQt::convertToC(std::vector<std::string> const& v) {
    std::vector<const char*> ret;
    for(auto const& i : v)
        ret.push_back(i.c_str());
    ret.push_back(nullptr);
    return std::move(ret);
}
