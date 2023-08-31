#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 1717
#define BUFFER_SIZE 50000

void handle_put_request(int client_socket, char *request_buffer) {

    ssize_t bytes_read;

    printf("Contenido de request_buffer:\n%s\n", request_buffer);

    // Enviar respuesta 200 OK al cliente
    const char *response = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_socket, response, strlen(response), 0);

    printf("\nMensaje completado.\n");
}

void handle_put_request2(int client_socket, char *request_buffer, ssize_t request_size, char *imgName) {
    char *file_start = strstr(request_buffer, "\r\n\r\n");

    if (file_start) {
        file_start += 4;  // Avanzar el puntero después de "\r\n\r\n"

        char path[100];
        strcpy(path, "./ImagenesRecibidas/");
        strcat(path, imgName);

        FILE *file = fopen(path, "wb");
        if (file == NULL) {
            perror("Error opening file");
            return;
        }

        size_t data_length = request_size - (file_start - request_buffer);
        fwrite(file_start, 1, data_length, file);

        fclose(file);

    }

    // Enviar respuesta 200 OK al cliente
    const char *response = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_socket, response, strlen(response), 0);

    printf("\nMensaje completado.\n");
}


int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, 10) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connections
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        char request_buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket, request_buffer, BUFFER_SIZE, 0);

        printf("Cantidad de bytes recibidos: %zd\n", bytes_received);

        if (bytes_received > 0) {
            // Parse the request to get the method and file name
            char method[10];
            char imageName[50];

            sscanf(request_buffer, "%s /%s", method, imageName);
        

            if (strcmp(method, "PUT") == 0) {
                printf("Handling PUT request\n");
                handle_put_request2(client_socket, request_buffer, bytes_received, imageName);
            } else {
                printf("Unsupported method: %s\n", method);
            }
        }

        close(client_socket);
    }

    close(server_socket);

    return 0;
}
