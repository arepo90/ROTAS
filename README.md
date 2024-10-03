# ROTAS (Real-time Optimized Transmission for Aggregated Sources)

Self-contained server and client applications for TCP video streaming. Optimized for limited bandwith (<=1 mbps) connections for low latency streaming of HD video sources, although it can be repurposed to send any kind of messages in both directions. Threading is used to stream multiple sources in parallel, so performance greatly depends on hardware.

> Server application is only for Windows. Client application is available for Windows and Linux.

## FAQ
1. Does it work? i guess

2. Is it fast? hell yeah

## Prerequisites
- CMake >=3.10
- OpenCV 4.10.0 + opencv_contrib extra modules
- g++ >=8.1.0

## How to use

### Windows

1. Clone repo
```
git clone https://github.com/arepo90/ROTAS.git
cd ROTAS
```

2. Build CMake cache
```
mkdir build
cmake -B .\build\
```

3. Compile executables
```
cmake --build .\build\
```

4. Run

Server (receiver):
```
.\build\Debug\server.exe <flags> <arguments>
```

Client (sender):
```
.\build\Debug\client.exe <flags> <arguments>
```

### Linux

1. Clone repo
```
git clone https://github.com/arepo90/ROTAS.git
cd ROTAS
```

2. Compile executables
```
g++ client_linux.cpp -o client_linux `pkg-config --cflags --libs opencv4`
```

3. Run
```
./client_linux <flags> <arguments>
```

### Flags

To change application settings without the need to modify the source code and rebuild the executables, you can include any of the following flags with its corresponding arguments:

> Any changes made to the client will be communicated to the server automatically during the initial handshake, with the notable exception of the port and camera amount, since communication cannot be established without a common port.

#### Server options

| Flag             | Argument | Setting              | Default |
|------------------|----------|----------------------|---------|
| `-H` `--help`    |          | Displays  options    |         |
| `-v` `--verbose` |          | Verbose console logs | false   |
| `-p` `--port`    | port     | TCP port             | 8080    |
| `-c` `--cams`    | number   | Camera input amount  | 1       |

#### Client options 

| Flag             | Argument     | Setting                     | Default               |
|------------------|--------------|-----------------------------|-----------------------|
| `-H` `--help`    |              | Displays  options           |                       |
| `-v` `--verbose` |              | Verbose console logs        | false                 |
| `-p` `--port`    | port         | TCP port                    | 8080                  |
| `-i` `--ip`      | address      | Server IP address           | 127.0.0.1 (localhost) |
| `-c` `--cams`    | number, list | List of camera sources      | 1 0                   |
| `-w` `--width`   | pixels       | Horizontal video resolution | 1280                  |
| `-h` `--height`  | pixels       | Vertical video resolution   | 720                   |
| `-q` `--quality` | number       | Video image quality (1-100) | 50                    |

> Camera USB ports tend to be in order (starting from 0), but this is not always the case. To verify, run the `cam_detection.py` script and use the `--cams` flag on the client to specify them.

> When streaming multiple webcams, one socket is used for every source (in ascending order). For example, for 3 sources on default settings, ports 8080, 8081 and 8082 must be open and available.

### Examples

Server will listen on port 9999 and output all logs. Client will connect to `192.168.0.1:9999` and stream with no image compression:
```
.\build\Debug\server.exe -p 9999 -v
.\build\Debug\client.exe -i 192.168.0.1 -p 9999 -q 100
```

Server will run with default settings. Client will transmit the cameras on ports `0` and `6` at a `640x360` resolution with medium image compression:
```
.\build\Debug\server.exe
.\build\Debug\client.exe -c 2 0 6 -w 640 -h 360 -q 50
```

## Transmission

In a nutshell:
1. Server initiates connection with client, one socket per video source 
2. Client sends handshake message
3. Server acknowledges handshake
4. Client captures a frame from each video source, compresses them and converts them to byte arrays
5. Client starts transaction by sending a metadata packet for each socket
6. Client sends a frame packet for each socket
7. Server acknowledges packets and ends transaction
8. Repeat steps 4-7

> WIP

## Logs
All applications regularly print console messages of different types (to receive __all__ available logs, make sure to include the `--verbose` flag on the executable):

- `[i]` __Information__: Provides updates as to what the program is currently doing. When followed by `...`, it means the program is awaiting a response.
- `[w]` __Warning__: Alerts about a non fatal error, most likely caused by the remote peer or a dependency.
- `[e]` __Error__: Gives out an error number and message relating to what went wrong, followed by ending the program. Usually caused by an internal problem or a fatal cononection loss.
- `[recv]` __Received__: Shows relevant information about the latest packet received (size, loss, frame rate, etc.).
- __Dependency warnings and info__: OpenCV, Winsock, CMake and g++ tend to print out compilation warnings and information throughout the execution of the applications. They can be safely ingnored as long as they don't include any errors.

## Author
Miguel Esteban Martinez Villegas - arepo90
