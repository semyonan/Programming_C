#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

static char *MonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

typedef struct list{
    int time;
    struct list *next;
}list;

typedef struct queue {
    list *head, *tail;
    int size;
    int interval;
}queue;

void init(queue *qoft) {
    qoft->head = NULL;
    qoft->tail = NULL;
    qoft->size = 0;
    qoft->interval = 0;
}

void delete(queue *qoft) {
    list* prev = NULL;
    prev = qoft->head;
    qoft->head = qoft->head->next;
    qoft->size -= 1;
    free(prev);
}

int add(queue *qoft, int time, int interval) {
    list *tmp = (list*)malloc(sizeof(list));
    tmp->time = time;
    tmp->next = NULL;
    if (qoft->tail != NULL) {
        qoft->tail->next = tmp;
    }
    qoft->tail = tmp;
    qoft->size +=1;
    if (qoft->head == NULL) {
        qoft->head = tmp;
    }

    qoft->interval = qoft->tail->time - qoft->head->time;
    while (qoft->interval > interval) {
        delete(qoft);
        qoft->interval = qoft->tail->time - qoft->head->time;
    }

    return qoft->size;
}

void emptyQ(queue *qoft) {
    while (qoft->size > 0) {
        delete(qoft);
    }
}

int getmonth(char *strmonth) {
    for (int i = 0; i < 12; i++) {
        if (strcmp(strmonth, MonthNames[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int gettime(char *timestr) {
    struct tm time;
    char month[4];
    int year;

    memset(&time, 0, sizeof(struct tm));

    sscanf(timestr,"%2d/%3s/%4d:%2d:%2d:%2d", &time.tm_mday, month, &year, &time.tm_hour, &time.tm_min, &time.tm_sec);
    time.tm_mon = getmonth(month);
    time.tm_year = year - 1900;
    time.tm_isdst = -1;

    time_t rawtime = mktime(&time);

    return (int)rawtime;
}

char *sprinttime(char *buf, int time) {
    time_t rawtime = time;
    struct tm *ptm = localtime(&rawtime);

    sprintf(buf, "%02d/%3s/%04d:%02d:%02d:%02d", ptm->tm_mday, MonthNames[ptm->tm_mon], ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    return buf;
}

char *getline(char *buf, int *pbuf_size, FILE *file) {

    int buf_size = *pbuf_size;
    int shift = 0;

    buf[buf_size-2] = '\0';

    while(!feof(file)) {
        if (fgets(buf + shift, buf_size - shift, file)) {
            char ch = *(buf+buf_size-2);
            if ((ch != '\0') && (ch != '\n')) {
                shift = buf_size;
                buf_size *= 2;
                buf = (char*)realloc(buf, buf_size);
                buf[buf_size-2] = '\0';
                memset(buf+shift, 0, shift);
                shift -=1;
                continue;
            }
            *pbuf_size = buf_size;
            return buf;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    
    if (argc < 3) {
        printf ("Usage:LabPr3 <interval> <filename>\n");
        return 1;
    }

    char *filename = argv[argc - 1];
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf ("Usage:LabPr3 <interval> <filename>\n");
        return 2;
    }

    int max, cursize;
    int interval = atoi(argv[argc - 2]);
    int boundleft, boundright;
    char timestr[21];
    char buftimel[21];
    char buftimer[21];
    int buf_size = 1024;
    int counter = 0;
    max = 0;
    cursize = 0;
    
    char *buf = (char*)malloc(buf_size);

    queue *qoft;
    qoft = (queue*)malloc(sizeof(queue));
    init(qoft);

    while(buf = getline(buf, &buf_size, file)) {
        if (strstr(buf, "\" 5")) {
            printf("%s", buf);
            counter++;
        }
        if (strchr(buf,'[') != NULL) {
            strncpy(timestr, strchr(buf,'[') + 1, 20);
            timestr[20] = '\0';
            cursize = add(qoft, gettime(timestr), interval);
            if (cursize > max) {
                max = cursize;
                boundleft = qoft->head->time;
                boundright = qoft->tail->time;
            }
        }
    }
    emptyQ(qoft);

    printf("Number of rquests with errors: %d\n", counter);
    printf("The higher number of requests in %d seconds: %d\nFrom %s to %s\n", interval, max, sprinttime(buftimel, boundleft), sprinttime(buftimer,boundright));
    free(qoft);
    free(buf);
    fclose(file);

    return 0;
}