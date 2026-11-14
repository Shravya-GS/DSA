#ifndef STUDENT_H
#define STUDENT_H

#include <stdio.h>

#define NAME_LEN 50
#define DEPT_LEN 30

typedef struct Student {
    int   id;
    char  name[NAME_LEN];
    char  dept[DEPT_LEN];
    float cgpa;
    int   semester;
    struct Student *next;
} Student;

typedef struct {
    Student **buckets;
    int capacity;
    int count;
} StudentTable;

typedef enum { SORT_BY_NAME, SORT_BY_DEPT, SORT_BY_CGPA } SortKey;

typedef enum {
    ST_OK = 0,
    ST_DUPLICATE_ID,
    ST_NOT_FOUND,
    ST_NO_MEMORY,
    ST_FILE_ERROR
} StStatus;

int  st_init(StudentTable *t);
void st_free(StudentTable *t);

StStatus st_register(StudentTable *t, int id, const char *name,
                     const char *dept, float cgpa, int semester);
Student *st_find(const StudentTable *t, int id);
StStatus st_update(StudentTable *t, int id, const char *name,
                   const char *dept, float cgpa, int semester);
StStatus st_remove(StudentTable *t, int id);

void st_print_one(const Student *s);
void st_print_all(const StudentTable *t, SortKey key);

StStatus st_save(const StudentTable *t, const char *path);
StStatus st_load(StudentTable *t, const char *path);

#endif
