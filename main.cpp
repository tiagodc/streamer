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

#include <thread>
#include "main.h"

using namespace seasocks;
std::string latest_img = "blablabla";

void multipleStream(cv::VideoCapture* cap, WebSocket* con, Server* server){

      if(!cap->isOpened()){   
          std::cout << "Input error" << std::endl;
          return;
      }else{
          cv::Mat frame;
          for(;;){
              *cap >> frame;
              std::vector<uchar> buf;
              cv::imencode(".jpg", frame, buf);
              auto *enc_msg = reinterpret_cast<unsigned char*>(buf.data());
              std::string encoded = base64Encode(enc_msg, buf.size());
              server->execute([con, encoded]{ con->send(error_img.c_str()); });
          }
      }
}

void imgUpdate(cv::VideoCapture* cap){
    if(!cap->isOpened()){   
        std::cout << "Input error" << std::endl;
        latest_img = error_img;
        return;
    }
    
    cv::Mat frame;
    for(;;){
        *cap >> frame;
        std::vector<uchar> buf;
        cv::imencode(".jpg", frame, buf);
        auto *enc_msg = reinterpret_cast<unsigned char*>(buf.data());
        std::string encoded = base64Encode(enc_msg, buf.size());
        latest_img = encoded;
    }
}

void imgSender(WebSocket* con, Server* server){    
    for(;;){
        if(!con) return;
        server->execute([con]{ con->send(latest_img.c_str()); });
        std::chrono::milliseconds interval(10);
        std::this_thread::sleep_for(interval);
    }
}

void imgSendAll(std::set<WebSocket*>* cons, Server* server){    
    for(;;){
        for(auto* con : *cons){
            if(con) server->execute([con]{ con->send(latest_img.c_str()); });            
        }
        std::chrono::milliseconds interval(10);
        std::this_thread::sleep_for(interval);
    }
}

class MyHandler : public WebSocket::Handler {
public:
    explicit MyHandler(Server* server): _server(server) { }

    void onConnect(WebSocket* connection) override {
        _connections.insert(connection);
        // std::thread stream_parallel(imgSender, connection, _server);
        // stream_parallel.detach();
    }

    void onData(WebSocket* connection, const char* data) override {
        if (0 == strcmp("die", data)) {
            _server->terminate();
            return;
        }
        if (0 == strcmp("close", data)) {
            std::cout << "Closing..\n";
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

// private:
    std::set<WebSocket*> _connections;
    Server* _server;
};

int main(int argc, char **argv)
{ 

    if(argc != 3){
        std::cout << "\n### PANTERA STREAMER ###\n" << std::endl;
        std::cout << "São necessários 2 argumentos no seguinte formato:\n" << std::endl;
        std::cout << "./streamer '<input_rtsp>' <output_port>\n" << std::endl;
        std::cout << "Exemplo:" << std::endl;
        std::cout << "$  ./streamer rtsp://localhost:8554/mystream 9090" << std::endl;
        std::cout << ">> info: Listening on http://hostname:9090/\n" << std::endl;
        return -1;
    }

    // const char* rtsp = "rtsp://service:!S1ntecsy5@200.205.247.132:10002/rtsp_tunnel";
    const char* rtsp = argv[1];
    int port = atoi(argv[2]);

    cv::VideoCapture cap(rtsp, cv::CAP_FFMPEG);
    if(!cap.isOpened()){
        std::cout << "ERRO: não foi possível abrir captura no endereço: " << rtsp << std::endl;
        return -1;
    }

    std::thread img_stream(imgUpdate, &cap);
    img_stream.detach();

    auto logger = std::make_shared<PrintfLogger>(Logger::Level::Debug);
    Server server(logger);

    auto handler = std::make_shared<MyHandler>(&server);
    std::thread stream_parallel(imgSendAll, &(handler->_connections), &server);
    stream_parallel.detach();

    server.addWebSocketHandler("/ws", handler);
    server.serve("", port);
    return 0;
}