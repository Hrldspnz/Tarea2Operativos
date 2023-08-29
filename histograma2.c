#include <stdio.h>
#include <stdlib.h>
#include <gif_lib.h> // Necesitarás giflib para trabajar con GIF

void equalizeHistogram(unsigned char *data, size_t size) {
    // Implementación de ecualización de histograma (igual que antes)
}

unsigned char *loadPNG(const char *filename, size_t *size) {
    // Implementación de carga de imagen PNG
}

void savePNG(const char *filename, unsigned char *data, size_t size) {
    // Implementación de guardado de imagen PNG
}

unsigned char *loadJPEG(const char *filename, size_t *size) {
    // Implementación de carga de imagen JPEG
}

void saveJPEG(const char *filename, unsigned char *data, size_t size) {
    // Implementación de guardado de imagen JPEG
}

unsigned char *loadGIF(const char *filename, size_t *size) {
    // Implementación de carga de imagen GIF
}

void saveGIF(const char *filename, unsigned char *data, size_t size) {
    // Implementación de guardado de imagen GIF
}

int main() {
    size_t imageSize;
    unsigned char *imageData;

    // Cargar imagen en PNG
    imageData = loadPNG("input.png", &imageSize);
    if (imageData) {
        equalizeHistogram(imageData, imageSize);
        saveJPEG("output.jpg", imageData, imageSize);
        free(imageData);
    }

    // Cargar imagen en JPEG
    imageData = loadJPEG("input.jpg", &imageSize);
    if (imageData) {
        equalizeHistogram(imageData, imageSize);
        savePNG("output.png", imageData, imageSize);
        free(imageData);
    }

    // Cargar imagen en GIF
    imageData = loadGIF("input.gif", &imageSize);
    if (imageData) {
        equalizeHistogram(imageData, imageSize);
        saveGIF("output.gif", imageData, imageSize);
        free(imageData);
    }

    return 0;
}