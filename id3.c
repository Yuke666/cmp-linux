#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "id3.h"
#include "utils.h"

struct id3_header {
    unsigned char file_identifier[3];
    unsigned char flags;
    uint16_t version;
    uint32_t size;
};

struct id3_frame {
    char id[5];
    uint32_t size;
    uint16_t flags;
};


static int getU32( uint32_t *val){
    uint32_t b0 = (*val & 0x000000FF);
    uint32_t b1 = (*val & 0x0000FF00) >> 8;
    uint32_t b2 = (*val & 0x00FF0000) >> 16;
    uint32_t b3 = (*val & 0xFF000000) >> 24;

    *val = b0 << 24 | b1 << 16 | b2 << 8 | b3;
    return 1;
}

static int getU24( uint32_t *val){
    uint32_t b0 = (*val & 0x000000FF);
    uint32_t b1 = (*val & 0x0000FF00) >> 8;
    uint32_t b2 = (*val & 0x00FF0000) >> 16;

    *val =  b0 << 16 | b1 << 8 | b2;
    return 1;
}

static int unsync( uint32_t *val){
    uint32_t b0 = (*val & 0x000000FF);
    uint32_t b1 = (*val & 0x0000FF00) >> 8;
    uint32_t b2 = (*val & 0x00FF0000) >> 16;
    uint32_t b3 = (*val & 0xFF000000) >> 24;

    if(b0 >= 0x80 || b1 >= 0x80 || b2 >= 0x80 || b3 >= 0x80) return 0;
    *val = b0 << 21 | b1 << 14 | b2 << 7 | b3;

    return 1;
}

static int ReadFrame(char *into, FILE *fp, struct id3_frame f){

    uint32_t size = abs(f.size);
    if(size == 0 || size > 2048) return 0;

    char temp[size];
    char temp2[size];
    memset(temp, 0, size);
    memset(temp2, 0, size);

    fread(temp,sizeof(char), size, fp);

    int k, j = 0;
    for(k = 0; k < size; k++)
        if(temp[k] > 31 && temp[k] < 127)
            temp2[j++] = temp[k];  

    k = strlen(temp2);
    while(temp2[k-1] == ' '){ k--; }

    temp2[k] = '\0';
    strcpy(into, temp2);

    return 1;
}

static int ReadPicture(SongImage *image, FILE *fp){

    char text_encoding;
    char mime_type[64];
    char picture_type;
    // char discription[64];

    fread(&text_encoding, sizeof(char), 1, fp);
    
    int k;
    char ch = ' ';
    for(k = 0; k < 64 && ch != 0; k++) {
        ch = fgetc(fp);
        mime_type[k] = ch;
    }
    
    fread(&picture_type, sizeof(char), 1, fp);

    ch = ' ';
    for(k = 0; k < 64 && ch != 0; k++) {
        ch = fgetc(fp);
        // discription[k] = ch;
    }

    if(strcmp((const char *)mime_type, "image/jpeg") == 0){
        Utils_LoadJPEG(fp, image);
    }

    if(strcmp((const char *)mime_type, "image/png") == 0){
        Utils_LoadPNG(fp, image);
    }

    return 0;
}

static int isFrameIDchar(char c){
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z');
}

static void ReadInto(void *into, void *from, int numOfBytes){
    int k;
    for(k = 0; k < numOfBytes; k++) ((char *)into)[k] = ((char *)from)[k];
}

int ID3_GetTagSize(char *buff){
    struct id3_header currHeader;
    ReadInto(&currHeader.file_identifier, &buff[0], 3);
    if(strncmp((const char *)currHeader.file_identifier, "ID3", 3) != 0) return 0;
    ReadInto(&currHeader.version, &buff[3], 2);
    ReadInto(&currHeader.flags, &buff[5], 1);
    ReadInto(&currHeader.size, &buff[6], 4);
    if(currHeader.version == 4) unsync(&currHeader.size);
    if(currHeader.version == 3) getU32(&currHeader.size);
    if(currHeader.version == 2) getU24(&currHeader.size);
    if(currHeader.flags && (1 << 4))
        return currHeader.size + 20;
    else 
        return currHeader.size + 10;
    return 0;
}

