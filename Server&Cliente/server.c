#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <jpeglib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>
#include <gif_lib.h>
#include <time.h>


#define PORT 1717
#define BUFFER_SIZE 50000

void analyceColor(char *imgPath,char *imgName);


typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} Pixel;

// Función para cargar la configuración desde el archivo
void cargarConfiguracion() {
    FILE *archivoConfig = fopen("config.conf", "r");
    if (archivoConfig == NULL) {
        printf("Error al abrir el archivo de configuración.\n");
        exit(1);
    }

    char linea[100]; // Supongamos un tamaño máximo de línea de 100 caracteres

    while (fgets(linea, sizeof(linea), archivoConfig)) {
        char *token = strtok(linea, ":");
        if (token != NULL) {
            char *clave = trim(token); // Función para eliminar espacios en blanco alrededor de la clave

            token = strtok(NULL, ":");
            if (token != NULL) {
                char *valor = trim(token); // Función para eliminar espacios en blanco alrededor del valor

                // Asignar el valor a la variable correspondiente
                if (strcmp(clave, "Port") == 0) {
                    Port = atoi(valor); // Convertir a entero
                } else if (strcmp(clave, "DirColores") == 0) {
                    strcpy(DirColores, valor);
                } else if (strcmp(clave, "DirHisto") == 0) {
                    strcpy(DirHisto, valor);
                } else if (strcmp(clave, "DirLog") == 0) {
                    strcpy(DirLog, valor);
                }
            }
        }
    }

    fclose(archivoConfig);
}

void escribirEnRegistro(const char *mensaje) {
    // Abrir el archivo de registro en modo de escritura (si no existe, se creará)
    FILE *archivoLog = fopen("registro.log", "a"); // "a" para agregar al archivo existente

    if (archivoLog == NULL) {
        printf("No se pudo abrir el archivo de registro.\n");
        return;
    }

    // Obtener la fecha y hora actual
    time_t tiempo;
    struct tm *tiempoInfo;
    time(&tiempo);
    tiempoInfo = localtime(&tiempo);
    char tiempoFormateado[80];
    strftime(tiempoFormateado, sizeof(tiempoFormateado), "%Y-%m-%d %H:%M:%S", tiempoInfo);

    // Escribir el mensaje de registro en el archivo
    fprintf(archivoLog, "[%s] %s\n", tiempoFormateado, mensaje);

    // Cerrar el archivo de registro
    fclose(archivoLog);
}

int main() {
    // Llamamos a la función escribirEnRegistro para registrar un mensaje
    escribirEnRegistro("Este es un mensaje de registro desde una función.");

    return 0;
}

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
        char nuevoNombre[50];
        char sufijo[]="_hist";
        char histopath = "./ImagenesHist";
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

        char *punto = strrchr(imgName, '.');

        // Calcular la longitud del nombre sin la extensión
        size_t longitudSinExtension = punto - imgName;

        // Copiar el nombre sin la extensión al nuevo nombre
        strncpy(nuevoNombre, imgName, longitudSinExtension);
        nuevoNombre[longitudSinExtension] = '\0'; // Añadir el terminador nulo

        // Agregar el sufijo deseado al nuevo nombre
        strcat(nuevoNombre, sufijo);
        strcat(histopath,nuevoNombre);

        Histograma(path,histopath);

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


void convertToGrayscalePNG(unsigned char *data, size_t size) {
    for (size_t i = 0; i < size; i += 4) { // 4 bytes por píxel (RGBA)
        // Convertir a escala de grises (promedio de los canales RGB)
        unsigned char gray = (data[i] + data[i + 1] + data[i + 2]) / 3;
        data[i] = data[i + 1] = data[i + 2] = gray; // R, G y B se establecen con el valor de escala de grises
    }
}

void convertToGrayscaleJPG(unsigned char *data, size_t size) {
    for (size_t i = 0; i < size; i += 3) {
        // Convertir a escala de grises (promedio de los canales RGB)
        unsigned char gray = (data[i] + data[i + 1] + data[i + 2]) / 3;
        data[i] = data[i + 1] = data[i + 2] = gray;
    }
}


void equalizeHistogram(unsigned char *data, size_t size) {
    int histogram[256] = {0};

    // Calcular el histograma de niveles de gris
    for (size_t i = 0; i < size; i++) {
        histogram[data[i]]++;
    }

    // Calcular la función de transformación acumulativa
    int cumulativeHistogram[256];
    cumulativeHistogram[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cumulativeHistogram[i] = cumulativeHistogram[i - 1] + histogram[i];
    }

    // Aplicar la ecualización de histograma
    for (size_t i = 0; i < size; i++) {
        data[i] = (cumulativeHistogram[data[i]] * 255) / size;
    }
}


