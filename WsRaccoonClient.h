#include "libwebsockets.h"
#include <string>
#include <map>
#include "queue.h"

#include <list>

using namespace std;

class WsClientCallback {
public:
    virtual void onConnSuccess(std::string *sessionId) = 0;

    virtual void onConnClosed() = 0;

    virtual void onConnError() = 0;

    virtual void onReceiveMessage(std::string *message) = 0;

    virtual ~WsClientCallback() = default;
};

class WsRaccoonClient {

public:
    WsRaccoonClient(std::string *address, int port, std::string *path, WsClientCallback *callback);

    ~WsRaccoonClient();

    void start();

    void stop();

    void sendMessage(std::string *message);

private:

    std::string address;
    int port;
    std::string path;

    // ws Network sending and receiving thread
    std::thread *pt;

    // It will be used when the network is reconnected.
    struct lws *wsClient;

    // Thread exit flag stop && noMsg
    volatile bool stop_;

    // noMsg is used to ensure that messages leaving the room are sent out before the thread exits
    volatile bool noMsg;

    struct lws_protocols protocols_[2];

    // Used to store messages received by the network
    std::string message;

    //Pending message queue
    Queue<std::string *> messageQueue;

    // Callback, please do not handle time-consuming operations in callbacks
    WsClientCallback *lwsCallback;

private:

    static int on_websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
                                     void *user_data, void *in, size_t len);

    static void netThread(WsRaccoonClient *pSelf);

    void netTask();
};