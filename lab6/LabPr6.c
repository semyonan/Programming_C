#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "Hoffman.h"

#define OPTCNT 4
#define ARCFILE 0
#define CREATE 1
#define EXTRACT 2
#define LIST 3

void tree_siftup(Queue_t* pqueue, int i);

void tree_insert(Queue_t* pqueue, Tree_t* tree);

void tree_siftdown(Queue_t* pqueue, int i);

Tree_t* tree_ex_min(Queue_t* pqueue);

int decrease_key(Queue_t *pqueue, unsigned char c);

Tree_t* tree_merge(Tree_t *x, Tree_t *y); 

void get_new_codes(Code_t **haff_code, Tree_t *ptree, unsigned char *bits, unsigned char bitsize, unsigned char bytesize);

int get_options(int argc, char *argv[], char *options[], int found[], char **file_arc_name, char **files_name[], int *files_name_len, int files_name_size) {
    if (argc < 4) {
        return 1; 
    }
    
    int flag;

    for (int i = 1; i < argc; i++) {
        flag = 0;
        for (int j = 0; j < OPTCNT; ++j) {
            if (strcmp(options[j], argv[i]) == 0) {
                found[j] = 1;
                flag = 1;
                if ((j == ARCFILE) && (i+1 < argc)) {
                    (*file_arc_name) = argv[i+1];
                    i++;
                } else if (j == ARCFILE) {
                    found[j] = 0;
                }
                break;
            }
        } 
        if (flag == 0) {
            if ((*files_name_len) + 1 == files_name_size){
                files_name_size *= 2;
                (*files_name) = (char**)realloc((*files_name), files_name_size);
            }
            (*files_name)[(*files_name_len)] = argv[i];
            (*files_name_len) += 1;
        }
    }

    if (found[ARCFILE] != 0) {
        return 0;
    } else {
        return 1;
    }

    return 0;
}

int get_tag(FILE* arc) {
    char *tag = (char*)malloc(sizeof(char)*4);
    fread(tag, sizeof(char), 3, arc);
    tag[3] = '\0';
    if (strcmp(tag, "ARC") != 0) {
        free(tag);
        return 0;
    } else {
        free(tag);
        return 1;
    }
}

void write_arc_info(Queue_t queue, char *filename, FILE *arc) {
    int len = strlen(filename);
    fwrite(&len, sizeof(uint32_t), 1, arc);
    for (int i = 0; i < len; i++) {
        fputc(filename[i], arc);
    }

    queue.len *= 2;
    fwrite(&queue.len, sizeof(uint32_t), 1, arc);
    queue.len /= 2;
    for (int i = 0; i < queue.len; i++) {
        fwrite(&queue.arr[i]->frequency, sizeof(char), 1, arc);
        fwrite(&queue.arr[i]->byte, sizeof(char), 1, arc);
    }
}

Tree_t* create_tree(char *filename, FILE *file, FILE *arc) {
    Queue_t queue;
    queue.arr = (Tree_t**)malloc(256*sizeof(Tree_t*));
    queue.size = 256;
    queue.len = 0;
    char c;

    while((c = fgetc(file)) != EOF) {
        if (!increase_key(&queue, c)) {
            Tree_t* tree = (Tree_t*)malloc(sizeof(Tree_t));
            tree->byte = c;
            tree->frequency = 1;
            tree->left = NULL;
            tree->right = NULL;
            queue_insert(&queue, tree);
        }
    }

    write_arc_info(queue, filename, arc);

    while (queue.len > 1) {
        Tree_t *lefttree = queue_ex_min(&queue);
        Tree_t *righttree = queue_ex_min(&queue);
        Tree_t *tree = tree_merge(lefttree, righttree);
        queue_insert(&queue, tree);
    }
    
    Tree_t* tree = queue.arr[0];

    free(queue.arr);
    
    return tree;
}

Tree_t* get_tree(FILE *arc) {
    Queue_t queue;
    fread(&queue.size, sizeof(uint32_t), 1, arc);
    queue.size /=2;
    queue.arr = (Tree_t**)malloc(queue.size*sizeof(Tree_t*));
    queue.len = 0;

    for (int i = 0; i < queue.size; i++) {
        Tree_t* tree = (Tree_t*)malloc(sizeof(Tree_t));
        fread(&tree->frequency, sizeof(char), 1, arc);
        fread(&tree->byte, sizeof(char), 1, arc);
        tree->left = NULL;
        tree->right = NULL;
        queue_insert(&queue, tree);
    }

    while (queue.len > 1) {
        Tree_t *lefttree = queue_ex_min(&queue);
        Tree_t *righttree = queue_ex_min(&queue);
        Tree_t *tree = tree_merge(lefttree, righttree);
        queue_insert(&queue, tree);
    }

    Tree_t* tree = queue.arr[0];

    free(queue.arr);

    return tree;
}

