#include <iostream>
#include <opencv2/opencv.hpp>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace cv;

string SERVER_IP = "127.0.0.1";
bool VERBOSE = false;
int PORT = 8080, WIDTH = 1280, HEIGHT = 720, BUFFER_SIZE = 1024, QUALITY = 50, MODE = 0;
vector<int> CAMS{0};

int args(int argc, char* argv[]);

int main(int argc, char* argv[]){
    if(args(argc, argv)) return 1;

    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    vector<char> recv_buffer(BUFFER_SIZE);
    int attempt = 1;

    cout << "[i] Initializing capture devices...\n";
    vector<VideoCapture> sources(CAMS.size());
    Mat frame;
    vector<uchar> img_buffer;
    int bytes_received, bytes_sent, packet_number = 0;
    for(int i = 0; i < CAMS.size(); i++){
        sources[i].open(CAMS[i]);
        if(!sources[i].isOpened()){
            cout << "[e] Error opening camera " << i << '\n';
            return 1;
        }
        sources[i].set(CAP_PROP_FRAME_WIDTH, WIDTH);
        sources[i].set(CAP_PROP_FRAME_HEIGHT, HEIGHT);
    }

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

        while(1){
            bytes_received = recv(client_socket, recv_buffer.data(), BUFFER_SIZE, 0);
            if(bytes_received > 0){
                string reply(recv_buffer.data(), bytes_received);
                if(reply == "401"){
                    waitKey(1000);
                    int handshake[5] = {0, WIDTH, HEIGHT, MODE, CAMS.size()};
                    bytes_sent = send(client_socket, (char*)handshake, sizeof(handshake), 0);
                    if(bytes_sent == SOCKET_ERROR){
                        cout << "[e] Send failed. Error Code: " << WSAGetLastError() << '\n';
                        break;
                    }
                    cout << "[i] Handshake replied\n";
                }
                else if(reply == "400"){
                    if(MODE == 0){
                        sources[0] >> frame;
                        if(frame.empty()){
                            cout << "[e] Failed to capture frame\n";
                            break;
                        }
                        imencode(".jpg", frame, img_buffer, {IMWRITE_JPEG_QUALITY, QUALITY});
                        img_buffer.insert(img_buffer.begin(), char(packet_number));
                        img_buffer.insert(img_buffer.begin(), char(1));
                    }
                    else if(MODE == 1){
                        vector<uchar> frame_buffer;
                        img_buffer.insert(img_buffer.begin(), char(packet_number));
                        img_buffer.insert(img_buffer.begin(), char(1));
                        for(int i = 0; i < CAMS.size(); i++){
                            int frame_size;
                            sources[i] >> frame;
                            if(frame.empty()){
                                cout << "[e] Failed to capture frame from source " << i << '\n';
                                break;
                            }
                            imencode(".jpg", frame, frame_buffer, {IMWRITE_JPEG_QUALITY, QUALITY});
                            frame_size = frame_buffer.size();
                            uchar size_buffer[sizeof(int)];
                            memcpy(size_buffer, &frame_size, sizeof(int));
                            img_buffer.insert(img_buffer.end(), size_buffer, size_buffer+sizeof(int));
                            img_buffer.insert(img_buffer.end(), frame_buffer.begin(), frame_buffer.end());
                            frame_buffer.clear();
                        }
                    }
                    else if(MODE >= 2){
                        cout << "WIP\n";
                        return 0;
                    }
                    else{
                        cout << "[e] Mode error: " << MODE << '\n';
                        return 1;
                    }
                    bytes_sent = send(client_socket, reinterpret_cast<const char*>(img_buffer.data()), img_buffer.size(), 0);
                    cout << "[send] " << fixed << setprecision(2) << img_buffer.size()/1000.0 << " kB\t" << "packet #" << packet_number << '\n';
                    if(bytes_sent == SOCKET_ERROR){
                        cout << "[e] Send failed. Error Code: " << WSAGetLastError() << '\n';
                        break;
                    }
                    packet_number++;
                    packet_number %= 100;
                    recv_buffer.clear();
                    img_buffer.clear();
                }
                else cout << "[w] Unexpected reply from server: " << reply << '\n';
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
                for(int i = 0; i < CAMS.size(); i++){
                    sources[i].release();
                }
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
    for(int i = 0; i < CAMS.size(); i++){
        sources[i].release();
    }
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
            int n = 0;
            if(i+3 <= argc){
                try{
                    n = atoi(argv[i+1]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --cams invalid number\n";
                    return 1;
                }
                if(argc < i+n){
                    cout << "[e] Incomplete camera list\n";
                    return 1;
                }
                CAMS.clear();
                for(int j = 0; j < n; j++){
                    CAMS.push_back(atoi(argv[i+j+2]));
                }
            }
            else{
                cout << "[e] --cams requires a camera list\n";
                return 1;
            }
            i += n+1;
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
        else if(arg == "--mode" || arg == "-m"){
            if(i+1 < argc){
                try{
                    MODE = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --mode invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --mode requires a number\n";
                return 1;
            }
        }
        else if(arg == "--quality" || arg == "-q"){
            if(i+1 < argc){
                try{
                    QUALITY = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --quality invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --quality requires a camera amount\n";
                return 1;
            }
        }
        else if(arg == "--help" || arg == "-H"){
            cout << "Options\n  -v\t\t\t= Verbose output\n  -H\t\t\t= Displays available options\n  -i <address>\t\t= Server ip address\n  -p <number>\t\t= Server TCP port number\n  -w <pixels>\t\t= Video horizontal resolution\n  -h <pixels>\t\t= Video vertical resolution\n  -c <number> <list>\t\t= Camera inputs to transmit\n  -q <number>\t\t= Transmission video quality (0-100)\n  -b <bytes>\t\t= Received messages buffer size\n  -m <number>\t\t= Transmission mode (see README)\n";
            return 1;
        }
        else if(arg == "--verbose" || arg == "-v") VERBOSE = true;
        else{
            cout << "[e] Invalid argument detected\n\nOptions\n  -v\t\t\t= Verbose output\n  -H\t\t\t= Displays available options\n  -i <address>\t\t= Server ip address\n  -p <number>\t\t= Server TCP port number\n  -w <pixels>\t\t= Video horizontal resolution\n  -h <pixels>\t\t= Video vertical resolution\n  -c <number> <list>\t\t= Camera inputs to transmit\n  -q <number>\t\t= Transmission video quality (0-100)\n  -b <bytes>\t\t= Received messages buffer size\n  -m <number>\t\t= Transmission mode (see README)\n";
            return 1;
        }
    }
    return 0;
}