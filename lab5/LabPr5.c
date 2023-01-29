#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>

#pragma pack(1)

#define OPTCNT 4
#define INPUT 0
#define OUTPUT 1
#define ITER 2
#define FREQ 3

int get_options(int argc, char *argv[], char *options[], char **originalgen, char **directory, int *maxiter, int *dumpfreq) {
    if (argc < 2) {
        return 1; 
    }

    int found[OPTCNT];
    found[3] = found[2] = found[1] = found[0] = 0;

    (*maxiter) = 0;
    (*dumpfreq) = 1;
    
    for (int i = 1; i < argc - 1; i = i + 2) {
        for (int j = 0; j < OPTCNT; ++j) {
            if (strcmp(options[j], argv[i]) == 0) {
                found[j] = 1;
                if (j == INPUT) {
                    (*originalgen) = argv[i+1];
                } else if (j == OUTPUT) {
                    (*directory) = argv[i+1];
                } else if (j == ITER) {
                    (*maxiter) = atoi(argv[i+1]);
                } else {
                    (*dumpfreq) = atoi(argv[i+1]);
                }
                break;
            }
        }
    }

    if ((found[INPUT] != 0) && (found[OUTPUT] != 0)) {
        return 0;
    } else {
        return 1;
    }

    return 0;
}

typedef struct BitmapFileHeader {
    char type[2];
    uint32_t size;
    uint16_t reserv1;
    uint16_t reserv2;
    uint32_t offbits;
}header;

typedef struct BitmapInfo {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitcount;
    uint32_t compression;
    uint32_t imgsize;
    uint32_t pelpermeter1;
    uint32_t pelpermeter2;
    uint32_t clrused;
    uint32_t clrimportant;
}info;

void get_file_info(FILE *file, header *fh, info *fi, int *addbits) {
    fread(fh, sizeof(header), 1, file);

    if ((fh->type[0] != 'B') || (fh->type[1] != 'M')) {
        printf("Not BMP file\n");
        exit(-1);
    }
    
    fread(fi, sizeof(info), 1, file);

    (*addbits) = (int)(((((fi->bitcount * fi->width) + 31) >> 5) << 2) - ((fi->bitcount*fi->width)>>3));

}

void read_first_gen(char **prev[], int addbits, info fi, FILE *file) {
    struct color {
        char r;
        char g;
        char b;
    };
    struct color *fc;
    fc = (struct color *)malloc(sizeof(struct color));

    for (int i = 1; i <= fi.height; i++) {
        for (int j = 1; j <= fi.width; j++ ) {
            fread(fc, sizeof(struct color), 1, file);
            if (fc->r == 0 & fc->g == 0 & fc->b == 0) {
                (*prev)[i][j] = 0;
            } else {
                (*prev)[i][j] = 1;
            }
        }
        fseek(file, addbits, SEEK_CUR);
    } 
}

void get_files_name(char *originalgen, char **originname, char **filename, char *directory) {
    sprintf((*filename), "%s%s", directory, originalgen);

    int namesize = strlen(originalgen) + strlen(directory)-4;
    (*originname) = (char*)malloc(sizeof(char)*(namesize + 1));
    strncpy((*originname), (*filename), namesize); 
    (*originname)[namesize] = '\0';
}

void get_next_filename(char **filename, int genscount, char *name){
    int namelen = strlen(name);
    int len = strlen(*filename);

    int numlen = 0;
    int svgenscount = genscount;
    while (svgenscount != 0) {
        numlen++;
        svgenscount /=10;
    }
    
    if (len - namelen -4 != 0) {
        char *oldnumber;
        oldnumber = (char*)malloc(sizeof(char) * (len-namelen-3));
        strncpy(oldnumber, (*filename) + namelen, len-namelen-4);
        oldnumber[len-namelen-4] = '\0';
        (*filename) = (char*)realloc((*filename), len + (numlen - strlen(oldnumber)));
    } else {
        (*filename) = (char*)realloc((*filename), len + numlen);
    }
    
    sprintf((*filename), "%s%d%s", name, genscount, ".bmp");
}

