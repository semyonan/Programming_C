#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Tree{
    struct Tree *left;
    struct Tree *right;
    unsigned char byte;
    int frequency;
}Tree_t;

typedef struct Priority_queue{
    Tree_t **arr;
    int size; 
    int len;
}Queue_t;

typedef struct New_byte_code {
    unsigned char bitsize;
    unsigned char bytesize;
    unsigned char *bytes;
}Code_t;

void tree_siftup(Queue_t* pqueue, int i) {
    Tree_t* tmp;
    int j = (i-1)/2;
    while (i > 0 && pqueue->arr[j]->frequency > pqueue->arr[i]->frequency) {
        tmp = pqueue->arr[j];
        pqueue->arr[j] = pqueue->arr[i];
        pqueue->arr[i] = tmp;
        i = j;
        j = (i-1)/2;
    }
}

void queue_insert(Queue_t* pqueue, Tree_t* tree) {
    if(pqueue->len == pqueue->size) {
        pqueue->size *= 2;
        pqueue->arr = (Tree_t**)realloc(pqueue->arr,pqueue->size*sizeof(Tree_t*));
        if (!pqueue->arr) {
            pqueue->size = pqueue->len = 0;
            return;
        }
    }
    pqueue->arr[pqueue->len] = tree;
    pqueue->len += 1;
    tree_siftup(pqueue, pqueue->len-1);
}

void tree_siftdown(Queue_t* pqueue, int i) {
    Tree_t* tmp;
    int root;
    while (2*i+1 < pqueue->len) {
        root = i;
        if ((pqueue->arr[2*i+1]->frequency < pqueue->arr[i]->frequency) && ((2*i+2 >= pqueue->len) || (pqueue->arr[2*i+1]->frequency <= pqueue->arr[2*i+2]->frequency))){
            root = 2*i+1;
        }
        else if ((2*i+2 < pqueue->len) && (pqueue->arr[2*i+2]->frequency < pqueue->arr[i]->frequency)) {
            root = 2*i+2;
        }
        if (root == i) {
            break;
        } 
        tmp = pqueue->arr[i];
        pqueue->arr[i] = pqueue->arr[root];
        pqueue->arr[root] = tmp;
        i = root;
    }
}

Tree_t* queue_ex_min(Queue_t* pqueue) {
    Tree_t* deltree;
    if (pqueue->len > 0) {
        deltree = pqueue->arr[0];
        pqueue->arr[0] = pqueue->arr[pqueue->len-1];
        pqueue->len -= 1;
        tree_siftdown(pqueue, 0);
    }
    return deltree;
}

int increase_key(Queue_t *pqueue, unsigned char c) {
    for (int i = 0; i < pqueue->len; i++) {
        if (pqueue->arr[i]->byte == c) {
            pqueue->arr[i]->frequency++;
            tree_siftdown(pqueue, i);
            return 1;
        }
    }
    return 0;
}

Tree_t* tree_merge(Tree_t *x, Tree_t *y) {
    Tree_t* newtree = (Tree_t*)malloc(sizeof(Tree_t));
    newtree->byte = 1;
    newtree->frequency = x->frequency + y->frequency;
    newtree->left = x;
    newtree->right = y;
    return newtree;
}

void get_new_codes(Code_t **Hoff_code, Tree_t *ptree, unsigned char *bits, unsigned char bitsize, unsigned char bytesize) {
    if(ptree != NULL) {
        if ((ptree->left == NULL) && (ptree->right == NULL)) {
            (*Hoff_code)[ptree->byte].bitsize = bitsize;
            (*Hoff_code)[ptree->byte].bytesize = bytesize;
            (*Hoff_code)[ptree->byte].bytes = (unsigned char *)malloc(sizeof(unsigned char) * (bytesize));
            memcpy((*Hoff_code)[ptree->byte].bytes, bits, bytesize);
            return;
        } 
        
        if (((bitsize-1)%8 == 0) && (bitsize != 1)) {
            bits = (unsigned char*)realloc(bits, sizeof(unsigned char) *(bytesize + 1));
            bits[bytesize] = 0;
            bytesize++;
        }
        unsigned char *rightbits = (unsigned char*)malloc(sizeof(unsigned char)*bytesize);
        memcpy(rightbits, bits, bytesize);
        get_new_codes(Hoff_code, ptree->left, bits ,bitsize + 1, bytesize);
        if (bitsize == 0) {
            rightbits[bytesize - 1] = rightbits[bytesize - 1] | (1<<7);
        } else {
            rightbits[bytesize - 1] = rightbits[bytesize - 1] | (1<<(7 - bitsize%8));
        }
        get_new_codes(Hoff_code, ptree->right, rightbits, bitsize + 1, bytesize);
    }
}
/*void print_tree(Tree_t *ptree) {
    if (ptree != NULL) {
        printf("%c\n", ptree->byte);
        print_tree(ptree->left);
        print_tree(ptree->right);
    }
}*/
