#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"
#include "seasocks/util/Json.h"

#include "internal/Base64.h"

using namespace seasocks;

bool CAM_MONITOR = true;

class MyHandler : public WebSocket::Handler {
public:
    explicit MyHandler(Server* server): _server(server) { }

    void onConnect(WebSocket* connection) override {
        _connections.insert(connection);
      cv::VideoCapture cap("rtsp://localhost:8554/mystream", cv::CAP_FFMPEG);

      if(!cap.isOpened()){   
          std::cout << "Input error\n";
          std::string status = "error opening streaming";           
          connection->send(status.c_str());
      }else{
          cv::Mat frame;
          for(;;){
              cap >> frame;
              std::vector<uchar> buf;
              cv::imencode(".jpg", frame, buf);
              auto *enc_msg = reinterpret_cast<unsigned char*>(buf.data());
              std::string encoded = base64Encode(enc_msg, buf.size());
              connection->send(encoded.c_str());
              if(!CAM_MONITOR) break;
          }   
      }
    }

    void onData(WebSocket* connection, const char* data) override {
        if (0 == strcmp("die", data)) {
            _server->terminate();
            return;
        }
        if (0 == strcmp("close", data)) {
            std::cout << "Closing..\n";
            CAM_MONITOR = false;
            connection->close();
            std::cout << "Closed.\n";
            return;
        }
    }

    void onDisconnect(WebSocket* connection) override {
        _connections.erase(connection);
        std::cout << "Disconnected: " << connection->getRequestUri()
                  << " : " << formatAddress(connection->getRemoteAddress()) << "\n";
    }

private:
    std::set<WebSocket*> _connections;
    Server* _server;
};

int main(int argc, char **argv)
{
    int port = atoi(argv[1]);    
    auto logger = std::make_shared<PrintfLogger>(Logger::Level::Debug);
    Server server(logger);

    auto handler = std::make_shared<MyHandler>(&server);
    server.addWebSocketHandler("/ws", handler);
    server.serve("", port);
    return 0;
}