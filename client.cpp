#include <iostream>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace cv;

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
#define QUALITY 40
#define WIDTH 1280
#define HEIGHT 720

int main(){
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char recv_buffer[BUFFER_SIZE];
    int attempt = 1;

    while(1){
        cout << "[i] Initializing client...\n";
        if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
            cout << "[e] Failed to initialize Winsock. Error Code: " << WSAGetLastError() << '\n';
            return 1;
        }
        if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
            cout << "[e] Socket creation failed. Error Code: " << WSAGetLastError() << '\n';
            return 1;
        }
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(PORT);

        if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
            cout << "[e] Connection failed. Error Code: " << WSAGetLastError() << '\n';
            closesocket(client_socket);
            cout << "[w] Attempt " << attempt << ". Restarting in 3 seconds...\n";
            Sleep(3000);
            attempt++;
            continue;
        }
        cout << "[i] Connected to server" << '\n';

        cout << "[i] Initializing capture device...\n";
        VideoCapture cap(0);
        if(!cap.isOpened()){
            cout << "[e] Error opening camera" << '\n';
            return 1;
        }
        cap.set(CAP_PROP_FRAME_WIDTH, WIDTH);
        cap.set(CAP_PROP_FRAME_HEIGHT, HEIGHT);
        Mat frame;
        vector<uchar> img_buffer;

        while(1){
            cap >> frame;
            if(frame.empty()){
                cout << "[e] Failed to capture frame" << '\n';
                break;
            }
            vector<int> compression_params = { IMWRITE_JPEG_QUALITY, QUALITY };
            imencode(".jpg", frame, img_buffer, compression_params);

            int bytes_sent = send(client_socket, reinterpret_cast<const char*>(img_buffer.data()), img_buffer.size(), 0);
            if(bytes_sent == SOCKET_ERROR){
                cout << "[e] Send failed. Error Code: " << WSAGetLastError() << '\n';
                break;
            }

            int bytes_received = recv(client_socket, recv_buffer, BUFFER_SIZE, 0);
            if(bytes_received > 0){
                string reply(recv_buffer, bytes_received);
                if(reply != "400") cout << "[w] Unexpected reply from server\n";
            }
            else if(bytes_received == 0){
                cout << "[w] Connection closed by server\n";
                break;
            }
            else{
                cout << "[e] Receive failed. Error Code: " << WSAGetLastError() << '\n';
                break;
            }

            if(GetAsyncKeyState('Q') & 0x8000){
                cout << "[i] Shutting down client...\n";
                closesocket(client_socket);
                WSACleanup();
                return 0;
            }
        }

        closesocket(client_socket);
        WSACleanup();
        cout << "[w] Attempt " << attempt << ". Restarting in 3 seconds...\n";
        Sleep(3000);
        attempt++;
    }
    
    cout << "[i] Shutting down client...\n";
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
