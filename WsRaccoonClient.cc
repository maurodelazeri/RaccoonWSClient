//
// Created by Mauro Delazeri on 2019-05-15.
//

#include <utility>
#include "WsRaccoonClient.h"
#include "iostream"

WsRaccoonClient::WsRaccoonClient(std::string *address, int port, std::string *path, WsClientCallback *callback) {

    this->address = std::string(*address);
    this->port = port;
    this->path = std::string(*path);

    cout << "[ws Reconnection] Connecting to ==> " << this->address << ":" << this->port << this->path << endl;

    this->pt = nullptr;
    this->stop_ = false;
    this->noMsg = true;
    this->lwsCallback = callback;

    protocols_[0] = {
            "zinnion-protocol",
            on_websocket_callback,
            0,
            20,
            0,
            this,
            0
    };

    protocols_[1] = {
            nullptr,
            nullptr,
            0,
            0
    };
}

//Join operation to ensure that the thread exits
WsRaccoonClient::~WsRaccoonClient() {
    // Try to stop
    stop();
}

int WsRaccoonClient::on_websocket_callback(struct lws *wsi,
                                           enum lws_callback_reasons reason,
                                           void *user_data,
                                           void *in,
                                           size_t len) {

    auto *self = (WsRaccoonClient *) user_data;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            cout << "[ws connection succeeded]" << endl;
            if (self->lwsCallback) {
                self->lwsCallback->onConnSuccess(nullptr);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            cout << "[ws error connecting]" << endl;
            if (self->lwsCallback)
                self->lwsCallback->onConnError();
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            cout << "[ws Connection closed]" << endl;
            self->wsClient = nullptr;
            if (self->lwsCallback)
                self->lwsCallback->onConnClosed();
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            if (len > 0) {
                self->message.append((const char *) in, len);
                if (lws_is_final_fragment(wsi)) {
                    self->lwsCallback->onReceiveMessage(&self->message);
                    self->message.clear();
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (!self->messageQueue.empty()) {
                // Send a message in the message queue
                std::string *message = self->messageQueue.pop();
                unsigned char buf[LWS_PRE + message->length() + LWS_SEND_BUFFER_POST_PADDING];
                unsigned char *p = &buf[LWS_PRE];
                auto n = static_cast<size_t>(sprintf((char *) p, "%s", message->c_str()));
                int ret = lws_write(wsi, p, n, LWS_WRITE_TEXT);
                delete message;
                self->noMsg = self->messageQueue.empty();
            } else {
                cout << "Send keep-alive message" << endl;
                uint8_t ping[LWS_PRE + 125];
                lws_write(wsi, ping + LWS_PRE, 0, LWS_WRITE_PING);
            }
            break;
        }
        case LWS_CALLBACK_WSI_DESTROY: {
            cout << "[ws Connection destruction]" << endl;
            self->wsClient = nullptr;
            break;
        }
        default: {
//            RTC_LOG(LERROR) << "on_websocket_callback" << reason;
            break;
        }
    }
    return 0;
}

void WsRaccoonClient::sendMessage(std::string *message) {
    auto *msg = new std::string(*message);
    messageQueue.push(msg);
    noMsg = false;
    if (wsClient) {
        lws_callback_on_writable(wsClient);
    }
}

void WsRaccoonClient::netThread(WsRaccoonClient *pSelf) {
    pSelf->netTask();
}

/**
 * Network read and write process
 */
void WsRaccoonClient::netTask() {
    cout << "[ws Thread start] " << pthread_self() << endl;

    struct lws_context_creation_info ctxInfo{};
    memset(&ctxInfo, 0, sizeof(ctxInfo));

    ctxInfo.port                     = CONTEXT_PORT_NO_LISTEN;
    ctxInfo.iface                    = NULL;
    ctxInfo.protocols                = protocols_;
    ctxInfo.ssl_cert_filepath        = NULL;
    ctxInfo.ssl_private_key_filepath = NULL;
    ctxInfo.extensions               = nullptr;
    ctxInfo.gid                      = -1;
    ctxInfo.uid                      = -1;
    ctxInfo.options                 |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    ctxInfo.fd_limit_per_thread      = 1024;
    ctxInfo.max_http_header_pool     = 1024;
    ctxInfo.ws_ping_pong_interval    = 10;
    ctxInfo.ka_time                  = 10;
    ctxInfo.ka_probes                = 10;
    ctxInfo.ka_interval              = 10;

    struct lws_context *wsContext = lws_create_context(&ctxInfo);

    if (wsContext == nullptr) {
        cout << "[ERROR] lws_create_context() failure" << endl;
        cout << "[ws Thread exit]" << endl;
        return;
    }

    struct lws_client_connect_info clientInfo{};
    memset(&clientInfo, 0, sizeof(clientInfo));
    clientInfo.context = wsContext;
    clientInfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    clientInfo.host = lws_canonical_hostname(wsContext);
    clientInfo.address = this->address.c_str();
    clientInfo.port = this->port;
    clientInfo.path = this->path.c_str();
    clientInfo.origin = "origin";
    clientInfo.ietf_version_or_minus_one = -1;
    clientInfo.protocol = protocols_[0].name;
    clientInfo.userdata = this;
    wsClient = lws_client_connect_via_info(&clientInfo);

    int countdown = 5;
    while (!stop_ || !noMsg) {
        if (!wsClient) {
            sleep(1);
            --countdown;
            if (stop_) break;
            if (countdown == 0) {
                cout << "[ws Reconnection] Reconnection to ==> " << this->address << ":" << this->port << this->path << endl;
                wsClient = lws_client_connect_via_info(&clientInfo);
                countdown = 5;
            } else {
                continue;
            }
        }
        lws_service(wsContext, 50);
    }

    lws_context_destroy(wsContext);
    wsClient = nullptr;

    cout << "[ws Thread exit]" << endl;
}

void WsRaccoonClient::start() {
    if (pt != nullptr) {
        cout << "[ERROR] ws Thread is already running" << endl;
    } else {
        pt = new std::thread(netThread, this);
    }
}

void WsRaccoonClient::stop() {
    stop_ = true;
    if (pt && pt->joinable()) {
        pt->join();
    }
    if (pt) {
        delete pt;
        pt = nullptr;
    }
}