void create_next_gen(info fi, char **next[], char **prev, int *changes) {
    for (int i = 1; i <= fi.height; i++) {
        for (int j = 1; j <= fi.width; j++ ) {
            if (prev[i][j] == 0) {
                if (near_count(prev,i,j)==3) {
                    (*next)[i][j] = 1;
                    (*changes)++;
                } else {
                    (*next)[i][j] = 0;
                }
            } else {
                int flag = near_count(prev, i, j);
                if ((flag==3) || (flag == 2)) {
                    (*next)[i][j] = 1;
                } else {
                    (*next)[i][j] = 0;
                    (*changes)++;
                }
            }
        }
    }
}

void print_gen(header fh, info fi, char **next, int addbits, FILE *fout) {
    fwrite(&fh, sizeof(header), 1, fout);
    fwrite(&fi, sizeof(info), 1, fout);
    
    for (int i = 1; i <= fi.height; i++) {
        for (int j = 1; j <= fi.width; j++ ) {
            if (next[i][j] == 0) {
                fputc(0, fout);
                fputc(0, fout);
                fputc(0, fout);
            } else {
                fputc(255, fout);
                fputc(0, fout);
                fputc(0, fout);
            }
        }
        for (int j = 0; j < addbits; j++) {
            fputc(0, fout);
        }
    }

}

int near_count(char *mat[],int i,int j) {
    int count = 0;
    if (mat[i - 1][j] == 1) {
        count++;
    }
    if (mat[i + 1][j] == 1) {
        count++;
    }
    if (mat[i][j-1] == 1) {
        count++;
    }
    if (mat[i][j+1] == 1) {
        count++;
    }
    if (mat[i - 1][j+1] == 1) {
        count++;
    }
    if (mat[i - 1][j-1] == 1) {
        count++;
    }
    if (mat[i +1][j + 1] == 1) {
        count++;
    }
    if (mat[i +1][j - 1] == 1) {
        count++;
    }
    return count;
}

int main(int argc, char *argv[]) {
    
    char *options[OPTCNT]; 
    options[INPUT] = "--input";  
    options[OUTPUT] = "--output";
    options[ITER] = "--max_iter";
    options[FREQ] = "--dump_freq";

    int maxiter, dfreq;
    char *originalgen, *directory;

    if (get_options(argc, argv, options, &originalgen, &directory, &maxiter, &dfreq) == 1) {
        printf("Error");
        return 0;
    }
    FILE *fin, *fout;
    fin = fopen(originalgen, "rb");

    header fh;
    info fi;
    int addbits;
    get_file_info(fin, &fh, &fi, &addbits);

    char **prev, **next;
    prev = (char**)malloc(sizeof(char*)*(fi.height + 2));
    next = (char**)malloc(sizeof(char*)*(fi.height + 2));
    for (int i = 0; i<fi.height + 2; i++) {
        prev[i] = (char *)malloc(sizeof(char)*(fi.width+2));
        memset(prev[i], 0, sizeof(char)*(fi.width+2));
        next[i] = (char *)malloc(sizeof(char)*(fi.width+2));
        memset(next[i], 0, sizeof(char)*(fi.width+2));
    }

    read_first_gen(&prev, addbits, fi, fin);

    int genscount = 1;
    int changes;

    char *originname;
    char *filename = (char*)malloc(strlen(originalgen));
    get_files_name(originalgen, &originname, &filename, directory);

    while ((genscount <= maxiter) || (maxiter == 0)) {
        changes = 0;
        create_next_gen(fi, &next, prev, &changes);

        if (changes == 0) {
            printf("GAME OVER\n");
            break;
        }

        if (genscount%dfreq == 0) {
            get_next_filename(&filename, genscount, originname);
            fout = fopen(filename, "wb");
            print_gen(fh, fi, next, addbits, fout);
            fclose(fout);
        }

        char **tmp = next;
        next = prev;
        prev = tmp;

        for (int i = 0; i < fi.height + 2; i++){
            memset(next[i], 0, fi.width + 2);
        }

        genscount++;
    }
    free(prev);
    free(next);
    fclose(fin);
    return 0;

}