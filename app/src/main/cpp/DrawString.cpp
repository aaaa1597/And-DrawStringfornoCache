#include <string>
#include <android/log.h>
#include <ft2build.h>
#include <GLES2/gl2.h>
#include <common/unicode/unistr.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include "DrawString.h"

#define DISPLAY_WIDTH   1888.f
#define DISPLAY_HEIGHT  1356.f
#define CHAR_RESOLUTION 720

#ifdef __cplusplus
extern "C" {
#endif

const static char *VERTEXSHADER_FOR_STRING =
        "attribute vec4 a_Position;\n"
        "attribute vec2 a_Texcoord;\n"
        "varying vec2 texcoordVarying;\n"
        "void main() {\n"
        "    gl_Position = a_Position;\n"
        "    texcoordVarying = a_Texcoord;\n"
        "}\n";

const static char *FRAGMENTSHADER_FOR_STRING =
        "precision mediump float;\n"
        "varying vec2 texcoordVarying;\n"
        "uniform sampler2D u_Sampler;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(u_Sampler, texcoordVarying);\n"
        "}\n";

void DrawStringbynoCache(int x, int y, int fontSize, std::string argstr) {
    FT_Library library;
    FT_Error ret = FT_Init_FreeType(&library);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Init_FreeType() ret=%d", ret);

    FT_Face a_face;
    ret = FT_New_Face(library, "/system/fonts/NotoSansJP-Regular.otf", 0, &a_face);
//    ret = FT_New_Face(library, "/data/local/rounded-mgenplus-1pp-regular.ttf", 0, &a_face);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_New_Face() ret=%d", ret);

    // 幅と高さ 水平解像度
    ret = FT_Set_Char_Size(a_face, 0, fontSize << 6, CHAR_RESOLUTION, CHAR_RESOLUTION);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Set_Char_Size() ret=%d", ret);

    /* UTF-8 → UTF-32変換 */
    icu::UnicodeString utf8Str(argstr.c_str(), "UTF-8");
    wchar_t *uft32String = new wchar_t[argstr.length()+1/*BOM*/];
    int utf32length = utf8Str.extract(0, utf8Str.length(), (char*) uft32String, "UTF-32");
    utf32length /= 4;
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa utf32length=%d 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x", utf32length, uft32String[0], uft32String[1], uft32String[2], uft32String[3], uft32String[4]);
                                    /* UTF-32LE BOM */            /* UTF-32BE BOM */
    int loopstart = (uft32String[0]==0x0000feff || uft32String[0]==0xfffe0000) ? 1 : 0;
    int loopend   = (uft32String[utf32length-1]=='\0') ? (utf32length-1) : utf32length;
    int text_image_width = 0, text_image_height = 0;
    for(int lpct = loopstart; lpct < loopend; lpct++) {
        ret = FT_Load_Char(a_face, uft32String[lpct], FT_LOAD_DEFAULT);
        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Load_Char() ret=%d", ret);
        if(ret != 0) continue;

        FT_GlyphSlot slot = a_face->glyph;
        ret = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Render_Glyph() ret=%d", ret);
        if(ret != 0) continue;

        if (slot->format != FT_GLYPH_FORMAT_BITMAP) continue;

        text_image_width += (slot->bitmap_left+slot->bitmap.width);
        if(text_image_height < (slot->bitmap.rows + 3*(slot->bitmap.rows-slot->bitmap_top)))
            text_image_height = slot->bitmap.rows + 3*(slot->bitmap.rows-slot->bitmap_top);
    }
    /* bitmap領域生成 */
    GLvoid *image = calloc(text_image_width * text_image_height, sizeof(GLbyte)*1); /* GL_ALPHA */
    GLubyte *pImage = (GLubyte*)image;
    /* ベースライン算出 */
    int baseline = (a_face->height + a_face->descender) * a_face->size->metrics.y_ppem / a_face->units_per_EM + 1;
    int xoffset = 0; bool isVertical = false;
    for(int lpct = loopstart; lpct < loopend; lpct++) {
        ret = FT_Load_Char(a_face, uft32String[lpct], FT_LOAD_DEFAULT);
        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Load_Char() ret=%d", ret);
        if(ret != 0) continue;

        FT_GlyphSlot slot = a_face->glyph;
        ret = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Render_Glyph() ret=%d", ret);
        if(ret != 0) continue;

        if(slot->format != FT_GLYPH_FORMAT_BITMAP) continue;

        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "lpct=%d left%d top=%d w=%d h=%d", lpct, slot->bitmap_left, slot->bitmap_top, slot->bitmap.width, slot->bitmap.rows);
        for(int dsthct = baseline-slot->bitmap_top, srchct = 0; dsthct < (baseline-slot->bitmap_top+slot->bitmap.rows); dsthct++, srchct++) {
            for(int dstwct = slot->bitmap_left, srcwct = 0; dstwct < (slot->bitmap_left+slot->bitmap.width); dstwct++, srcwct++) {
                if(dstwct < 0 || dsthct < 0 || dstwct >= text_image_width || dsthct >= text_image_height)
                    continue;
                pImage[(dsthct*text_image_width)+dstwct+xoffset] |= slot->bitmap.buffer[(srchct*slot->bitmap.width)+srcwct];
            }
        }
        /* 次の文字のx座標を計算.advanceの分解能は1/65536[pixel] */
        /* advenceはフォント毎に決まっている隣接文字描画時に移動する幅 */
        if(isVertical == true) {
            /* 縦書き */
            if(slot->advance.y)
                baseline += slot->advance.y >> 16;
            else
                baseline += a_face->height;   /* グリフが縦書きに対応していない場合は、フォントサイズで対応する */
        }
        else {
            /* 横書き */
            xoffset += (slot->bitmap_left+slot->bitmap.width);
            __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "slot->advance.x=%d %d", slot->advance.x, slot->bitmap_left+slot->bitmap.width);
        }
    }

    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa width=%d height=%d", text_image_width, text_image_height);
    delete[] uft32String;
    FT_Done_Face(a_face);

    /* シェーダ初期化 */
    int programid = createProgramforDrawString(VERTEXSHADER_FOR_STRING, FRAGMENTSHADER_FOR_STRING);
    if(programid == -1)
        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa createProgramforDrawString() failed=%d", programid);

    /* 頂点座標点 */
    float vertexs[] = {
        (2*x-DISPLAY_WIDTH                 )/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y                  )/DISPLAY_HEIGHT,
        (2*x-DISPLAY_WIDTH                 )/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y-text_image_height)/DISPLAY_HEIGHT,
        (2*x-DISPLAY_WIDTH+text_image_width)/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y                  )/DISPLAY_HEIGHT,
        (2*x-DISPLAY_WIDTH+text_image_width)/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y-text_image_height)/DISPLAY_HEIGHT,
    };
    int positionid = glGetAttribLocation(programid, "a_Position");
    glEnableVertexAttribArray(positionid);
    glVertexAttribPointer(positionid, 2, GL_FLOAT, GL_FALSE, 0, vertexs);

    /* UV座標点 */
    float texcoords[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
    };
    int texcoordid = glGetAttribLocation(programid, "a_Texcoord");
    glEnableVertexAttribArray(texcoordid);
    glVertexAttribPointer(texcoordid, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

    /* テクスチャ初期化 */
    GLuint textureid = -1;
    glGenTextures(1, &textureid);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* サンプラ取得 */
    int samplerid = glGetUniformLocation(programid, "u_Sampler");

    /* プログラム有効化 */
    glUseProgram(programid);

    GLint format = GL_ALPHA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, text_image_width, text_image_height, 0, format, GL_UNSIGNED_BYTE, image);
    free(image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glUniform1i(samplerid, GL_TEXTURE0-GL_TEXTURE0);

    /* 描画実行 */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

//    ret = FT_Done_Face(a_face);
//    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Done_Face() ret=%d", ret);
    ret = FT_Done_FreeType(library);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Done_FreeType() ret=%d", ret);
}

