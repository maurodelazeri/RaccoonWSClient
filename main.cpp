#include "WsRaccoonClient.h"
#include "string"
#include "iostream"

using namespace std;

class App : public WsClientCallback {

private:
    WsRaccoonClient *wsClient;

public:
    void onConnSuccess(std::string *sessionId) override {
        cout << "connection succeeded" << endl;
        std::string ban = "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"diff_order_book_btcusd\" } }";
        wsClient->sendMessage(&ban);
    }

    void onConnClosed() override {
        cout << "Connection closed" << endl;
    }

    void onConnError() override {
        cout << "Connection error" << endl;

    }

    void onReceiveMessage(std::string *message) override {
        cout << message->c_str() << endl;
    };

    App() {
        wsClient = nullptr;
    }

    void start() {
        std::string *address = new std::string("ws.bitstamp.net");
        std::string *path = new std::string("/");
        wsClient = new WsRaccoonClient(address, 443, path, this);
        delete address;
        delete path;

        wsClient->start();

        sleep(10);

        wsClient->stop();
        delete wsClient;
        wsClient = nullptr;
    }
};

int main(int args, char **argv) {
    (new App())->start();
    return 0;
}
