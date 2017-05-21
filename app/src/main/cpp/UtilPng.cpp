//
// Created by jun on 2017/05/01.
//
#include <android/log.h>
#include "UtilPng.h"

#define LOG_TAG ("PNG")
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

CppPng::CppPng(const std::string& file_name) :
        file_name_(file_name), data_(NULL),
        width_(0), height_(0),
        bit_depth_(0), color_type_(0), interlace_type_(0),
        compression_type_(0), filter_method_(0)
{
    FILE* fp = fopen(file_name_.c_str(), "rb");
    if (fp == NULL) {
        LOGE("%s is not found.", file_name_.c_str());
        fclose(fp);
        return;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL) {
        LOGE("png_create_read_struct error.");
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (info == NULL) {
        LOGE("png_create_info_struct error.");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        LOGE("png_jmpbuf error.");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    unsigned int sig_bytes = 0;
    png_set_sig_bytes(png, sig_bytes);

    png_read_png(png, info, (PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND), NULL);
    png_get_IHDR(png, info, &width_, &height_, &bit_depth_, &color_type_, &interlace_type_, &compression_type_, &filter_method_);

    unsigned int row_bytes = png_get_rowbytes(png, info);
    if (data_ != NULL) {
        delete[] data_;
    }
    data_ = new unsigned char[row_bytes * height_];

    png_bytepp rows = png_get_rows(png, info);
    for (int i = 0; i < height_; ++i) {
        memcpy(data_ + (row_bytes * i), rows[i], row_bytes);
    }

    png_destroy_read_struct(&png, &info, NULL);
}

CppPng::CppPng(const std::string& writefile_name, int color_type, int w, int h, void *img) :
        file_name_(writefile_name), data_(NULL),
        width_(w), height_(h),
        bit_depth_(0), color_type_(color_type), interlace_type_(0),
        compression_type_(0), filter_method_(0)
{
    int row_size = sizeof(png_byte) * width_;
    switch(color_type) {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            break;
        case PNG_COLOR_TYPE_RGB:
            row_size *= 3;
            break;
        case PNG_COLOR_TYPE_RGBA:
            row_size *= 4;
            break;
        default:
            throw "colortype is illigal argument!!";
    }


    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png == NULL)
        throw "png_create_write_struct() failed!!";

    png_infop info = png_create_info_struct(png);
    if(info == NULL)
        throw "png_create_info_struct() failed!!";

    if(setjmp(png_jmpbuf(png)))
        throw "setjmp(png_jmpbuf(png)) failed!!";

    FILE* fp = fopen(file_name_.c_str(), "wb");
    if(fp == NULL)
        throw "fopen() failed!!";

    png_init_io(png, fp);
    png_set_IHDR(png, info, width_, height_, 8,
                 color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_bytepp rows = (png_bytepp)png_malloc(png, sizeof(png_bytep) * height_);
    if(rows == NULL)
        throw "png_malloc() failed!!";
    png_set_rows(png, info, rows);

    memset(rows, 0, sizeof(png_bytep) * height_);
    for(int y = 0; y < height_; y++) {
        if ((rows[y] = (png_byte*)png_malloc(png, row_size)) == NULL)
            throw "png_malloc() failed!!";
    }

    char *imgdata = (char*)img;
    switch(color_type) {
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            for(int y = 0; y < height_; y++) {
                png_bytep row = rows[y];
                for (int x = 0; x < width_; x++) {
                    *row++ = imgdata[y*width_+x];
                }
            }
            break;
        case PNG_COLOR_TYPE_RGB:  // RGB
            for(int y = 0; y < height_; y++) {
                png_bytep row = rows[y];
                for(int x = 0; x < width_; x++) {
                    *row++ = imgdata[y*width_*3+x*3+0];
                    *row++ = imgdata[y*width_*3+x*3+1];
                    *row++ = imgdata[y*width_*3+x*3+2];
                }
            }
            break;
        case PNG_COLOR_TYPE_RGBA:  // RGBA
            for(int y = 0; y < height_; y++) {
                png_bytep row = rows[y];
                for(int x = 0; x < width_; x++) {
                    *row++ = imgdata[y*width_*4+x*4+0];
                    *row++ = imgdata[y*width_*4+x*4+1];
                    *row++ = imgdata[y*width_*4+x*4+2];
                    *row++ = imgdata[y*width_*4+x*4+3];
                }
            }
            break;
    }

    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
    if(rows != NULL) {
        for(int y = 0; y < height_; y++)
            png_free(png, rows[y]);
        png_free(png, rows);
    }
    png_destroy_write_struct(&png, &info);
}

CppPng::~CppPng()
{
    if (data_) delete[] data_;
}

unsigned int CppPng::get_width()
{
    return width_;
}

unsigned int CppPng::get_height()
{
    return height_;
}

unsigned char* CppPng::get_data()
{
    return data_;
}

int CppPng::get_colortype()
{
    return color_type_;
}

int CppPng::get_bitdepth()
{
    return bit_depth_;
}

bool CppPng::has_alpha()
{
    if (color_type_ == PNG_COLOR_TYPE_RGBA) {
        return true;
    }
    return false;
}
