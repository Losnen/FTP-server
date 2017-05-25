# REDES Y SISTEMAS DISTRIBUIDOS 15-16
## FTP Server

En una terminal ejecuta el servidor:
```
 make clean
 make
 ./ftp_server
```

En una terminal aparte ejecuta el cliente:
```
ftp -d
```
Una vez dentro de la interfaz ejecuta:
```
open localhost 2121
```
