//
// Created by jun on 2017/05/01.
//

#ifndef INC_3RDPARTYLIBPNG_CPPPNG_H
#define INC_3RDPARTYLIBPNG_CPPPNG_H

#include <string>
#include "png.h"

class CppPng {
public:
    CppPng(const std::string &file_name);
    CppPng(const std::string &writefile_name, int colortype, int w, int h, void *img);
    ~CppPng();
    unsigned int get_width();
    unsigned int get_height();
    int get_colortype();
    int get_bitdepth();
    bool has_alpha();
    unsigned char* get_data();

private:
    const std::string file_name_;
    unsigned char* data_;
    png_uint_32 width_, height_;
    int bit_depth_, color_type_, interlace_type_;
    int compression_type_, filter_method_;
};


#endif //INC_3RDPARTYLIBPNG_CPPPNG_H
