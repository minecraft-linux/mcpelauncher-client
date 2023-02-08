#include "lib_http_client_websocket.h"
#include "../util.h"
void HttpClientWebSocket::connect(std::shared_ptr<FakeJni::JString> url, std::shared_ptr<FakeJni::JString> subProtocol) {
    FakeJni::LocalFrame frame;
    auto method = getClass().getMethod("(Jcom/xbox/httpclient/HTTPClientWebSocket;)V", "onOpen");
    method->invoke(frame.getJniEnv(), this, call_handle, frame.getJniEnv().createLocalReference(this->shared_from_this()));
}