unsigned char* loadGIF(const char* filename, size_t* size, size_t* width, size_t* height) {
    int error;
    GifFileType* gif = DGifOpenFileName(filename, &error);
    if (!gif) {
        fprintf(stderr, "Error al abrir el archivo GIF: %s\n", GifErrorString(error));
        return NULL;
    }

    // Avanzar hasta el último frame (si hay más de uno)
    while (DGifGetRecordType(gif,NULL) != TERMINATE_RECORD_TYPE) {
        if (DGifGetRecordType(gif,NULL) == IMAGE_DESC_RECORD_TYPE) {
            if (DGifGetImageDesc(gif) == GIF_ERROR) {
                fprintf(stderr, "Error al obtener la descripción de la imagen.\n");
                DGifCloseFile(gif, &error);
                return NULL;
            }
        }
    }

    // Verificar si se leyó al menos un frame
    if (gif->ImageCount < 1) {
        fprintf(stderr, "El archivo GIF no contiene frames.\n");
        DGifCloseFile(gif, &error);
        return NULL;
    }

    // Obtener el tamaño del cuadro (frame)
    *width = gif->SWidth;
    *height = gif->SHeight;

    // Crear un búfer para los datos de la imagen (frame)
    unsigned char* data = (unsigned char*)malloc(3 * (*width) * (*height));
    if (!data) {
        fprintf(stderr, "Error al asignar memoria para los datos de la imagen.\n");
        DGifCloseFile(gif, &error);
        return NULL;
    }

    // Leer los datos del cuadro (frame)
    if (DGifGetLine(gif, data, 3 * (*width) * (*height)) == GIF_ERROR) {
        fprintf(stderr, "Error al leer los datos del cuadro (frame) GIF.\n");
        free(data);
        DGifCloseFile(gif, &error);
        return NULL;
    }

    DGifCloseFile(gif, &error);

    // Establecer el tamaño de la imagen
    *size = 3 * (*width) * (*height);

    return data;
}

void liberarImagen(unsigned char* data) {
    if (data) {
        free(data);
    }
}

unsigned char *loadPNG(const char *filename, size_t *size, size_t *width, size_t *height) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error al abrir el archivo");
        return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(file);
        perror("Error al crear estructura PNG");
        return NULL;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(file);
        png_destroy_read_struct(&png, NULL, NULL);
        perror("Error al crear estructura de información PNG");
        return NULL;
    }

    png_init_io(png, file);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Calcular el tamaño total de los datos de la imagen
    *size = *width * *height * (bit_depth / 8) * png_get_channels(png, info);

    // Reservar memoria para los datos de la imagen
    unsigned char *imageData = (unsigned char *)malloc(*size);
    if (!imageData) {
        fclose(file);
        png_destroy_read_struct(&png, &info, NULL);
        perror("Error de asignación de memoria");
        return NULL;
    }

    // Leer y cargar los datos de la imagen
    png_bytep row_pointers[*height];
    for (size_t y = 0; y < *height; y++) {
        row_pointers[y] = imageData + y * *width * png_get_channels(png, info) * (bit_depth / 8);
    }
    png_read_image(png, row_pointers);

    // Finalizar y limpiar
    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);

    return imageData;
}

unsigned char *loadJPEG(const char *filename, size_t *size, size_t *width, size_t *height) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error al abrir el archivo");
        return NULL;
    }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    size_t channels = cinfo.output_components;

    *size = *width * *height * channels;

    unsigned char *imageData = (unsigned char *)malloc(*size);
    if (!imageData) {
        fclose(file);
        jpeg_destroy_decompress(&cinfo);
        perror("Error de asignación de memoria");
        return NULL;
    }

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *row_pointer = imageData + cinfo.output_scanline * cinfo.output_width * cinfo.output_components;
        jpeg_read_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(file);

    return imageData;
}

unsigned char *loadImage(const char *filename, size_t *size, size_t *width, size_t *height, const char **format) {
    const char *extension = strrchr(filename, '.');
    if (!extension) {
        printf("No se pudo determinar el formato de la imagen.\n");
        return NULL;
    }

    // Avanza el puntero para omitir el punto del formato
    extension++;

    *format = extension;

    if (strcmp(extension, "png") == 0) {
        return loadPNG(filename, size, width, height);
    } else if (strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0) {
        return loadJPEG(filename, size, width, height);
    } else if (strcmp(extension, "gif") == 0) {
        return loadGIF(filename, size, width, height);
    } else {
        printf("Formato de imagen no compatible.\n");
        return NULL;
    }
}


