#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define OPTCNT 3
#define LINES 0
#define BYTES 1
#define WORDS 2

int get_opts(int argc, char *argv[], char *options[], int opt_cnt, int found[], char **filename) {
    if (argc < 2) {
        return 1; 
    }
    
    for (int i = 1; i < argc - 1; ++i) {
        if (argv[i] == strstr(argv[i], "--")) {
            int fnd = 0;
            for (int j = 0; j < opt_cnt; ++j) {
                if (strcmp(options[j], argv[i]+2) == 0) {
                    found[j] = 1;
                    fnd = 1;
                    break;
                }
            }
            if (!fnd) {
                return 2; 
            }
        }
        else if (*(argv[i]) == '-') {
            int len = strlen(argv[i]);
            if (len == 1) {
                return 3; 
            }
            for (int j = 1; j < len; j++) { 
                int fnd = 0;
                for (int k = 0; k < opt_cnt; ++k) {
                    if (*(options[k]) == *(argv[i]+j)) { 
                        found[k] = 1;
                        fnd = 1;
                        break;
                    } 
                }
                if (!fnd) {
                    return 3; 
                }
            }
        }
        else {
            return 4;
        }
    }

    *filename = argv[argc-1];

    return 0;
}

int main(int argc, char *argv[]) 
{
    char *options[OPTCNT]; 
    options[LINES] = "lines";  
    options[BYTES] = "bytes";
    options[WORDS] = "words";

    int found[OPTCNT];
    found[2] = found[1] = found[0] = 0;

    char *filename;

    if (get_opts(argc, argv, options, OPTCNT, found, &filename) != 0) {
        printf("%s\n", "Usage: test [OPTIONS] filename");
        return 10;
    }

    else {

        FILE *file= fopen(filename,"rb");

        if (!file) {
            printf("%s\n", "Cannot open file.");
            return 10;
        }

        int count_lines, count_bytes, count_words;
        count_lines = found[LINES];
        count_bytes = found[BYTES];
        count_words = found[WORDS];

        char c;
        int nl, nw, b, state, nstr;
        nl = nw = b = state = nstr = 0;

        while ((c = fgetc(file)) != EOF){

            if (count_lines) {
                if (nstr == 0) {
                    nstr = 1;
                    ++nl;
                }
                if (c == '\n') {
                    nstr = 0;
                }
                
            }

            if (count_bytes) {
                ++b;
            }

            if (count_words || !count_lines && !count_bytes && !count_words) {
                if (isspace(c)){
                    state = 0;
                }
                else if (state == 0) {
                    state = 1;
                    ++nw;
                }
            }
        }

        if (count_lines){
            printf ("%d\n", nl);
        }
        
        if (count_bytes){
            printf ("%d\n", b);
        }

        if (count_words || !count_lines && !count_bytes && !count_words){
            printf ("%d\n", nw);
        }

        fclose(file);

        return 0;    
    }

}
