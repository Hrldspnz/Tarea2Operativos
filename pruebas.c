#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"
#include <string.h>

unsigned char *loadGIF(const char *filename, size_t *size, size_t *width, size_t *height) {
    return "s";
}

unsigned char *loadPNG(const char *filename, size_t *size, size_t *width, size_t *height) {
    return "s";
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

int main() {
    char filename[100]; // Se reserva espacio para el nombre del archivo
    printf("Ingresa el nombre del archivo de imagen: ");
    scanf("%s", filename);

    size_t imageSize;
    size_t imageWidth, imageHeight;
    unsigned char *imageData;
    const char *imageFormat;
    imageData = loadImage(filename, &imageSize, &imageWidth, &imageHeight, &imageFormat); 

    if (imageData) {
        printf("Carga completa\n");
        // Aquí puedes realizar cualquier otra operación que necesites con la imagen cargada
        // Por ejemplo, liberar la memoria cuando ya no la necesites: free(imageData);
    }

    return 0;
}

/*int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <input_image> <output_image>\n", argv[0]);
        return 1;
    }

    const char *inputFilename = argv[1];
    const char *outputFilename = argv[2];

    size_t imageSize;
    size_t imageWidth, imageHeight;
    unsigned char *imageData;
    const char *imageFormat;

    // Cargar imagen y determinar formato
    imageData = loadImage(inputFilename, &imageSize, &imageWidth, &imageHeight, &imageFormat);
    if (!imageData) {
        return 1;
    }

    // Determinar el tipo de imagen y realizar el procesamiento adecuado
    if (strcmp(imageFormat, "png") == 0 || strcmp(imageFormat, "jpg") == 0 || strcmp(imageFormat, "jpeg") == 0) {
        // Convertir a escala de grises
        convertToGrayscale(imageData, imageSize);

        // Aplicar ecualización de histograma
        equalizeHistogram(imageData, imageSize);

        // Guardar la imagen resultante
        if (strcmp(imageFormat, "png") == 0) {
            savePNG(outputFilename, imageData, imageWidth, imageHeight);
        } else {
            saveJPEG(outputFilename, imageData, imageWidth, imageHeight);
        }
    } else if (strcmp(imageFormat, "gif") == 0) {
        // Convertir a escala de grises
        convertToGrayscale(imageData, imageSize);

        // Aplicar ecualización de histograma
        equalizeHistogram(imageData, imageSize);

        // Guardar la imagen resultante
        saveGIF(outputFilename, imageData, imageWidth, imageHeight);
    } else {
        printf("Formato de imagen no compatible.\n");
    }
}*/