void savePNG(const char *filename, unsigned char *data, size_t width, size_t height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error al abrir el archivo");
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(file);
        perror("Error al crear estructura PNG");
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fclose(file);
        png_destroy_write_struct(&png, NULL);
        perror("Error al crear estructura de información PNG");
        return;
    }

    png_init_io(png, file);

    // Configurar los parámetros de la imagen PNG
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // Escribir los datos de la imagen
    png_bytep row_pointers[height];
    for (size_t y = 0; y < height; y++) {
        row_pointers[y] = data + y * width * 4; // 4 canales RGBA
    }
    png_write_image(png, row_pointers);

    // Finalizar y limpiar
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(file);
}

void saveJPEG(const char *filename, unsigned char *data, size_t width, size_t height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error al abrir el archivo");
        return;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, file);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3; // 3 canales: RGB
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &data[cinfo.next_scanline * width * 3]; // 3 canales: RGB
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(file);
}

int saveGIF(const char* filename, unsigned char* data, size_t size, size_t width, size_t height) {
    int error;

    GifFileType* gif = EGifOpenFileName(filename, false, &error);
    if (!gif) {
        fprintf(stderr, "Error al abrir el archivo de salida GIF: %s\n", GifErrorString(error));
        return 1;
    }

    if (EGifPutScreenDesc(gif, width, height, 8, 0, NULL) == GIF_ERROR) {
        fprintf(stderr, "Error al escribir la descripción de la pantalla GIF.\n");
        EGifCloseFile(gif, &error);
        return 1;
    }

    // Esto es necesario para definir una paleta de colores para la imagen GIF
    ColorMapObject* colormap = GifMakeMapObject(256, NULL);
    if (!colormap) {
        fprintf(stderr, "Error al crear el mapa de colores para la imagen GIF.\n");
        EGifCloseFile(gif, &error);
        return 1;
    }

    // Agregar una paleta de colores (en este caso, blanco y negro)
    colormap->Colors[0].Red = 0;
    colormap->Colors[0].Green = 0;
    colormap->Colors[0].Blue = 0;
    colormap->Colors[1].Red = 255;
    colormap->Colors[1].Green = 255;
    colormap->Colors[1].Blue = 255;
    gif->SColorMap = colormap;

    if (EGifPutImageDesc(gif, 0, 0, width, height, 8,colormap) == GIF_ERROR) {
        fprintf(stderr, "Error al escribir la descripción de la imagen GIF.\n");
        EGifCloseFile(gif, &error);
        GifFreeMapObject(colormap); // Liberar el mapa de colores
        return 1;
    }

    if (EGifPutLine(gif, data, size) == GIF_ERROR) {
        fprintf(stderr, "Error al escribir los datos de la imagen GIF.\n");
        EGifCloseFile(gif, &error);
        GifFreeMapObject(colormap); // Liberar el mapa de colores
        return 1;
    }

    EGifCloseFile(gif, &error);
    GifFreeMapObject(colormap); // Liberar el mapa de colores
    return 0;
}

void Histograma(char filename[100],char outputFilename[100]) {
    // Se reserva espacio para el nombre del archivo
    size_t imageSize;
    size_t imageWidth, imageHeight;
    unsigned char *imageData;
    const char *imageFormat;
    imageData = loadImage(filename, &imageSize, &imageWidth, &imageHeight, &imageFormat); 

    if (!imageData) {
        printf("No se cargo una imagen.\n");
    }

    // Determinar el tipo de imagen y realizar el procesamiento adecuado
    if (strcmp(imageFormat, "png") == 0 || strcmp(imageFormat, "jpg") == 0 || strcmp(imageFormat, "jpeg") == 0) {
        // Convertir a escala de grises
        
        
        // Aplicar ecualización de histograma
        

        // Guardar la imagen resultante
        if (strcmp(imageFormat, "png") == 0) {
            convertToGrayscalePNG(imageData, imageSize);
            equalizeHistogram(imageData, imageSize);
            savePNG(outputFilename, imageData, imageWidth, imageHeight);
            printf("imagen con Formato png.\n");
        } else {
            convertToGrayscaleJPG(imageData, imageSize);
            equalizeHistogram(imageData, imageSize);
            saveJPEG(outputFilename, imageData, imageWidth, imageHeight);//Esta vara da error
            printf("imagen con Formato jpg o jpeg.\n");
        }
    } else if (strcmp(imageFormat, "gif") == 0) {
        printf("imagen con Formato gif.\n");
        // Convertir a escala de grises
        convertToGrayscalePNG(imageData, imageSize);

        // Aplicar ecualización de histograma
        equalizeHistogram(imageData, imageSize);

        // Guardar la imagen resultante
        saveGIF(outputFilename, imageData, imageSize, imageWidth, imageHeight);
    } else {
        printf("Formato de imagen no compatible.\n");
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