void GetBitmapSizeUsingFTCCMapforDrawString(char isvertical, int fontsize, wchar_t *utf32String, int utf32Length, FTC_ImageTypeRec *font_type, FT_Face lft_face, FT_Int cmap_index, GLsizei *width, GLsizei *height, FTC_CMapCache ftc_mapcache, FTC_ImageCache ftc_imagecache) {
    /* ベースライン算出 */
    int baseline = 0;
    if(lft_face->size != NULL) {
        baseline = (lft_face->height + lft_face->descender) * lft_face->size->metrics.y_ppem / lft_face->units_per_EM + 1;
    }

    int start_x = 0;
    int x_max = 0, y_max = 0;
    GLsizei text_image_w = 0, text_image_h = 0;
    for(int lpct = 0; lpct < utf32Length; lpct++) {
        FT_UInt iGhyhIndex = FTC_CMapCache_Lookup(ftc_mapcache, font_type->face_id, cmap_index, utf32String[lpct]);
        if (!iGhyhIndex) continue;

        FT_Glyph glyph;
        FT_Error ret = FTC_ImageCache_Lookup(ftc_imagecache, font_type, iGhyhIndex, &glyph, NULL);
        if (ret) continue;

        if (glyph->format != FT_GLYPH_FORMAT_BITMAP) continue;

        FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
        x_max = start_x + glyph_bitmap->left + glyph_bitmap->bitmap.width;
        y_max = baseline- glyph_bitmap->top  + glyph_bitmap->bitmap.rows;
        if(x_max > text_image_w) text_image_w = x_max;
        if(y_max > text_image_h) text_image_h = y_max;

        /* 次の文字のx座標を計算.advanceの分解能は1/65536[pixel] */
        /* advenceはフォント毎に決まっている隣接文字描画時に移動する幅 */
        if(isvertical == true) {
            /* 縦書き */
            if(glyph_bitmap->root.advance.y)
                baseline += glyph_bitmap->root.advance.y >> 16;
            else
                baseline += fontsize;   /* グリフが縦書きに対応していない場合は、フォントサイズで対応する */
        }
        else {
            /* 横書き */
            start_x += glyph_bitmap->root.advance.x >> 16;
        }
    }

    text_image_w = (text_image_w + 7) & ~7; /* テクスチャ幅制限(8ピクセル) */
    *width  = text_image_w;
    *height = text_image_h;
    return;
}

