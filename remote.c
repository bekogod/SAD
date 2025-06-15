#include "remote.h"

int connect_server(int server){

	int client_fd;
    struct sockaddr_in serv_addr;
    
    // Creating socket file descriptor
    // https://man7.org/linux/man-pages/man2/socket.2.html
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    if (server==1){
        serv_addr.sin_port = htons(PORT);
    }
    else {
        serv_addr.sin_port = htons(PORT2);
    }
    

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    //connect to server 
    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return client_fd;

}


void close_server(int client_fd){

	//closing the connected socket
    close(client_fd);

}



size_t rpwrite(int client_fd, int client_fd2, const char* path, const char *buffer, size_t size, off_t offset) {
    size_t total_written = 0;

    while (total_written < size) {
        size_t chunk_size = (size - total_written > BSIZE) ? BSIZE : size - total_written;

        MSG m, r1, r2;
        m.op = WRITE;
        strcpy(m.path, path);
        memcpy(m.buffer, buffer + total_written, chunk_size);
        m.size = chunk_size;
        m.offset = offset + total_written;

        int success1 = 0, success2 = 0;

        // Tentar servidor 1
        if (write(client_fd, &m, sizeof(m)) > 0 &&
            read(client_fd, &r1, sizeof(m)) > 0 &&
            r1.size != -1) {
            printf("Client: W Received Msg from server 1: %s\n", r1.buffer);
            success1 = 1;
        } else {
            perror("Client: Erro ao escrever no servidor 1");
        }

        // Tentar servidor 2
        if (write(client_fd2, &m, sizeof(m)) > 0 &&
            read(client_fd2, &r2, sizeof(m)) > 0 &&
            r2.size != -1) {
            printf("Client: W Received Msg from server 2: %s\n", r2.buffer);
            success2 = 1;
        } else {
            perror("Client: Erro ao escrever no servidor 2");
        }

        if (success1 || success2) {
            total_written += chunk_size;
        } else {
            return -1; // Ambos falharam no chunk atual
        }
    }

    return total_written;
}




size_t rpread(int client_fd, int client_fd2, const char* path, char *buffer, size_t size, off_t offset, int rrobin){
    
    size_t total_read = 0;
    char temp_buffer[BSIZE];


    int fd[2];
    if (rrobin == 0) {
        fd[0] = client_fd;
        fd[1] = client_fd2;
    }
    else {
        fd[0] = client_fd2;
        fd[1] = client_fd;
    }

    while (total_read < size) {
        size_t chunk_size = (size - total_read > BSIZE) ? BSIZE : size - total_read;

        MSG m;
        m.op = READ;
        strcpy(m.path, path);
        m.size = chunk_size;
        m.offset = offset + total_read;

        int success = 0;

        for (int i = 0; i < 2; i++){

            ssize_t w = write(fd[i], &m, sizeof(m));
            printf("Client: READ message sent path: %s\n", m.path);

            if (w <= 0) {
                    perror("Pedido de read não enviado\n");
                    continue;
                }

            ssize_t r = read(fd[i], &m, sizeof(m)); 
            printf("Client: R Received Msg: %zu bytes received.\n", m.size);


            if (r <= 0) {
                perror("Resposta de read não recebida\n");
                continue;
            }

            memcpy(temp_buffer, m.buffer, m.size);
            memcpy(buffer + total_read, temp_buffer, m.size);
            total_read += m.size;
            success = 1;

            break;
        }

        if (!success) {
            return -1;  // Falha nos dois servidores
        }

        if (m.size < chunk_size) {
            break;  // EOF provável
        }

    }

    return total_read;
}


int rstat(int client_fd, int client_fd2, const char* path, struct stat *stbuf, int rrobin){

    MSG m;
    m.op = STAT;
    strcpy(m.path, path);

    int fd[2];
    if (rrobin == 0) {
        fd[0] = client_fd;
        fd[1] = client_fd2;
    }
    else {
        fd[0] = client_fd2;
        fd[1] = client_fd;
    }

    for (int i = 0; i < 2; i++){

        ssize_t w = write(fd[i], &m, sizeof(m));
        printf("Client: STAT message sent path: %s\n", m.path);

        if (w <= 0) {
                perror("Pedido de stat não enviado\n");
                continue;
            }

        ssize_t r = read(fd[i], &m, sizeof(m)); 
        printf("Client: S Received Msg: %s\n", m.buffer);

        if (r <= 0) {
            perror("Resposta de stat não enviado\n");
            continue;
        }

        break;
    }


    *stbuf = m.st; 


    return m.res; 
}


int rpmkdir(int client_fd1, int client_fd2, const char *path, mode_t mode) {
    MSG m, r1, r2;
    m.op = MKDIR;
    strcpy(m.path, path);
    m.mode = mode;

    int success1 = 0, success2 = 0;

    // Tentar servidor 1
    if (write(client_fd1, &m, sizeof(m)) > 0 && read(client_fd1, &r1, sizeof(m)) > 0 && r1.res != -1) {
        printf("Client: MKDIR - Recebido do servidor 1: %s\n", r1.buffer);
        success1 = 1;
    } else {
        perror("Client: Erro ao criar diretório no servidor 1");
    }

    // Tentar servidor 2
    if (write(client_fd2, &m, sizeof(m)) > 0 && read(client_fd2, &r2, sizeof(m)) > 0 && r2.res != -1) {
        printf("Client: MKDIR - Recebido do servidor 2: %s\n", r2.buffer);
        success2 = 1;
    } else {
        perror("Client: Erro ao criar diretório no servidor 2");
    }

    // Decisão de retorno
    if (success1 && success2) {
        return 0;  // Ambos ok
    } else if (success1 || success2) {
        return 0;  // Pelo menos um ok
    } else {
        return -1; // Nenhum conseguiu criar
    }
}

int rprmdir(int client_fd1, int client_fd2, const char *path) {
    MSG m, r1, r2;
    m.op = RMDIR;
    strcpy(m.path, path);

    int success1 = 0, success2 = 0;

    // Tentar servidor 1
    if (write(client_fd1, &m, sizeof(m)) > 0 && read(client_fd1, &r1, sizeof(m)) > 0 && r1.res != -1) {
        printf("Client: RMDIR - Recebido do servidor 1: %s\n", r1.buffer);
        success1 = 1;
    } else {
        perror("Client: Erro ao remover diretório no servidor 1");
    }

    // Tentar servidor 2
    if (write(client_fd2, &m, sizeof(m)) > 0 && read(client_fd2, &r2, sizeof(m)) > 0 && r2.res != -1) {
        printf("Client: RMDIR - Recebido do servidor 2: %s\n", r2.buffer);
        success2 = 1;
    } else {
        perror("Client: Erro ao remover diretório no servidor 2");
    }

    // Decisão de retorno
    if (success1 && success2) {
        return 0;  // Ambos ok
    } else if (success1 || success2) {
        return 0;  // Pelo menos um ok
    } else {
        return -1; // Nenhum conseguiu remover
    }
}



