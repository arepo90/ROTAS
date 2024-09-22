# ROTAS (Real-time Optimized Transmission for Aggregated Sources)

Self-contained server and client applications for TCP video streaming. Optimized for low latency and low bandwith (<=1 mbps) at standard definition, although it can be repurposed to send any kind of messages.

_Windows only version for now, Linux coming soon_

## FAQ
1. Does it work? i guess

2. Is it fast? hell yeah

## Prerequisites
- CMake >=3.10
- OpenCV 4.10.0
- g++ >= 8.1.0

## How to use

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

Client (sendeer):
```
.\build\Debug\client.exe
```

## Author
Miguel Esteban Martinez Villegas - arepo90
