# ROTAS (Real-time Optimized Transmission for Aggregated Sources)

Self-contained server and client applications for TCP video streaming. Optimized for limited bandwith (<=1 mbps) connections for low latency streaming, although it can be repurposed to send any kind of messages in both directions.

Works by sequencing _transactions_ between the client and server, each including an encoded video frame and information about the next transaction.

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
./server_linux <flags> <arguments>
```

### Flags

To change application settings without the need to modify the source code and rebuild the executables

#### General options

| Flag             | Argument   | Setting               | Default |
|------------------|------------|-----------------------|---------|
| `-H` `--help`    |            | Displays flag options |         |
| `-v` `--verbose` |            | Verbose console logs  | false   |
| `-p` `--port`    | port       | TCP port              | 8080    |

#### Client specific options 

| Flag             | Argument   | Setting                     | Default               |
|------------------|------------|-----------------------------|-----------------------|
| `-i` `--ip`      | address    | Server IP address           | 127.0.0.1 (localhost) |
| `-m` `--mode`    | number     | [Transmission mode](#Transmission-modes) | 0        |
| `-c` `--cams`    | number     | Number of camera sources    | 1                     |
| `-w` `--width`   | pixels     | Horizontal video resolution | 1280                  |
| `-h` `--height`  | pixels     | Vertical video resolution   | 720                   |
| `-q` `--quality` | number     | Video image quality (1-100) | 50                    |

## Transmission modes

Depending on the network infrastructure and available bandwith, you may want to prioritize frame rate, connection stability, image quality, etc. This can be achieved by changing the argument of the `--mode` flag on the client executable:

- `0` __Bundles__: All video source frames are compresed and merged into one package, which is transmitted in a single transaction. Keeps the frame rate relatively unchanged, but package size increases linearly with the number of sources if kept at the same quality and resolution.
- `1` __Swaps__: Video source frames are put into separate packages, which are then sent one after the other in a single transaction. Keeps the package size unchanged, but the frame rate is effectively divided by the number of sources.

> More coming soon

## Logs
All applications regularly print console messages of different types (to receive __all__ available logs, make sure to include the `--verbose` flag on the executable):

- `[i]` __Information__: Provides updates as to what the program is currently doing. When followed by `...`, it means the program is awaiting a response from a blocking function.
- `[w]` __Warning__: Alerts about a non catastrophic error, most likely caused by the remote peer. Usually followed by an attempt to reset the connection.
- `[e]` __Error__: Gives out an error number and message relating to what went wrong, followed by ending the program. Usually caused by an internal problem or a fatal cononection loss.
- `[recv]` __Received__: Shows relevant information about the latest transaction (package size, response code, current frame rate, etc.). 
- __Dependency warnings and info__: OpenCV, Winsock, CMake and g++ tend to print out compilation warnings and information throughout the execution of the applications. They can be safely ingnored as long as they don't include any errors.

## Author
Miguel Esteban Martinez Villegas - arepo90