#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>q
#include <dirent.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1717

void SendImages(const char *NameFile);
void LoadImages();

// Definición de la función
void SendImages(const char *NameFile) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    char path[100]; 

    strcpy(path, "./imagenes/");
    strcat(path, NameFile);

    FILE *image_file = fopen(path, "rb");
    if (!image_file) {
        perror("Error opening image file");
       
    }

    fseek(image_file, 0, SEEK_END);
    long image_size = ftell(image_file);
    fseek(image_file, 0, SEEK_SET);

    char *image_data = (char *)malloc(image_size);
    fread(image_data, 1, image_size, image_file);
    fclose(image_file);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
       
    }

    server = gethostbyname(SERVER_IP);
    if (server == NULL) {
        perror("Error getting host by name");
        
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
   
    }

    // Especifica que es una solicitud PUT

    char method[100];  // Ajusta el tamaño según tus necesidades
    const char *suffix = " HTTP/1.1\r\n";
    const char *headers = "Content-Length: %ld\r\n\r\n";

    strcpy(method, "PUT /");
    strcat(method, NameFile);
    strcat(method, suffix);
        
    char request[strlen(method) + strlen(headers) + 20]; // +20 for potential content length digits
    sprintf(request, "%s", method);
        
    long content_length = image_size;

    sprintf(request + strlen(method), headers, content_length);

    // Combina la solicitud HTTP con los datos de la imagen
    char *request_with_image = (char *)malloc(strlen(request) + image_size);
    strcpy(request_with_image, request);
    memcpy(request_with_image + strlen(request), image_data, image_size);

    ssize_t data_sent = send(sockfd, request_with_image, strlen(request) + image_size, 0);
    

    if (data_sent < 0) {
        perror("Error sending image data");
       
    }

    printf("Sent PUT request and image data\n");

    close(sockfd);
    free(image_data);
}

void LoadImages(){
    DIR *dir;
    struct dirent *entry;

    // Abre el directorio actual (puedes reemplazar "." con la ruta de tu directorio)
    dir = opendir("./imagenes");
    
    if (dir == NULL) {
        perror("No se pudo abrir el directorio");
    }

    // Lee cada entrada en el directorio
    while ((entry = readdir(dir)) != NULL) {

        // Ignora las entradas "." y ".."
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            sleep(2);

            SendImages(entry->d_name);
        }
    }

    // Cierra el directorio
    closedir(dir);
}

int main() {
    


    int choice;

    while (1) {
        printf("Menú:\n");
        printf("1. Enviar imagenes al servidor\n");
        printf("2. Salir\n");
        printf("Selecciona una opción: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Seleccionaste la Opción 1\n");
                LoadImages();
                //SendImages();
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
