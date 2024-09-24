#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h>

using namespace std;
using namespace cv;
typedef unsigned char uchar;

string SERVER_IP = "127.0.0.1";
int PORT = 8080, WIDTH = 1280, HEIGHT = 720, BUFFER_SIZE = 1024, CAMS = 1;

int args(int argc, char* argv[]);

int main(int argc, char* argv[]){
    if(args(argc, argv)) return 1;

    int client_socket = 0, attempt = 1;
    struct sockaddr_in server_addr;
    char recv_buffer[BUFFER_SIZE];

    while(1){
        cout << "[i] Initializing client...\n";
        
        if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            cout << "[e] Socket creation failed\n";
            return 1;
        }
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(8080);
        if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0){
            cout << "[e] Invalid address or address not supported\n";
            return 1;
        }

        if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
            int err = errno;
            cout << "[e] Connection failed. Error Code: " << err << '\n';
            close(client_socket);
            cout << "[w] Attempt " << attempt << ". Restarting in 3 seconds...\n";
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
            if(bytes_sent < 0){
                int err = errno;
                cout << "[e] Send failed. Error Code: " << err << '\n';
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
                cout << "[e] Receive failed\n";
                break;
            }
        }

        close(client_socket);
        cout << "[w] Attempt " << attempt << ". Restarting in 3 seconds...\n";
        attempt++;
    }
    
    cout << "[i] Shutting down client...\n";
    close(client_socket);
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
                    PORT = stoi(argv[++i]);
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
                    WIDTH = stoi(argv[++i]);
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
                    HEIGHT = stoi(argv[++i]);
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
                    CAMS = stoi(argv[++i]);
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
                    BUFFER_SIZE = stoi(argv[++i]);
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