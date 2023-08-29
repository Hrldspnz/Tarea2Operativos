#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <png.h>
#include <gif_lib.h>

int getJPEGDimensions(const char *filename, int *width, int *height) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error al abrir el archivo");
        return 0;
    }

    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);

    *width = cinfo.image_width;
    *height = cinfo.image_height;

    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    return 1;
}

int getPNGDimensions(const char *filename, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error al abrir el archivo");
        return 0;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        perror("Error al crear la estructura de lectura PNG");
        return 0;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        perror("Error al crear la estructura de información PNG");
        return 0;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    return 1;
}

int getGIFDimensions(const char *filename, int *width, int *height) {
    GifFileType *gif_file = DGifOpenFileName(filename, NULL);
    if (!gif_file) {
        perror("Error al abrir el archivo GIF");
        return 0;
    }

    *width = gif_file->SWidth;
    *height = gif_file->SHeight;

    DGifCloseFile(gif_file, NULL);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s archivo\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int width, height;

    if (getJPEGDimensions(filename, &width, &height)) {
        printf("Tipo: JPEG\n");
        printf("Ancho: %d\n", width);
        printf("Altura: %d\n", height);
    } else if (getPNGDimensions(filename, &width, &height)) {
        printf("Tipo: PNG\n");
        printf("Ancho: %d\n", width);
        printf("Altura: %d\n", height);
    } else if (getGIFDimensions(filename, &width, &height)) {
        printf("Tipo: GIF\n");
        printf("Ancho: %d\n", width);
        printf("Altura: %d\n", height);
    } else {
        fprintf(stderr, "No se pudo obtener información de dimensiones para el archivo\n");
        return 1;
    }

    return 0;
}