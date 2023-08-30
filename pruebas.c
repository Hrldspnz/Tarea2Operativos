#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"
#include <string.h>
#include <png.h>
#include <gif_lib.h> 

unsigned char *loadGIF(const char *filename, size_t *size, size_t *width, size_t *height) {
    GifFileType *gif = DGifOpenFileName(filename, NULL);
    if (!gif) {
        perror("Error al abrir el archivo GIF");
        return NULL;
    }

    if (DGifSlurp(gif) != GIF_OK) {
        DGifCloseFile(gif, NULL);
        perror("Error al leer el archivo GIF");
        return NULL;
    }

    *width = gif->SWidth;
    *height = gif->SHeight;
    size_t channels = 3; // GIF se carga en formato RGB

    *size = *width * *height * channels;

    unsigned char *imageData = (unsigned char *)malloc(*size);
    if (!imageData) {
        DGifCloseFile(gif, NULL);
        perror("Error de asignación de memoria");
        return NULL;
    }

    for (size_t i = 0; i < gif->ImageCount; i++) {
        GifImageDesc imageDesc = gif->SavedImages[i].ImageDesc;
        GifByteType *src = gif->SavedImages[i].RasterBits;
        unsigned char *dest = imageData + imageDesc.Top * *width * channels + imageDesc.Left * channels;

        for (size_t row = 0; row < imageDesc.Height; row++) {
            for (size_t col = 0; col < imageDesc.Width; col++) {
                dest[col * channels + 0] = src[col * channels + 0]; // R
                dest[col * channels + 1] = src[col * channels + 1]; // G
                dest[col * channels + 2] = src[col * channels + 2]; // B
            }
            src += imageDesc.Width * channels;
            dest += *width * channels;
        }
    }

    DGifCloseFile(gif, NULL);

    return imageData;
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