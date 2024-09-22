#include <iostream>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <chrono>
#include <iomanip>
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
    int addr_len = sizeof(client_addr), attempt = 1;

    while(1){
        cout << "[i] Initializing server...\n";

        if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
            cout << "[e] Failed to initialize Winsock. Error Code: " << WSAGetLastError() << '\n';
            return 1;
        }
        if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
            cout << "[e] Socket creation failed. Error Code: " << WSAGetLastError() << '\n';
            return 1;
        }
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if(::bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
            cout << "[e] Bind failed. Error Code: " << WSAGetLastError() << '\n';
            closesocket(server_socket);
            return 1;
        }

        listen(server_socket, 3);
        cout << "[i] Waiting for connections...\n";

        if((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len)) == INVALID_SOCKET){
            cout << "[e] Accept failed. Error Code: " << WSAGetLastError() << '\n';
            closesocket(server_socket);
            return 1;
        }
        cout << "[i] Connection accepted\n";

        char buffer[BUFFER_SIZE];
        int bytes_received;
        vector<uchar> img_data;
        auto prev = high_resolution_clock().now();

        cout << "[i] Waiting for incoming messages...\n";
        while(1){
            bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if(bytes_received == SOCKET_ERROR){
                cout << "[e] Receive failed. Error Code: " << WSAGetLastError() << '\n';
                break;
            }
            else if(bytes_received == 0){
                cout << "[w] Connection closed by client\n";
                break;
            }
            cout << "[recv] " << fixed << setprecision(2) << bytes_received/1000.0 << " kB\t";

            auto curr = high_resolution_clock().now();
            auto duration = duration_cast<milliseconds>(curr-prev);
            prev = high_resolution_clock().now();
            cout << "@ " << fixed << setprecision(2) << 1000.0/duration.count() << " fps\n";

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

            if(GetAsyncKeyState('Q') & 0x8000){
                cout << "[i] Shutting down server...\n";
                closesocket(client_socket);
                closesocket(server_socket);
                WSACleanup();
                return 0;
            }
        }

        closesocket(client_socket);
        closesocket(server_socket);
        WSACleanup();

        cout << "[w] Attempt " << attempt << ". Restarting in 3 seconds...\n";
        Sleep(3000);
        attempt++;
    }

    cout << "[i] Shutting down server...\n";
    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