GLuint createProgramforDrawString(const char *vertexshader, const char *fragmentshader) {
    GLuint vhandle = loadShaderorDrawString(GL_VERTEX_SHADER, vertexshader);
    if(vhandle == GL_FALSE) return GL_FALSE;

    GLuint fhandle = loadShaderorDrawString(GL_FRAGMENT_SHADER, fragmentshader);
    if(fhandle == GL_FALSE) return GL_FALSE;

    GLuint programhandle = glCreateProgram();
    if(programhandle == GL_FALSE) {
        checkGlErrororDrawString("glCreateProgram");
        return GL_FALSE;
    }

    glAttachShader(programhandle, vhandle);
    checkGlErrororDrawString("glAttachShader(VERTEX_SHADER)");
    glAttachShader(programhandle, fhandle);
    checkGlErrororDrawString("glAttachShader(FRAGMENT_SHADER)");

    glLinkProgram(programhandle);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(programhandle, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE) {
        GLint bufLen = 0;
        glGetProgramiv(programhandle, GL_INFO_LOG_LENGTH, &bufLen);
        if(bufLen) {
            char *logstr = (char*)malloc(bufLen);
            glGetProgramInfoLog(programhandle, bufLen, NULL, logstr);
            __android_log_print(ANDROID_LOG_ERROR, "aaaaa", "glLinkProgram() Fail!!\n %s", logstr);
            free(logstr);
        }
        glDeleteProgram(programhandle);
        programhandle = GL_FALSE;
    }

    return programhandle;
}

GLuint loadShaderorDrawString(int shadertype, const char *sourcestring) {
    GLuint shaderhandle = glCreateShader(shadertype);
    if(!shaderhandle) return GL_FALSE;

    glShaderSource(shaderhandle, 1, &sourcestring, NULL);
    glCompileShader(shaderhandle);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shaderhandle, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shaderhandle, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen) {
            char *logbuf = (char*)malloc(infoLen);
            if(logbuf) {
                glGetShaderInfoLog(shaderhandle, infoLen, NULL, logbuf);
                __android_log_print(ANDROID_LOG_ERROR, "aaaaa", "shader failuer!!\n%s", logbuf);
                free(logbuf);
            }
        }
        glDeleteShader(shaderhandle);
        shaderhandle = GL_FALSE;
    }

    return shaderhandle;
}

void checkGlErrororDrawString(const char *argstr) {
    for(GLuint error = glGetError(); error; error = glGetError())
        __android_log_print(ANDROID_LOG_ERROR, "aaaaa", "after %s errcode=%d", argstr, error);
}

#ifdef __cplusplus
}
#endif
