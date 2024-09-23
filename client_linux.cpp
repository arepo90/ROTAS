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

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
#define QUALITY 40
#define WIDTH 1280
#define HEIGHT 720

int main(){
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
        vector<uchar> img_buffer{0, 1, 2, 3, 4, 254, 255};

        while(1){
            cap >> frame;
            if(frame.empty()){
                cout << "[e] Failed to capture frame" << '\n';
                break;
            }
            vector<int> compression_params = { IMWRITE_JPEG_QUALITY, QUALITY };
            imencode(".jpg", frame, img_buffer, compression_params);

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
