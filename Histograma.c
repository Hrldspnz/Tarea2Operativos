#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "jpeglib.h"
#include <string.h>
#include <png.h>
#include <gif_lib.h> 

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
    char filename[100]; // Se reserva espacio para el nombre del archivo
    char outputFilename[100];
    printf("Ingresa el nombre del archivo de imagen: ");
    scanf("%s", filename);
    printf("Ingresa el nombre del archivo de salida: ");
    scanf("%s", outputFilename);
    Histograma(filename,outputFilename);

    return 0;
}