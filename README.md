# ROTAS (Real-time Optimized Transmission for Aggregated Sources)

Self-contained server and client applications for TCP video streaming. Optimized for limited bandwith (<=1 mbps) connections for low latency streaming of multiple video sources, although it can be repurposed to send any kind of messages in both directions.

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

> Any changes made to the client will be communicated to the server automatically during the initial handshake, with the notable exception of the port, since communication cannot be established without a common port.

#### General options

| Flag             | Argument | Setting              | Default |
|------------------|----------|----------------------|---------|
| `-H` `--help`    |          | Displays  options    |         |
| `-v` `--verbose` |          | Verbose console logs | false   |
| `-p` `--port`    | port     | TCP port             | 8080    |

#### Client specific options 

| Flag             | Argument    | Setting                     | Default               |
|------------------|-------------|-----------------------------|-----------------------|
| `-i` `--ip`      | address     | Server IP address           | 127.0.0.1 (localhost) |
| `-m` `--mode`    | number      | [Transmission mode](#Transmission-modes) | 0        |
| `-c` `--cams`    | number, list| List of camera sources      | 1 0                   |
| `-w` `--width`   | pixels      | Horizontal video resolution | 1280                  |
| `-h` `--height`  | pixels      | Vertical video resolution   | 720                   |
| `-q` `--quality` | number      | Video image quality (1-100) | 50                    |

Camera source ports tend to be in order (starting from 0), but this is not always the case. To verify, run the `cam_detection.py` script and use the `--cams` flag to specify the ports.

### Examples

Server will listen on port 9999 and output all logs. Client will connect to `192.168.0.1:9999` with transmission mode `2` and no image compression:
```
.\build\Debug\server.exe -p 9999 -v
.\build\Debug\client.exe -i 192.168.0.1 -p 9999 -m 2 -q 100
```

Server will run with default settings. Client will transmit the cameras on ports `0` and `6` with transmission mode `1` at a `640x360` resolution with medium image compression:
```
.\build\Debug\server.exe
.\build\Debug\client.exe -m 1 -c 2 0 6 -w 640 -h 360 -q 50
```

## Transmission modes

In general, for all modes:
1. Server initiates transaction with client
2. Client captures a frame from each source
3. Client compresses frames
4. Client converts frames to packets of byte arrays and adds metadata
5. Clients sends packets to server bia TCP socket
6. Server receives encoded packets
7. Server decodes packets back into frames
8. Server shows frames in separate windows
9. Server concludes transaction
10. Process repeats for every frame

Depending on the network infrastructure and available bandwith, you may want to prioritize frame rate, connection stability, image quality, etc. This can be achieved by changing the argument of the `--mode` flag on the client executable, and the server will adapt automatically:

- `0` __Single__: For testing purposes. Only the first camera source will be transmitted.
- `1` __Bundles__: All video source frames are compresed and merged into one packet, which is transmitted in a single transaction. Every frame is guaranteed to be in sync, but packet size increases linearly with the number of sources.
- `2` __Swaps__ (WIP): Video source frames are put into separate packets, which are then sent one after the other in a single transaction. Keeps the packet size unchanged, but increases packets per transaction and frames may become out of sync because of packet loss.
- > More coming soon

## Logs
All applications regularly print console messages of different types (to receive __all__ available logs, make sure to include the `--verbose` flag on the executable):

- `[i]` __Information__: Provides updates as to what the program is currently doing. When followed by `...`, it means the program is awaiting a response from a blocking function.
- `[w]` __Warning__: Alerts about a non fatal error, most likely caused by the remote peer. Usually followed by an attempt to reset the connection.
- `[e]` __Error__: Gives out an error number and message relating to what went wrong, followed by ending the program. Usually caused by an internal problem or a fatal cononection loss.
- `[recv]` __Received__: Shows relevant information about the latest packet received (size, loss, frame rate, etc.).
- `[send]` __Send__: Shows relevant information about the latest packet sent (size, number, etc.).
- __Dependency warnings and info__: OpenCV, Winsock, CMake and g++ tend to print out compilation warnings and information throughout the execution of the applications. They can be safely ingnored as long as they don't include any errors.

## Author
Miguel Esteban Martinez Villegas - arepo90
