#include <opencv2/opencv.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <errno.h>
#include <vector>

using namespace cv;
using namespace std;

string SERVER_IP = "127.0.0.1";
vector<int> CAMS{0};
int PORT = 8080, MODE = 0, WIDTH = 1280, HEIGHT = 720, QUALITY = 75;
bool VERBOSE = false;

int args(int argc, char* argv[]);
bool sendPacket(int socket_fd, vector<uchar>& buffer, int packetNumber);
bool handshake(int socket_fd);
//void cnlog(const string& str, int lvl);

int main(int argc, char** argv) {
    if(args(argc, argv)) return -1;
    cnlog("[i] Initializing client...", 2);

    vector<int> socket_fds(CAMS.size());
    vector<VideoCapture> sources(CAMS.size());
    vector<vector<uchar>> buffers(CAMS.size());
    vector<int> packet_numbers(CAMS.size(), 0);

    for (int i = 0; i < CAMS.size; i++){
        sources[i].open(i);
        if (!sources[i].isOpened()) {
            cerr << "Error: Could not open webcam " << i << endl;
            return -1;
        }

        socket_fds[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fds[i] < 0) {
            cerr << "Error: Could not create socket for webcam " << i << endl;
            return -1;
        }

        int flags = fcntl(socket_fds[i], F_GETFL, 0);
        fcntl(socket_fds[i], F_SETFL, flags | O_NONBLOCK);

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT + i);  // Different port for each camera
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        connect(socket_fds[i], (sockaddr*)&server_addr, sizeof(server_addr));
    }

    if(!handshake(socket_fds[0])){
        for(int j = 0; j < CAMS.size(); j++){
            close(socket_fds[0]);
            sources[j].release();
        }
        return -1;
    }

    // Main event loop
    while (true) {
        for (int i = 0; i < NUM_CAMS; ++i) {
            Mat frame;
            sources[i] >> frame;  // Capture frame

            if (frame.empty()) {
                cerr << "Warning: Webcam " << i << " captured an empty frame" << endl;
                continue;
            }

            // Compress frame
            buffers[i].clear();
            imencode(".jpg", frame, buffers[i], {IMWRITE_JPEG_QUALITY, QUALITY});

            // Attempt to send the frame if possible (non-blocking send)
            if(!sendPacket(socket_fds[i], buffers[i], packet_numbers[i]++)){
                //cnlog("[w] Failed to send frame from source " + to_string(i), 1);
                return 1;
            }
            packet_numbers[i] %= 100;
        }

        // Simulate other background tasks
        // Insert your additional processes here

        // Limit frame rate (adjust to your needs)
        usleep(30000);
    }

    // Clean up
    for (int i = 0; i < NUM_CAMS; ++i) {
        sources[i].release();
        close(socket_fds[i]);
    }

    return 0;
}

bool sendPacket(int socket_fd, vector<uchar>& buffer, int packetNumber){
    int metadata[] = {buffer.size(), packet_number};
    if(send(socket_fd, (char*)metadata, sizeof(metadata), MSG_NOSIGNAL) < 0){
        cnlog("[e] Metadata send failed. Code: " + to_string(WSAGetLastError()), 0);
        return false;
    }
    if(send(socket_fd, (char*)buffer.data(), buffer.size(), MSG_NOSIGNAL) < 0){
        cnlog("[e] Frame send failed. Code: " + to_string(WSAGetLastError()), 0);
        return false;
    }
    return true;
}

bool handshake(int socket_fd){
    cnlog("[i] Starting handshake...", 2);
    int handshakeMessage[] = {0, MODE, CAMS.size()}, handshakeAck = 0;
    if(send(socket_fd, (char*)handshakeMessage, sizeof(handshakeMessage), 0) == SOCKET_ERROR){
        cnlog("[e] Could not send handshake", 0);
        return false;
    }
    while(1){
        if(recv(socket_fd, (char*)&handshakeAck, sizeof(handshakeAck), 0) <= 0){ 
            int err = errno;
            if(err == BSD_ERROR_WOULDBLOCK){
                Sleep(50);
                continue; 
            }
            else{
                //cnlog("[e] Did not receive handshake acknowledgment. Code: " + to_string(WSAGetLastError()), 0);
                return false;
            }
        }
        else break;
    }
    if(handshakeAck != 400){
        //cnlog("[e] Invalid handshake response: " + to_string(handshakeAck), 0);
        return false;
    }
    //cnlog("[i] Handshake complete", 2);
    return true;
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
            cout << "Options\n  -v\t\t\t= Verbose output\n  -H\t\t\t= Displays available options\n  -i <address>\t\t= Server ip address\n  -p <number>\t\t= Server TCP port number\n  -w <pixels>\t\t= Video horizontal resolution\n  -h <pixels>\t\t= Video vertical resolution\n  -c <number> <list>\t\t= Camera inputs to transmit\n  -q <number>\t\t= Transmission video quality (0-100)\n  -m <number>\t\t= Transmission mode (see README)\n";
            return 1;
        }
        else if(arg == "--verbose" || arg == "-v") VERBOSE = true;
        else{
            cout << "[e] Invalid argument detected\n\nOptions\n  -v\t\t\t= Verbose output\n  -H\t\t\t= Displays available options\n  -i <address>\t\t= Server ip address\n  -p <number>\t\t= Server TCP port number\n  -w <pixels>\t\t= Video horizontal resolution\n  -h <pixels>\t\t= Video vertical resolution\n  -c <number> <list>\t\t= Camera inputs to transmit\n  -q <number>\t\t= Transmission video quality (0-100)\n  -m <number>\t\t= Transmission mode (see README)\n";
            return 1;
        }
    }
    return 0;
}