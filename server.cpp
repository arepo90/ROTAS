#include <opencv2/opencv.hpp>
#include <iostream>
#include <sys/types.h>
#include <winsock2.h>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace cv;
using namespace chrono;

#define PORT 8080
#define BUFFER_SIZE 921632 //230432
#define WIDTH 1280
#define HEIGHT 720

int main(){
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        cout << "Failed to initialize Winsock. Error Code: " << WSAGetLastError() << '\n';
        return 1;
    }
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
        cout << "Socket creation failed. Error Code: " << WSAGetLastError() << '\n';
        return 1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(::bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
        cout << "Bind failed. Error Code: " << WSAGetLastError() << '\n';
        closesocket(server_socket);
        return 1;
    }

    listen(server_socket, 3);
    cout << "Waiting for incoming connections...\n";

    if((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len)) == INVALID_SOCKET){
        cout << "Accept failed. Error Code: " << WSAGetLastError() << '\n';
        closesocket(server_socket);
        return 1;
    }
    cout << "Connection accepted\n";

    char buffer[BUFFER_SIZE];
    int bytes_received;
    vector<uchar> img_data;
    auto prev = high_resolution_clock().now();

    while(true){
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if(bytes_received == SOCKET_ERROR){
            cout << "Receive failed. Error Code: " << WSAGetLastError() << '\n';
            break;
        }
        else if(bytes_received == 0){
            cout << "Connection closed by client\n";
        }
        cout << bytes_received/1000.0 << " kB\t";

        auto curr = high_resolution_clock().now();
        auto duration = duration_cast<milliseconds>(curr-prev);
        prev = high_resolution_clock().now();
        cout << 1000.0/duration.count() << " fps\n";

        img_data.insert(img_data.end(), buffer, buffer + bytes_received);
        if(bytes_received < BUFFER_SIZE){
            Mat img = imdecode(img_data, IMREAD_COLOR);
            if(!img.empty()) {
                imshow("Received image", img);
                waitKey(1);
            }
            img_data.clear();

            string reply = "400";
            send(client_socket, reply.c_str(), reply.size(), 0);
        }
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    waitKey(0);
    return 0;
}