void write_arc_file(Code_t *haff_code, FILE *fin, FILE *arc_file) {
    int size_pos = ftell(arc_file);

    for (int i = 0; i < 8; i++) {
        fputc('0', arc_file);
    }

    int new_file_size = 0, size = 0;

    fseek(fin, 0L, SEEK_SET);
    unsigned char byte = 0;
    char cursize = 0;
    char c;
    while ((c = fgetc(fin)) != EOF) {
        size++;
        for (int i = 0; i < haff_code[c].bytesize; i++) {
            byte = byte | (haff_code[c].bytes[i]>>cursize);

            if( i == haff_code[c].bytesize - 1) {
                if (cursize + haff_code[c].bitsize%8 < 8) {
                    cursize += haff_code[c].bitsize%8;
                } else {
                    fputc(byte, arc_file);
                    new_file_size++;
                    byte = haff_code[c].bytes[i]<<(8 - cursize);
                    cursize = (haff_code[c].bitsize%8 + cursize) % 8;
                }
            } else {
                fputc(byte, arc_file);
                new_file_size++;
                byte = haff_code[c].bytes[i]<<(8 - cursize);
            }
        }
    }
    
    if (cursize != 0) {
        fputc(byte, arc_file);
        new_file_size++;
    }

    fseek(arc_file, size_pos, SEEK_SET);
    fwrite(&size, sizeof(uint32_t), 1, arc_file);
    fwrite(&new_file_size, sizeof(uint32_t), 1, arc_file);
    fseek(arc_file, new_file_size, SEEK_CUR);
}

void write_rearc_file(Tree_t *haff_tree, int size, int end_of_data, FILE *arc, FILE* file){
    unsigned char byte = fgetc(arc);
    int cursize = 0;
    Tree_t *root = haff_tree;
    for (int i = 0; i < size; i++) {
        while ((haff_tree->left != NULL) || (haff_tree->right != NULL)) {
            if (((byte>>(8-1-cursize))&1) == 1) {
                haff_tree = haff_tree->right;
            } else {
                haff_tree = haff_tree->left;
            }
            cursize++;
            if (cursize == 8) {
                cursize = 0;
                if (ftell(arc) == end_of_data) {
                    return;
                }
                byte = fgetc(arc);
            }
        }
        fputc(haff_tree->byte, file);
        haff_tree = root;
        if (cursize == 8) {
            cursize = 0;
            if (ftell(arc) == end_of_data) {
                return;
            }
            byte = fgetc(arc);
        }
    }
}

int main(int argc, char *argv[]) {
    
    char *options[OPTCNT]; 
    options[ARCFILE] = "--file";  
    options[CREATE] = "--create";
    options[EXTRACT] = "--extract";
    options[LIST] = "--list";

    int found[OPTCNT];
    found[3] = found[2] = found[1] = found[0] = 0;

    int files_num = 0;
    char **filename, *arc_filename;
    filename = (char**)malloc(sizeof(char)*1);

    if (get_options(argc, argv, options, found, &arc_filename, &filename, &files_num, 1) == 1) {
        printf("ERROR");
        return 0;
    }

    if ((found[CREATE] == 1) && (files_num > 0)) {
        
        FILE *arc = fopen(arc_filename, "wb+");

        Code_t *haff_code = (Code_t*)malloc(256 * sizeof(Code_t));
        Tree_t *haff_tree;
        
        fputc('A', arc);
        fputc('R', arc);
        fputc('C', arc);
        fwrite(&files_num, sizeof(uint32_t), 1, arc);

        for (int i = 0; i < files_num; i++) {
            FILE *fin = fopen(filename[i], "rb");
            if (!fin) {
                printf("%s not a file!", filename[i]);
                continue;
            }
            
            haff_tree = create_tree(filename[i], fin, arc);
            unsigned char *bits = (unsigned char*)malloc(sizeof(unsigned char));
            bits[0] = 0;

            get_new_codes(&haff_code, haff_tree, bits, 0, 1);

            write_arc_file(haff_code, fin, arc);

            fclose(fin);
            free(bits);
        }
        
        fclose(arc);
        free(haff_code);
    }

    if (found[EXTRACT] == 1) {
        FILE *arc;
        arc = fopen(arc_filename, "rb");

        char *filename;
        int filename_len;
        
        if (!get_tag(arc)) {
            printf("%s is not an .arc file\n", arc_filename);
            return 0;
        }

        fread(&files_num, sizeof(uint32_t), 1, arc);
        
        for (int i = 0; i < files_num; i++) {
            fread(&filename_len, sizeof(uint32_t), 1, arc);
            filename = (char*)malloc(sizeof(char)*(filename_len+1));
            fread(filename, sizeof(char), filename_len, arc);
            filename[filename_len] = '\0';

            FILE *file = fopen(filename, "wb");

            Tree_t *haff_tree = (Tree_t*)malloc(sizeof(Tree_t));
            haff_tree = get_tree(arc);

            int end_of_data, size_of_data;
            fread(&size_of_data, sizeof(uint32_t), 1, arc);
            fread(&end_of_data, sizeof(uint32_t), 1, arc);
            int begin_of_data = ftell(arc);
            end_of_data += begin_of_data;

            write_rearc_file(haff_tree, size_of_data, end_of_data, arc, file);

            fclose(file);
            free(haff_tree);
        }
        fclose(arc);
        free(filename);
    }

    if (found[LIST] == 1) {
        FILE *arc;
        arc = fopen(arc_filename, "rb");
        
        char *filename;
        int filename_len;

        if (!get_tag(arc)) {
            printf("%s is not an .arc file\n", arc_filename);
            return 0;
        }

        fread(&files_num, sizeof(uint32_t), 1, arc);

        for (int i = 0; i < files_num; i++) {
            fread(&filename_len, sizeof(uint32_t), 1, arc);
            filename = (char*)malloc(sizeof(char)*(filename_len+1));
            fread(filename, sizeof(char), filename_len, arc);
            filename[filename_len] = '\0';
            printf("%s\n", filename);

            int size;
            fread(&size, sizeof(uint32_t), 1, arc);
            fseek(arc, size+4, SEEK_CUR);
            fread(&size, sizeof(uint32_t), 1, arc);
            fseek(arc, size, SEEK_CUR);
        }
        fclose(arc);
        free(filename);
    }

    free(filename);

    return 0;
}