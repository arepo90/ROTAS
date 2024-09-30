#include <opencv2/opencv.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace cv;

vector<pair<int, int>> PACKETS;
int PORT = 8080, NUM_CAMS = 1, MODE = 0;
bool VERBOSE = false;

int args(int argc, char* argv[]);
bool handshake(SOCKET client_socket);
void cnlog(const string& str, int lvl);
void handleClient(SOCKET client_socket, int index);

int main(int argc, char* argv[]){
    if(args(argc, argv)) return -1;
    cnlog("[i] Initializing server...", 2);

    WSADATA wsaData;
    vector<SOCKET> server_sockets(NUM_CAMS);
    vector<thread> client_threads(NUM_CAMS);

    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        cnlog("[e] Failed to initialize Winsock", 0);
        return -1;
    }

    cnlog("[i] Initializing socket threads...", 2);
    for(int i = 0; i < NUM_CAMS; i++){
        server_sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        if(server_sockets[i] == INVALID_SOCKET){
            cnlog("[e] Could not create server socket", 0);
            WSACleanup();
            return -1;
        }
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT + i);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        if(::bind(server_sockets[i], (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
            cnlog("[e] Could not bind to port " + to_string(PORT+i), 0);
            closesocket(server_sockets[i]);
            WSACleanup();
            return -1;
        }
        if(listen(server_sockets[i], 5) == SOCKET_ERROR){
            cnlog("[e] Could not listen on port " + to_string(PORT+i), 0);
            closesocket(server_sockets[i]);
            WSACleanup();
            return -1;
        }
        cnlog("[i] Server listening on port " + to_string(PORT+i) + " for source " + to_string(i) + "...", 2);
    }

    for(int i = 0; i < NUM_CAMS; i++){
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_sockets[i], (sockaddr*)&client_addr, &client_len);
        if(client_socket == INVALID_SOCKET){
            cnlog("[e] Could not accept client connection", 0);
            closesocket(server_sockets[i]);
            WSACleanup();
            return -1;
        }
        if(i == 0 && !handshake(client_socket)){
            cnlog("[i] Shutting down server...", 2);
            closesocket(client_socket);
            for(int j = 0; j < NUM_CAMS; j++){
                closesocket(server_sockets[j]);
            }
            WSACleanup();
            return -1;
        }
        client_threads[i] = thread(handleClient, client_socket, i);
    }

    cnlog("[i] Shutting down server...", 2);
    for(int i = 0; i < NUM_CAMS; i++){
        if(client_threads[i].joinable()) client_threads[i].join();
        closesocket(server_sockets[i]);
    }
    WSACleanup();
    return 0;
}

void cnlog(const string& str, int lvl){
    if(VERBOSE || lvl == 0) cout << str << '\n';
}

bool handshake(SOCKET client_socket){
    cnlog("[i] Initializing handshake...", 2);
    int handshakeMessage[3], handshakeAck = 400;
    if(recv(client_socket, (char*)handshakeMessage, sizeof(handshakeMessage), 0) <= 0){
        cnlog("[e] Did not receive handshake", 0);
        return false;
    }
    if(handshakeMessage[0] != 0){
        cnlog("[e] Invalid handshake message", 0);
        return false;
    }
    MODE = handshakeMessage[1];
    NUM_CAMS = handshakeMessage[2];
    PACKETS.resize(NUM_CAMS, {-1, 0});
    if(send(client_socket, (char*)&handshakeAck, sizeof(handshakeAck), 0) == SOCKET_ERROR){
        cnlog("[e] Could not send handshake acknowledgment", 0);
        return false;
    }
    cnlog("[i] Handshake complete", 2);
    return true;
}

void handleClient(SOCKET client_socket, int index){
    int metadata[2];
    vector<uchar> buffer;
    while(1){
        int bytesReceived = recv(client_socket, (char*)metadata, sizeof(metadata), 0);
        if(bytesReceived <= 0) break;
        buffer.resize(metadata[0]);
        if(PACKETS[index].first == 99){
            PACKETS[index].first = -1;
            PACKETS[index].second = 0;
        }
        else if(int(metadata[1]) != PACKETS[index].first+1) PACKETS[index].second++;
        PACKETS[index].first = metadata[1];
        bytesReceived = recv(client_socket, (char*)buffer.data(), metadata[0], 0);
        if(bytesReceived <= 0) break;
        Mat frame = imdecode(buffer, IMREAD_COLOR);
        if(frame.empty()) continue;
        imshow("Source " + to_string(index), frame);
        stringstream stream;
        stream << "[recv" << index << "] " << fixed << setprecision(2) << bytesReceived/1000.0 << " kB\t" << PACKETS[index].second << "% packet loss";
        cnlog(stream.str(), 2);
        if(waitKey(1) == 'q') break;
    }
    closesocket(client_socket);
}

int args(int argc, char* argv[]){
    for(int i = 1; i < argc; ++i){
        string arg = argv[i];
        if(arg == "--port" || arg == "-p"){
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
        else if(arg == "--help" || arg == "-H"){
            cout << "Options\n  -v\t\t\t= Verbose output\n  -H\t\t\t= Displays available options\n  -p <number>\t\t= Server TCP port number\n";
            return 1;
        }
        else if(arg == "--cams" || arg == "-c"){
            if(i+1 < argc){
                try{
                    NUM_CAMS = atoi(argv[++i]);
                }
                catch(const invalid_argument&){
                    cout << "[e] --cams invalid number\n";
                    return 1;
                }
            }
            else{
                cout << "[e] --cams requires a source number\n";
                return 1;
            }
        }
        else if(arg == "--verbose" || arg == "-v") VERBOSE = true;
        else{
            cout << "[e] Invalid argument detected\n\nOptions\n  -v\t\t\t= Verbose output\n  -H\t\t\t= Displays available options\n  -p <number>\t\t= Server TCP port number\n";
            return 1;
        }
    }
    return 0;
}