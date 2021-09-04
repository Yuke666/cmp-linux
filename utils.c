#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <jpeglib.h>
#include <jerror.h>
#include <png.h>
#include "utils.h"
#include "ui.h"

void Utils_FormatStr(WINDOW *win, int len, const char *format, ...){
    
    char buffer[len];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, len, format, args);
    va_end(args);

    char commaStrs[3][1024];
    int commas = 0;

    int lastCommaPos = 0;
    
    int k;
    for(k = 0; k < len; k++){
        if(buffer[k] == (char)FORMAT_STR_SEPERATOR || k == strlen(buffer)) {
            
            int nextCommaPos = k;

            int strLength = nextCommaPos - lastCommaPos;

            memcpy(commaStrs[commas++], &buffer[lastCommaPos], strLength);
            commaStrs[commas-1][strLength] = '\0';

            lastCommaPos = k+1;
            if(commas == 3) break;
        }
    }

    memset(buffer, ' ', sizeof(buffer));
    memcpy(buffer, commaStrs[0], strlen(commaStrs[0]));

    int nextPos = len/2 - (strlen(commaStrs[1])/2);
    if(nextPos + strlen(commaStrs[1]) <= len)
        memcpy(&buffer[nextPos], commaStrs[1], strlen(commaStrs[1]));

    nextPos = len - strlen(commaStrs[2]);
    if(nextPos + strlen(commaStrs[2]) <= len)
        memcpy(&buffer[nextPos], commaStrs[2], strlen(commaStrs[2]));

    buffer[len] = '\0';

    wprintw(win, "%s", buffer);
}

void Utils_DrawSongImageAscii(SongImage *img, int xPos, int yPos, int maxSize, int useColor){
    if(!img->pixels) return;

    char *characters = " ..,;accceeexxxddeeCCXXXWWW";
    int xPlus = 1;
    int yPlus = 1;

    xPlus = ceil((float)img->width / (float)maxSize);
    yPlus = xPlus;

    int x, y;
    for(y = 0; y < img->height && (y/(yPlus*2))+yPos < LINES; y+=yPlus*2){
        for(x = 0; x < img->width && (x/xPlus)+xPos < COLS; x+=xPlus){

            move((y/(yPlus*2))+yPos-1, (x/xPlus)+xPos);

            int r = img->pixels[(((y*img->width) + x)*img->channels)  ] & 0xFF;
            int g = img->pixels[(((y*img->width) + x)*img->channels)+1] & 0xFF;
            int b = img->pixels[(((y*img->width) + x)*img->channels)+2] & 0xFF;
            int a = img->channels == 4 ? img->pixels[(((y*img->width) + x)*img->channels)+3] & 0xFF : 3;

            int color = COLOR_PAIR_DEFAULT;
            if(useColor){
                if( round(r/20)*20 >  round(g/20)*20 && round(r/20)*20 >  round(b/20)*20) color = RED_BLACK_COLOR_PAIR;
                if( round(g/20)*20 >  round(r/20)*20 && round(g/20)*20 >  round(b/20)*20) color = GREEN_BLACK_COLOR_PAIR;
                if( round(b/20)*20 >  round(r/20)*20 && round(b/20)*20 >  round(g/20)*20) color = CYAN_BLACK_COLOR_PAIR;
                if( round(r/20)*20 == round(b/20)*20 && round(r/20)*20 >  floor(g/20)*20) color = MAGENTA_BLACK_COLOR_PAIR;
                if( round(r/20)*20 == round(g/20)*20 && round(g/20)*20 >  floor(b/20)*20) color = YELLOW_BLACK_COLOR_PAIR;
                if( round(g/10)*10 == round(r/10)*10 && round(r/10)*10 == floor(b/10)*10) color = COLOR_PAIR_DEFAULT;
            }

            attron(COLOR_PAIR(color));
            int gray = (0.21*r + 0.72*g + 0.07*b) * a;

            if(gray > 240)      printw("%c", characters[25]);
            else if(gray > 230) printw("%c", characters[24]);
            else if(gray > 220) printw("%c", characters[23]);
            else if(gray > 210) printw("%c", characters[22]);
            else if(gray > 200) printw("%c", characters[21]);
            else if(gray > 190) printw("%c", characters[20]);
            else if(gray > 180) printw("%c", characters[19]);
            else if(gray > 170) printw("%c", characters[18]);
            else if(gray > 160) printw("%c", characters[17]);
            else if(gray > 150) printw("%c", characters[16]);
            else if(gray > 140) printw("%c", characters[15]);
            else if(gray > 130) printw("%c", characters[14]);
            else if(gray > 120) printw("%c", characters[13]);
            else if(gray > 110) printw("%c", characters[12]);
            else if(gray > 100) printw("%c", characters[11]);
            else if(gray > 90)  printw("%c", characters[10]);
            else if(gray > 80)  printw("%c", characters[9]);
            else if(gray > 70)  printw("%c", characters[8]);
            else if(gray > 60)  printw("%c", characters[7]);
            else if(gray > 50)  printw("%c", characters[6]);
            else if(gray > 40)  printw("%c", characters[5]);
            else if(gray > 30)  printw("%c", characters[4]);
            else if(gray > 20)  printw("%c", characters[3]);
            else if(gray > 10)  printw("%c", characters[2]);
            else if(gray > 0)   printw("%c", characters[1]);
            else if(gray == 0)  printw("%c", characters[0]);
            attroff(COLOR_PAIR(color));
        }
    }
}

