#include <iostream>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace cv;

string SERVER_IP = "127.0.0.1";
int PORT = 8080, WIDTH = 1280, HEIGHT = 720, BUFFER_SIZE = 1024, CAMS = 1, QUALITY = 50;

int args(int argc, char* argv[]);

int main(int argc, char* argv[]){
    if(args(argc, argv)) return 1;

    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    //char recv_buffer[BUFFER_SIZE];
    char* recv_buffer = new char[BUFFER_SIZE];
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
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());
        server_addr.sin_port = htons(PORT);

        if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
            cout << "[e] Connection failed. Error Code: " << WSAGetLastError() << '\n';
            closesocket(client_socket);
            cout << "[w] Attempt " << attempt << ". Restarting in 3 seconds...\n";
            Sleep(3000);
            attempt++;
            continue;
        }
        cout << "[i] Connected to server\n";

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
                cout << "[e] Failed to capture frame\n";
                break;
            }
            imencode(".jpg", frame, img_buffer, {IMWRITE_JPEG_QUALITY, QUALITY});

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

int args(int argc, char* argv[]){
    for(int i = 1; i < argc; ++i){
        string arg = argv[i];
        if(arg == "--ip" || arg == "-i"){
            if(i+1 < argc) SERVER_IP = argv[++i];
            else{
                cout << "[e] --ip requires an ip address\n";
                return 1;
            }
        }
        else if(arg == "--port" || arg == "-p"){
            if(i+1 < argc){
                try{
                    PORT = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --port invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --port requires a port number\n";
                return 1;
            }
        }
        else if(arg == "--width" || arg == "-w"){
            if(i+1 < argc){
                try{
                    WIDTH = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --width invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --width requires a horizontal resolution\n";
                return 1;
            }
        }
        else if(arg == "--height" || arg == "-h"){
            if(i+1 < argc){
                try{
                    HEIGHT = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --height invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --height requires a vertical resolution\n";
                return 1;
            }
        }
        else if(arg == "--cams" || arg == "-c"){
            if(i+1 < argc){
                try{
                    CAMS = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --cams invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --cams requires a camera amount\n";
                return 1;
            }
        }
        else if(arg == "--buffer" || arg == "-b"){
            if(i+1 < argc){
                try{
                    BUFFER_SIZE = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --buffer invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --buffer requires a camera amount\n";
                return 1;
            }
        }
        else if(arg == "--help" || arg == "-h"){
            cout << "Options\n  -h\t\t\t= Displays available options\n  -i <address>\t\t= Server ip address\n  -p <number>\t\t= Server TCP port number\n  -w <pixels>\t\t= Video horizontal resolution\n  -h <pixels>\t\t= Video vertical resolution\n  -c <number>\t\t= Number of camera inputs to transmit\n  -b <bytes>\t\t= Received messages buffer size\n";
            return 1;
        }
        else{
            cout << "[e] Invalid argument detected\n\nOptions\n  -h\t\t\t= Displays available options\n  -i <address>\t\t= Server IP address\n  -p <number>\t\t= Server TCP port number\n  -w <pixels>\t\t= Horizontal video resolution\n  -h <pixels>\t\t= Vertical video resolution\n  -c <number>\t\t= Amount of camera inputs\n  -b <bytes>\t\t= Received messages buffer size\n";
            return 1;
        }
    }
    return 0;
}