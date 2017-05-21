#ifndef DRAWSTRING_H__
#define DRAWSTRING_H__

#ifdef __cplusplus
extern "C" {
#endif

GLuint createProgramforDrawString(const char *vertexshader, const char *fragmentshader);
GLuint loadShaderorDrawString(int shadertype, const char *sourcestring);
void checkGlErrororDrawString(const char *argstr);

void DrawStringbynoCache(int x, int y, int fontSize, std::string argstr);
void GetBitmapSizeforDrawString(char vertical, int size, wchar_t *utf32String, int utf32Length, FTC_ImageTypeRec *font_type, FT_Face lft_face, FT_Int cmap_index, GLsizei *width, GLsizei *height, FTC_CMapCache ftc_mapcache, FTC_ImageCache ftc_imagecache);

#ifdef __cplusplus
}
#endif

#endif //DRAWSTRING_H__