int Utils_LoadPNG(FILE *fp, SongImage *img){

    unsigned char header[8];
    fread(header, 1, 8, fp);
    int ispng = !png_sig_cmp(header, 0, 8); 

    if(!ispng){
        return 0;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        return 0;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }

    if(setjmp(png_jmpbuf(png_ptr))){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }

    png_set_sig_bytes(png_ptr, 8);
    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int bit_depth, color_type;
    png_uint_32 twidth, theight;

    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) 
        png_set_tRNS_to_alpha(png_ptr);

    if(bit_depth < 8)
        png_set_packing(png_ptr);

    if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
        png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);

    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    img->width = twidth;
    img->height = theight;

    png_read_update_info(png_ptr, info_ptr);

    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    png_byte *image_data = (png_byte *)malloc(sizeof(png_byte) * rowbytes * img->height);
    if(!image_data){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(img);
        return 0;
    }

    png_bytep *row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * img->height);
    if(!row_pointers){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(image_data);
        free(img);
        return 0;
    }

    int i;
    for( i = 0; i < img->height; ++i)
        row_pointers[img->height - 1 - i] = &image_data[(img->height - 1 - i) * rowbytes];
    
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, NULL);

    img->pixels = (char *)image_data;
    img->channels = 4;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if(row_pointers) free(row_pointers);

    return 1;
}

int Utils_LoadJPEG(FILE *fp, SongImage *image){
    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr jerror;
    jmp_buf jmp_buffer;
    int err = 0;


    info.err = jpeg_std_error(&jerror);

    void func(j_common_ptr cinfo){
        err = 1;
        longjmp(jmp_buffer, 1);
    }

    info.err->error_exit = func;
    if(setjmp(jmp_buffer)){
        jpeg_destroy_decompress(&info);
        return 0;
    }

    if(err) {
        jpeg_finish_decompress(&info);
        jpeg_destroy_decompress(&info);
        return 0;
    }

    jpeg_create_decompress(&info);
    jpeg_stdio_src(&info, fp);
    jpeg_read_header(&info, TRUE);
    jpeg_start_decompress(&info);

    image->pixels = NULL;
    image->width =  info.output_width;
    image->height = info.output_height;
    image->channels = info.num_components;

    int data_size = image->width * image->height * image->channels;
    image->pixels = (char *)malloc(sizeof(char) * data_size);
    char *rowptr[1];
    while(info.output_scanline < info.output_height){
        rowptr[0] = (char *)image->pixels + 3 * info.output_width * info.output_scanline;
        jpeg_read_scanlines(&info, (JSAMPARRAY)rowptr, 1);
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);

    return 1;
}


int Utils_GetExt(char *str){

    char ext[512];
    
    int k, j = 0;
    for(k = strlen(str); k > 0 && str[k] != '.'; k--){}
    for(; k < strlen(str); k++) ext[j++] = str[k]; 
    
    ext[j] = '\0';

    if(strcmp(ext, ".mp3") == 0) return EXTENSION_MP3;
    if(strcmp(ext, ".flac") == 0) return EXTENSION_FLAC;
    if(strcmp(ext, ".png") == 0) return EXTENSION_PNG;
    if(strcmp(ext, ".jpg") == 0) return EXTENSION_JPEG;
    if(strcmp(ext, ".jpeg") == 0) return EXTENSION_JPEG;

    return EXTENSION_UNSUPPORTED;
}

int Utils_QSortStr(const void *one, const void *two){
    int k = 0;
    if((((char*)one)[0]) > ((char*)two)[0]) return 1;
    if((((char*)one)[0]) == ((char*)two)[0]){
        while(((char*)two)[k] == ((char*)two)[k] && k < strlen(((char*)one)) && k < strlen(((char*)two))){ k++; }
        if(((char*)one)[k] < ((char*)two)[k]) return -1;
        if(((char*)one)[k] > ((char*)two)[k]) return 1;
        if(((char*)one)[k] == ((char*)two)[k]) return 0;
    }
    if((((char*)one)[0]) < ((char*)two)[0]) return -1;
    return 0;
}
