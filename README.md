# ROTAS (Real-time Optimized Transmission for Aggregated Sources)

Self-contained server and client applications for TCP video streaming. Optimized for limited bandwith (<=1 mbps) connections for low latency streaming, although it can be repurposed to send any kind of messages in both directions.

Works by sequencing _transactions_ between the client and server, each including an encoded video frame and information about the next transaction.

> Server application is only for Windows. Client application is available for Windows and Linux.

## FAQ
1. Does it work? i guess

2. Is it fast? hell yeah

## Prerequisites
- CMake >=3.10
- OpenCV 4.10.0
- g++ >=8.1.0

## How to use (Windows)

1. Clone repo

2. Build CMake cache
```
mkdir build
cmake -B .\build\
```

3. Change the target server IP and port in [client.cpp](https://github.com/arepo90/ROTAS/blob/main/client.cpp), default is 127.0.0.1:8080 (localhost)

4. Compile executables (required after every modification)
```
cmake --build .\build\
```

5. Run

Server (receiver):
```
.\build\Debug\server.exe
```

Client (sender):
```
.\build\Debug\client.exe
```

## How to use (Linux)

_Coming soon_

## Logs
All applications regularly print console messages of different types:
- `[i]` __Information__: Provides updates as to what the program is currently doing. When followed by `...`, it means the program is awaiting a response from a blocking function.
- `[w]` __Warning__: Alerts about a non catastrophic error, most likely caused by the remote peer. Usually followed by an attempt to reset the connection.
- `[e]` __Error__: Gives out an error number and message relating to what went wrong, followed by ending the program. Usually caused by an internal problem or a fatal cononection loss.
- `[recv]` __Received (server only)__: Shows relevant information about the latest transaction, namely package size and current frame rate. 
- __Dependency warnings and info__: OpenCV, Winsock, CMake and g++ tend to print out compilation warnings and information throughout the execution of the applications. They can be safely ingnored as long as they don't include any errors.

## Author
Miguel Esteban Martinez Villegas - arepo90