void ID3_Open(const char *filename, SongInfo *info){
    
    int k;
    char track[10];

    memset(info, 0, sizeof(SongInfo));
    struct id3_frame currFrame;
    struct id3_header currHeader;

    FILE *mp3 = fopen(filename, "rb");
    if(mp3 == NULL) return;
    fread(&currHeader.file_identifier,sizeof(unsigned char), 3, mp3);
    fread(&currHeader.version,sizeof(uint16_t), 1, mp3);
    fread(&currHeader.flags,sizeof(unsigned char), 1, mp3);
    fread(&currHeader.size,sizeof(uint32_t), 1, mp3);
    
    if(currHeader.version == 4) unsync(&currHeader.size);
    if(currHeader.version == 3) getU32(&currHeader.size);
    if(currHeader.version == 2) getU24(&currHeader.size);
    if(currHeader.version < 2) { fclose(mp3); return; }

    while(ftell(mp3) < currHeader.size){

        if(currHeader.version != 2){
            fread(&currFrame.id,sizeof(char), 4, mp3);
            fread(&currFrame.size,sizeof(char), 4, mp3);
            fread(&currFrame.flags,sizeof(char), 2, mp3);
            currFrame.id[4] = 0;
        } else {
            fread(&currFrame.id,sizeof(char), 3, mp3);
            fread(&currFrame.size,sizeof(char), 3, mp3);
            currFrame.flags = 0;
            currFrame.id[3] = 0;
        }

        if(currHeader.version == 4) unsync(&currFrame.size);
        if(currHeader.version == 3) getU32(&currFrame.size);
        if(currHeader.version == 2) getU24(&currFrame.size);
    
        int lessThan = currHeader.version > 2 ? 4 : 3;

        int k;
        for(k = 0; k < lessThan; k++) 
            if(!isFrameIDchar(currFrame.id[k]) || (k == lessThan-1 && currFrame.id[k] == '\0')) 
                goto out;

        if(!currFrame.size) break;
        if(currFrame.size > ftell(mp3) + currHeader.size) break;

        if(strcmp(currFrame.id, "TIT2") == 0) ReadFrame(info->title, mp3, currFrame);
        else if(strcmp(currFrame.id, "TALB") == 0) ReadFrame(info->album, mp3, currFrame);
        else if(strcmp(currFrame.id, "TYER") == 0) ReadFrame(info->year, mp3, currFrame);
        else if(strcmp(currFrame.id, "TRCK") == 0) ReadFrame(track, mp3, currFrame);
        else if(strcmp(currFrame.id, "TCON") == 0) ReadFrame(info->contentType, mp3, currFrame);
        else if(strcmp(currFrame.id, "TCOM") == 0) ReadFrame(info->composer, mp3, currFrame);
        else if(strcmp(currFrame.id, "TPE3") == 0) ReadFrame(info->artist, mp3, currFrame);
        else if(strcmp(currFrame.id, "TPE2") == 0) ReadFrame(info->artist, mp3, currFrame);
        else if(strcmp(currFrame.id, "TPE1") == 0) ReadFrame(info->artist, mp3, currFrame);
        else if(strcmp(currFrame.id, "APIC") == 0) ReadPicture(&info->image, mp3);
        else if(strcmp(currFrame.id, "PIC") == 0) ReadPicture(&info->image, mp3);
        else if(strcmp(currFrame.id, "TAL") == 0) ReadFrame(info->album, mp3, currFrame);
        else if(strcmp(currFrame.id, "TP1") == 0) ReadFrame(info->artist, mp3, currFrame);
        else if(strcmp(currFrame.id, "TP2") == 0) ReadFrame(info->artist, mp3, currFrame);
        else if(strcmp(currFrame.id, "TRK") == 0) ReadFrame(track, mp3, currFrame);
        else if(strcmp(currFrame.id, "TT2") == 0) ReadFrame(info->title, mp3, currFrame);
        else if(strcmp(currFrame.id, "TYE") == 0) ReadFrame(info->year, mp3, currFrame);
        else if(strcmp(currFrame.id, "TCO") == 0) ReadFrame(info->contentType, mp3, currFrame);
        else {
            char temp[1024];
            ReadFrame(temp, mp3, currFrame);
        }
    }

    out:

    k = 0;
    while(k < strlen(track) && track[k] != '/'){ k++; }

    track[k] = 0;
    info->track = atoi(track);

    fclose(mp3);
}