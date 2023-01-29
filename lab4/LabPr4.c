#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

typedef unsigned char byte;

typedef enum option{
    SHOW,
    GET,
    SET,
    ERR
}option_t;

typedef struct frame{
    char name[5];
    int size;
    char *content;
}frame;

option_t get_op(int argc, char *argv[], char **propname, char **propvalue, char **filename) {
    if (argc < 3) {
        printf("Too few arguments\n");
        return ERR;
    }

    (*propname) = (*propvalue) = (*filename) = NULL;

    if (argv[1] == strstr(argv[1], "--filepath=")) {
        (*filename) = argv[1] + sizeof("--filepath=") - 1;
    } else {
        return ERR;
    }

    if (argc == 3) {
        if (strcmp(argv[2],"--show") == 0){
            return SHOW;
        } else if (argv[2] == strstr(argv[2], "--get=")) {
            (*propname) = argv[2] + sizeof("--get=") - 1;
            return GET;
        } else {
            return ERR;
        }
    } else if (argc == 4) {
        if (argv[2] == strstr(argv[2], "--set=") && argv[3] == strstr(argv[3], "--value=")) {

            (*propname) = argv[2] + sizeof("--set=") - 1;

            (*propvalue) = argv[3] + sizeof("--value=") - 1;

            return SET;
        } else {
            return ERR;
        }
    } else {
        return ERR;
    }

    return ERR;
}

void get_tag(FILE *file, int *size){
    byte bsize[4];
    char ssize[5];
    int i = 0;
    char tag[3];
    int lsize, tmp;

    if ((fread(&tag, sizeof(char), 3, file) != 3) || (strcmp(tag, "ID3") != 0)) {
        exit(-1);
    }
    printf("TAG: %s\n", tag);

    fseek(file, 3, SEEK_CUR);

    fread(&bsize, sizeof(bsize), 1, file);

    (*size) = ((bsize[0] << 21) | (bsize[1] << 14) | (bsize[2] << 7) | (bsize[3]));
    (*size) += 10;
    printf ("Size of data: %d\n", *size);
}

int get_frame(FILE *file, frame *fr, int *unicode) {
    byte bsize[4];

    fread(fr->name, sizeof(char), 4, file);
    if (fr->name && !fr->name[0])  {
        return -1;
    }
    fr->name[4] = '\0';

    fread(&bsize, sizeof(byte), 4, file);
    fr->size = ((bsize[0] << 24) | (bsize[1] << 16) | (bsize[2] << 8) | (bsize[3]));

    fseek(file, 2, SEEK_CUR);
    (*unicode) = fgetc(file);

    fr->content = (char*)malloc((fr->size)*sizeof(char));
    fread(fr->content, sizeof(char), (fr->size-1), file);
    fr->content[fr->size-1] = '\0';
    return 0;
}

void move_data(long start, long end, int shift, FILE *file) {
    if (!shift) {
        return;
    }
    byte buf[1024];
    long svend = end;
    long svpos = ftell(file);
    long size = end - start;
    long pos;
    int chunksize;
    while (size > 0) {
        chunksize = size%1024;
        if (!chunksize) {
            chunksize = 1024;
        }
        if (shift > 0) {
            pos = end - chunksize;
        } else {
            pos = start;
        }
        fseek(file, pos, SEEK_SET);
        fread(buf, sizeof(byte), chunksize, file);
        fseek(file, pos + shift, SEEK_SET);
        fwrite(buf, sizeof(byte), chunksize, file);
        if (shift > 0) {
            end -=chunksize;
        } else {
            start += chunksize;
        }
        size = end - start;
    }
    if (shift < 0) {
        fseek(file, svend+shift, SEEK_SET);
        while (shift++ < 0) {
            fputc(0, file);
        }
    }
    fseek(file, svpos, SEEK_SET);
}

void update_frame (long frpos, char *propval, FILE *file) {
    long svpos = ftell(file);
    fseek(file, frpos+4, SEEK_SET);
    int size = strlen(propval) + 1;
    fputc((byte)(size>>24), file);
    fputc((byte)(size>>16), file);
    fputc((byte)(size>>8), file);
    fputc((byte)(size), file);
    fputc(0, file);
    fputc(0, file);
    fputc(0, file);
    fwrite(propval, sizeof(char), size -1, file);
    fseek(file, svpos, SEEK_SET);
}

void update_tag_size(FILE *file, int size) {
    fseek(file, 6, SEEK_SET);
    fputc((byte)(size>>21)&0x7F, file);
    fputc((byte)(size>>14)&0x7F, file);
    fputc((byte)(size>>7)&0x7F, file);
    fputc((byte)(size)&0x7F, file);
}

long get_file_size(FILE *file) {
    long size;
    long pos = ftell(file);
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    fseek(file, pos, SEEK_SET);
    return size;
}

int main(int argc, char *argv[]) {
    frame fr;
    int size;
    option_t option;
    int unicode;
    char *filename, *propname, *propvalue;

    option = get_op(argc, argv, &propname, &propvalue, &filename);
    if (option == ERR) {
        printf("Usage: LabPr4.exe --filepath=[FILENAME] [OPTION]");
    }

    FILE *file = fopen(filename, "rb+");
    if(!file) {
        printf("Not file");
        return -1;
    }

    get_tag(file, &size);

    char *locale = setlocale(LC_ALL,"");

    if (option == SHOW) {
        printf("Name of frame  Size of frame  Content of frame\n");
        while ((ftell(file) < size) && (get_frame(file, &fr, &unicode) != -1)) {
            if (unicode) {
                printf("%8s       %7d        %ls", fr.name, fr.size, (unsigned short *)fr.content + 1);
            } else {
                printf("%8s       %7d        %s", fr.name, fr.size, fr.content);
            }
            printf("\n");
            free(fr.content);
        }
    }

    if (option == GET) {
        int flag = 0;
        while ((ftell(file) < size) && (get_frame(file, &fr, &unicode) != -1) && (flag == 0)){
            if (strcmp(propname, fr.name) == 0) {
                if (unicode) {
                    printf("Name of frame: %s\nSize of frame: %d\nContent of frame: %ls", fr.name, fr.size, (unsigned short *)fr.content + 1);
                } else {
                    printf("Name of frame: %s\nSize of frame: %d\nContent of frame: %s", fr.name, fr.size, fr.content);
                }
                flag = 1;
                printf("\n");
            }
            free(fr.content);
        }
        if (flag == 0) {
            printf("Can not find this frame\n");
        }
    }

    if (option == SET) {
        long endpos;
        int frsize;
        long frpos = 0;
        int flag = 0;
        while ((ftell(file) < size) && (get_frame(file, &fr, &unicode) != -1)){
            if ((strcmp(propname, fr.name) == 0) && (flag == 0)) {
                frpos = ftell(file);
                frsize = fr.size;
                flag = 1;
            }
            endpos = ftell(file);
            free(fr.content);
        }
        if (flag == 0) {
            printf("Can not find this frame\n");
        } else {
            int shift = strlen(propvalue) + 1 - frsize;
            if (size - endpos >= shift) {
                move_data(frpos, endpos, shift, file);
                update_frame(frpos - frsize - 10, propvalue, file);
            } else {
                move_data(frpos, get_file_size(file), shift, file);
                update_frame(frpos - frsize - 10, propvalue, file);
                size = size + shift;
                update_tag_size(file, size - 10);
            }
        }
    }
    

    fclose(file);

    return 0;
}