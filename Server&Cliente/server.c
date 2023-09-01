#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <jpeglib.h>


#define PORT 1717
#define BUFFER_SIZE 50000

void analyceColor(char *imgPath,char *imgName);


typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} Pixel;

void handle_put_request2(int client_socket) {
    char request_buffer[BUFFER_SIZE];
    ssize_t bytes_received = 0;

    // Inicializa el búfer de solicitud
    memset(request_buffer, 0, sizeof(request_buffer));

    // Lee los datos de la solicitud hasta que se complete
    while (1) {
        ssize_t bytes = recv(client_socket, request_buffer + bytes_received, BUFFER_SIZE - bytes_received, 0);
        if (bytes <= 0) {
            break;  // Fin de la solicitud o error
        }
        bytes_received += bytes;

        // Verifica si la solicitud está completa
        if (strstr(request_buffer, "\r\n\r\n") != NULL) {
            break;
        }
    }

    if (bytes_received <= 0) {
        // Error o conexión cerrada por el cliente
        perror("Error receiving request");
        return;
    }

    // Analiza la solicitud una vez que se completa
    char *file_start = strstr(request_buffer, "\r\n\r\n");

    if (file_start) {
        file_start += 4;  // Avanzar el puntero después de "\r\n\r\n"

        char imgName[50];
        // Parsea el nombre de la imagen desde la solicitud si es necesario
        sscanf(request_buffer, "PUT /%s", imgName);

        char path[100];
        strcpy(path, "./ImagenesRecibidas/");
        strcat(path, imgName);

        FILE *file = fopen(path, "wb");
        if (file == NULL) {
            perror("Error opening file");
            return;
        }

        size_t data_length = bytes_received - (file_start - request_buffer);
        fwrite(file_start, 1, data_length, file);

        fclose(file);

        analyceColor(path, imgName);
    }

    // Enviar respuesta 200 OK al cliente
    const char *response = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_socket, response, strlen(response), 0);

    printf("\nMensaje completado.\n");
}

void analyceColor(char *imgPath,char *imgName){
    const char *newPath = "./ImagenesColor/"; 
    FILE *jpg_file = fopen(imgPath, "rb");
    if (!jpg_file) {
        printf("No se pudo abrir la imagen.\n");
    }


    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, jpg_file);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    int red_sum = 0;
    int green_sum = 0;
    int blue_sum = 0;
    int pixel_count = 0;

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *row_ptr = (unsigned char *)malloc(cinfo.output_width * cinfo.output_components);
        jpeg_read_scanlines(&cinfo, &row_ptr, 1);

        for (int i = 0; i < cinfo.output_width * cinfo.output_components; i += cinfo.output_components) {
            red_sum += row_ptr[i];
            green_sum += row_ptr[i + 1];
            blue_sum += row_ptr[i + 2];
            pixel_count++;
        }

        free(row_ptr);
    }

    fclose(jpg_file);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    double red_avg = (double)red_sum;
    double green_avg = (double)green_sum;
    double blue_avg = (double)blue_sum;

    if (red_avg > green_avg && red_avg > blue_avg) {
        char path[100];
        strcpy(path, newPath);
        strcat(path, "rojo/");
        strcat(path, imgName);
        rename(imgPath, path);
        printf("Imagen: %s\n", path);
        printf("Promedio de color rojo: %.2f\n", red_avg);
        printf("Promedio de color verde: %.2f\n", green_avg);
        printf("Promedio de color azul: %.2f\n", blue_avg);
    } else if (green_avg > red_avg && green_avg > blue_avg) {
        char path[100];
        strcpy(path, newPath);
        strcat(path, "verde/");
        strcat(path, imgName);
        rename(imgPath, path);
        printf("Imagen: %s\n", path);
        printf("Promedio de color rojo: %.2f\n", red_avg);
        printf("Promedio de color verde: %.2f\n", green_avg);
        printf("Promedio de color azul: %.2f\n", blue_avg);
    } else {
        char path[100];
        strcpy(path, newPath);
        strcat(path, "azul/");
        strcat(path, imgName);
        rename(imgPath, path);
        printf("Imagen: %s\n", path);
        printf("Promedio de color rojo: %.2f\n", red_avg);
        printf("Promedio de color verde: %.2f\n", green_avg);
        printf("Promedio de color azul: %.2f\n", blue_avg);
    }

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

        printf("Connection accepted...\n");

        // Manejar la solicitud en un nuevo hilo o proceso si es necesario
        handle_put_request2(client_socket);

        close(client_socket);
    }

    close(server_socket);

    return 0;
}