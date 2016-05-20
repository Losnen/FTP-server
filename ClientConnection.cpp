//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2ª de grado de Ingeniería Informática
//
//              This class processes an FTP transactions.
//
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"


int define_socket_TCP2(int port) {
    struct sockaddr_in sin;

    int s, type;

    s = socket(AF_INET,SOCK_STREAM, 0);
    if(s < 0) {
                errexit("No puedo crear el socket: %s :(\n", strerror(errno));
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            errexit("No puedo hacer el bind con el puerto: %s:(\n", strerror(errno));
    }

    if (listen(s, 5) < 0)
            errexit("Fallo en el listen: %s:(\n", strerror(errno));

    return s;
}


ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);

    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }

    ok = true;
    data_socket = -1;



};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket);

}


int connect_TCP( uint32_t address,  uint16_t  port) {
    struct sockaddr_in sin;
    struct hostent *hent;
    int s;
    char str[10];

    sprintf( str, "%u", address);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    sin.sin_addr.s_addr = address;

    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s < 0){
        errexit("No se puede crear el socket: %s :(\n", strerror(errno));
    }

    if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        errexit("No se puede conectar con %s: %s\n :(", address, strerror(errno));
	}

    return s;

}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;

}






#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }

    fprintf(fd, "220 Service ready\n");

    while(!parar) {



      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	      fscanf(fd, "%s", arg);
        if (strcmp(arg,"alu") == 0)
          fprintf(fd, "331 User name ok, need password.\n");
        else
          fprintf(fd, "332 Need account for login.\n");
      }
      else if (COMMAND("PWD")) {
        char aux[200];
        getcwd(aux,sizeof(aux)); //La funcion getcwd esta en la librería unistd.h
        fprintf(fd, "257 'PATHNAME' created  %s\n",aux);
      }
      else if (COMMAND("PASS")) {
        fscanf(fd, "%s", arg);
        printf("(PASS):%s\n", arg);
        if (!strcmp(arg,"1234"))
          fprintf(fd, "230 User logged in, proceed.\n");
        else
          fprintf(fd, "530 Not logged in.\n");
      }
      else if (COMMAND("PORT")) {
        int ip[4];
        int puertos[2];
        fscanf(fd, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &puertos[0], &puertos[1]);

        uint32_t ip_addr = ip[3]<<24 | ip[2]<<16 | ip[1]<<8 | ip[0];
        uint16_t port_v = puertos[0] << 8 | puertos[1];

        data_socket = connect_TCP(ip_addr,port_v);

        fprintf(fd, "200 Okey\n");
      }
      else if (COMMAND("PASV")) {
        struct sockaddr_in fsin;
      	socklen_t len = sizeof(fsin);
      	uint16_t s = define_socket_TCP2(0);
      	uint16_t port = fsin.sin_port;

        getsockname(s, (struct sockaddr *) &fsin, &len);
      	fprintf(fd, "227 entering passive mode (127,0,0,1,%d,%d)\n", port>>8 , port & 0xFF);
        fflush(fd);

        data_socket = accept(s, (struct sockaddr *) &fsin, &len);
      }
      else if (COMMAND("CWD")) {
        char aux[200];
        getcwd(aux,sizeof(aux));
        if(chdir(aux) == 0)
			     fprintf(fd, "200 Working directory changed\n");
		    else
			     fprintf(fd, "431 No such directory\n");
      }
      else if (COMMAND("STOR") ) {
        fscanf(fd, "%s", arg);
        char buffer[MAX_BUFF];
        int file;
        int aux;

        file=open(arg, O_RDWR|O_CREAT,S_IRWXU);
        fprintf(fd, "150 File ok, creating connection\n");
        fflush(fd);
        if(file < 0)
        {
          fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
        }
        else
        {
          do{
            aux = read(data_socket,buffer,sizeof(buffer));
            write(file,buffer,aux);
          }while(aux > 0);

          fprintf(fd,"250 Requested file action okay, completed.\n");
          close(file);
          close(data_socket);
        }
      }
      else if (COMMAND("SYST")) {
        fprintf(fd, "215 UNIX Type: L8.\n");
      }
      else if (COMMAND("TYPE")) {
        fprintf(fd, "200 TYPE OK\n");
      }
      else if (COMMAND("RETR")) {
        fscanf(fd, "%s", arg);
        FILE* file = fopen(arg,"rb");
        if (!file){
          fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
          close(data_socket);
        }
        else{
          fprintf(fd, "150 File status okay; about to open data connection.\n");
          struct sockaddr_in sa;
          socklen_t sa_len = sizeof(sa);
          char buffer[MAX_BUFF];
          int aux;
          do{
            aux = fread(buffer, sizeof(char), MAX_BUFF, file);
            send(data_socket, buffer, aux, 0);
          }while(aux == MAX_BUFF);

          fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
          fclose(file);
          close(data_socket);
        }
      }
      else if (COMMAND("QUIT")) {
        parar = true;
	      fprintf(fd, "221 Service closing control connection\n");
      }
      else if (COMMAND("LIST")) {
        fprintf(fd, "125 Data Transfer starting\n");
        struct sockaddr_in sa;
        socklen_t sa_len = sizeof(sa);
        char buffer[MAX_BUFF];
        std::string listado;
        std::string ls = "ls";
        FILE* file = popen(ls.c_str(), "r");
        if (!file){
          fprintf(fd, "450 File unavaible.\n");
          close(data_socket);
        }else{
          while (!feof(file))
            if (fgets(buffer, MAX_BUFF, file) != NULL)
              listado.append(buffer);

              send(data_socket, listado.c_str(), listado.size(), 0);
              fprintf(fd, "250 Requested file action successful.\n");
              pclose(file);
              close(data_socket);
        }
      }
      else  {
	      fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	      printf("Comando : %s %s\n", command, arg);
	      printf("Error interno del servidor\n");

      }

    }

    fclose(fd);


    return;

};
