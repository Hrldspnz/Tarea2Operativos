#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <stdbool.h>

#define SERVER_IP "172.172.221.50"
#define SERVER_PORT 1717
#define MAX_RETRIES 3

bool SendImages(const char *NameFile);
bool LoadImages();

bool SendImages(const char *NameFile) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    char path[100];

    strcpy(path, "./imagenes/");
    strcat(path, NameFile);

    FILE *image_file = fopen(path, "rb");
    if (!image_file) {
        perror("Error opening image file");
        return false;
    }

    fseek(image_file, 0, SEEK_END);
    long image_size = ftell(image_file);
    fseek(image_file, 0, SEEK_SET);

    char *image_data = (char *)malloc(image_size);
    fread(image_data, 1, image_size, image_file);
    fclose(image_file);

    bool connected = false;

    for (int attempt = 0; attempt < MAX_RETRIES && !connected; attempt++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error creating socket");
            continue;
        }

        server = gethostbyname(SERVER_IP);
        if (server == NULL) {
            perror("Error getting host by name");
            close(sockfd);
            continue;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Error connecting to server");
            close(sockfd);
            continue;
        }

        connected = true;
    }

    if (!connected) {
        printf("No se pudo establecer conexión después de %d intentos.\n", MAX_RETRIES);
        return false;
    }

    char method[100];
    const char *suffix = " HTTP/1.1\r\n";
    const char *headers = "Content-Length: %ld\r\n\r\n";

    strcpy(method, "PUT /");
    strcat(method, NameFile);
    strcat(method, suffix);

    char request[strlen(method) + strlen(headers) + 20];
    sprintf(request, "%s", method);

    long content_length = image_size;
    sprintf(request + strlen(method), headers, content_length);

    char *request_with_image = (char *)malloc(strlen(request) + image_size);
    strcpy(request_with_image, request);
    memcpy(request_with_image + strlen(request), image_data, image_size);

    ssize_t data_sent = send(sockfd, request_with_image, strlen(request) + image_size, 0);

    if (data_sent < 0) {
        perror("Error sending image data");
        close(sockfd);
        return false;
    }

    printf("Sent PUT request and image data\n");

    close(sockfd);
    free(image_data);

    return true;
}

bool LoadImages() {
    DIR *dir;
    struct dirent *entry;

    dir = opendir("./imagenes");

    if (dir == NULL) {
        perror("No se pudo abrir el directorio");
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            sleep(2);

            if (!SendImages(entry->d_name)) {
                printf("Error al enviar la imagen: %s\n", entry->d_name);
            }
        }
    }

    closedir(dir);

    return true;
}

int main() {
    int choice;

    while (1) {
        printf("Menú:\n");
        printf("1. Enviar imágenes al servidor\n");
        printf("2. Salir\n");
        printf("Selecciona una opción: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Seleccionaste la Opción 1\n");
                if (LoadImages()) {
                    printf("Imágenes enviadas con éxito.\n");
                } else {
                    printf("Error al enviar imágenes.\n");
                }
                break;
            case 2:
                printf("Saliendo del programa\n");
                return 0;
            default:
                printf("Opción inválida\n");
                break;
        }
    }

    return 0;
}
