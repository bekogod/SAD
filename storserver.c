#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "remote.h"

#define SPATH "/home/vagrant/server"
#define SPATH2 "/home/vagrant/server2"

int server_fd;

void intHandler(int dummy) {

    printf("Server shutting down\n");
    close(server_fd);
    exit(0);   
}

void tstpHandler(int signum) {
    printf("Caught SIGTSTP (Ctrl+Z). Cleaning up...\n");
    close(server_fd);
    exit(0);
}


void handle_path(char* oldpath, char* newpath, int server){

    if(server == 1){
        strcpy(newpath, SPATH);
    } else {
        strcpy(newpath, SPATH2);
    }

    // Skip the "/backend" prefix
    const char* relative_path = oldpath + strlen("/backend");

    // Ensure there's a '/' after /backend (i.e., /backend/...)
    if (*relative_path == '/')
        strcat(newpath, relative_path);
    else
        strcat(newpath, "/"); // fallback in case of malformed input
}


//TODO ex1 - dummy example
//Note: Create file if it doesn't exist
void handle_write(int socket_fd, MSG m, int server){

    char path[PSIZE];
    handle_path(m.path, path, server);
    MSG r;
    
    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    lseek(fd, m.offset, SEEK_SET);
    r.size = write(fd, m.buffer, m.size);

    //printf("Server: r.size = %d\n", r.size);

    close(fd);

    printf("Server: WRITE message received for oldpath: %s newpath: %s\n", m.path, path);

    strcpy(r.buffer, "write received");
    r.buffer[15]='\0';


    write(socket_fd, &r, sizeof(r));


}

//TODO ex2
void handle_read(int socket_fd, MSG m, int server){
    
    char path[PSIZE];
    handle_path(m.path, path, server);

    int fd = open(path, O_RDONLY, 0666);
    lseek(fd, m.offset, SEEK_SET);
    m.size = read(fd, m.buffer, m.size);
    close(fd);

    printf("Server: READ message received for oldpath: %s newpath: %s\n", m.path, path);

    write(socket_fd, &m, sizeof(m));
}

//TODO ex3
void handle_stat(int socket_fd, MSG m, int server){

    char path[PSIZE];
    handle_path(m.path, path, server);
    struct stat st;

    if (lstat(path, &st) == -1) {
        m.res = -1;
        snprintf(m.buffer, BSIZE, "STAT failed for path '%s': %s", path, strerror(errno));
    } else {
        m.res = 0;
        m.st = st;
        snprintf(m.buffer, BSIZE,
                "STAT for '%s'\n  Size: %ld bytes\n  Mode: %o\n  UID: %d\n  GID: %d\n  Last modified: %ld",
                path,
                st.st_size,
                st.st_mode,
                st.st_uid,
                st.st_gid,
                st.st_mtime);
    }

    printf("Server: STAT requested for path '%s' | Result: %s | Size: %ld bytes | Mode: %o | UID: %d | GID: %d\n",
       path,
       (m.res == 0) ? "SUCCESS" : "FAILURE",
       (m.res == 0) ? st.st_size : 0,
       (m.res == 0) ? st.st_mode : 0,
       (m.res == 0) ? st.st_uid : -1,
       (m.res == 0) ? st.st_gid : -1);

    
    //printf("Server: STAT message received: %s\n", path);    
    write(socket_fd, &m, sizeof(m));
}


void handle_mkdir(int socket_fd, MSG m, int server) {
    char path[PSIZE];
    handle_path(m.path, path, server);
    MSG r;

    // Tenta criar o diretório com as permissões fornecidas
    int result = mkdir(path, m.mode);
    r.res = result;

    printf("Server: MKDIR message received for oldpath: %s newpath: %s\n", m.path, path);

    if (result == 0) {
        strcpy(r.buffer, "mkdir success");
    } else {
        perror("Server: Erro ao criar diretório");
        strcpy(r.buffer, "mkdir failed");
    }

    r.buffer[13] = '\0'; // Garante terminação da string

    write(socket_fd, &r, sizeof(r));
}

void handle_rmdir(int socket_fd, MSG m, int server) {
    char path[PSIZE];
    handle_path(m.path, path, server);
    MSG r;

    // Tenta remover o diretório
    int result = rmdir(path);
    r.res = result;

    printf("Server: RMDIR message received for oldpath: %s newpath: %s\n", m.path, path);

    if (result == 0) {
        strcpy(r.buffer, "rmdir success");
    } else {
        perror("Server: Erro ao remover diretório");
        strcpy(r.buffer, "rmdir failed");
    }

    r.buffer[13] = '\0'; // Garante terminação da string

    write(socket_fd, &r, sizeof(r));
}

/*void handle_readdir(int socket_fd, MSG m, int server) {
    MSG r;
    char path[PSIZE];
    handle_path(m.path, path, server);

    DIR *dp;
    struct dirent *de;
    r.res = -1;
    r.buffer[0] = '\0';

    dp = opendir(path);
    if (dp == NULL) {
        perror("Server: Erro ao abrir diretório");
        strcpy(r.buffer, "readdir failed");
    } else {
        while ((de = readdir(dp)) != NULL) {
            // Ignora "." e ".."
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            strcat(r.buffer, de->d_name);
            strcat(r.buffer, "\n");
        }
        closedir(dp);
        r.res = 0;
    }

    write(socket_fd, &r, sizeof(r));
}
*/

int main(int argc, char const* argv[])
{
    
    int server;
    if (argc==1){
        server = 1;
    }
    else{
        server = 2;
    }

    signal(SIGINT, intHandler);

    signal(SIGTSTP, tstpHandler);


    //structure for dealing with internet addresses
    struct sockaddr_in address;

    // Creating socket file descriptor
    // https://man7.org/linux/man-pages/man2/socket.2.html
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //Initialize struct and parameters
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    if(argc==1){
        address.sin_port = htons(PORT);
    }
    else {
        address.sin_port = htons(PORT2);
    }

    // Attaches address to socket
    // https://man7.org/linux/man-pages/man2/bind.2.html
    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    //https://man7.org/linux/man-pages/man2/listen.2.html
    if (listen(server_fd, LISTEN_BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1){

        int new_socket;
    
        socklen_t addrlen = sizeof(address);

        //https://man7.org/linux/man-pages/man2/accept.2.html
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //single threaded server... 
        ssize_t res;
        MSG m;
        while((res = read(new_socket, &m, sizeof(m))>0)){
            
            if(m.op==WRITE){
                handle_write(new_socket, m, server);
            }
            if(m.op==READ){
                handle_read(new_socket, m, server);
            }
            if(m.op==STAT){
                handle_stat(new_socket, m, server);
            }
            if(m.op==MKDIR){
                handle_mkdir(new_socket, m, server);
            }
            if(m.op==RMDIR){
                handle_rmdir(new_socket, m, server);
            }
            /*
            if(m.op==READDIR){
                handle_readdir(new_socket, m, server);
            }*/
        }

        close(new_socket);
    }

    
    // closing the listening socket
    close(server_fd);
    return 0;
}