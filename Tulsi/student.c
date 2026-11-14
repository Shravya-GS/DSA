#include <stdlib.h>
#include <string.h>
#include "student.h"

#define INITIAL_CAPACITY 16
#define MAX_LOAD_FACTOR  0.75

static unsigned int hash_id(int id, int capacity)
{
    return ((unsigned int)id * 2654435761u) % (unsigned int)capacity;
}

int st_init(StudentTable *t)
{
    t->capacity = INITIAL_CAPACITY;
    t->count = 0;
    t->buckets = calloc(t->capacity, sizeof(Student *));
    return t->buckets != NULL;
}

void st_free(StudentTable *t)
{
    for (int i = 0; i < t->capacity; i++) {
        Student *cur = t->buckets[i];
        while (cur) {
            Student *nxt = cur->next;
            free(cur);
            cur = nxt;
        }
    }
    free(t->buckets);
    t->buckets = NULL;
    t->capacity = t->count = 0;
}

static int st_resize(StudentTable *t)
{
    int new_cap = t->capacity * 2;
    Student **nb = calloc(new_cap, sizeof(Student *));
    if (!nb) return 0;

    for (int i = 0; i < t->capacity; i++) {
        Student *cur = t->buckets[i];
        while (cur) {
            Student *nxt = cur->next;
            unsigned int h = hash_id(cur->id, new_cap);
            cur->next = nb[h];
            nb[h] = cur;
            cur = nxt;
        }
    }
    free(t->buckets);
    t->buckets = nb;
    t->capacity = new_cap;
    return 1;
}

Student *st_find(const StudentTable *t, int id)
{
    Student *cur = t->buckets[hash_id(id, t->capacity)];
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

StStatus st_register(StudentTable *t, int id, const char *name,
                     const char *dept, float cgpa, int semester)
{
    if (st_find(t, id)) return ST_DUPLICATE_ID;

    if ((double)(t->count + 1) / t->capacity > MAX_LOAD_FACTOR)
        st_resize(t);

    Student *s = malloc(sizeof(Student));
    if (!s) return ST_NO_MEMORY;

    s->id = id;
    strncpy(s->name, name, NAME_LEN - 1);  s->name[NAME_LEN - 1] = '\0';
    strncpy(s->dept, dept, DEPT_LEN - 1);  s->dept[DEPT_LEN - 1] = '\0';
    s->cgpa = cgpa;
    s->semester = semester;

    unsigned int h = hash_id(id, t->capacity);
    s->next = t->buckets[h];
    t->buckets[h] = s;
    t->count++;
    return ST_OK;
}

StStatus st_update(StudentTable *t, int id, const char *name,
                   const char *dept, float cgpa, int semester)
{
    Student *s = st_find(t, id);
    if (!s) return ST_NOT_FOUND;

    strncpy(s->name, name, NAME_LEN - 1);  s->name[NAME_LEN - 1] = '\0';
    strncpy(s->dept, dept, DEPT_LEN - 1);  s->dept[DEPT_LEN - 1] = '\0';
    s->cgpa = cgpa;
    s->semester = semester;
    return ST_OK;
}

StStatus st_remove(StudentTable *t, int id)
{
    unsigned int h = hash_id(id, t->capacity);
    Student *prev = NULL, *cur = t->buckets[h];

    while (cur) {
        if (cur->id == id) {
            if (prev) prev->next = cur->next;
            else      t->buckets[h] = cur->next;
            free(cur);
            t->count--;
            return ST_OK;
        }
        prev = cur;
        cur = cur->next;
    }
    return ST_NOT_FOUND;
}

static int cmp_name(const Student *a, const Student *b)
{
    int r = strcmp(a->name, b->name);
    return r != 0 ? r : (a->id - b->id);
}

static int cmp_dept(const Student *a, const Student *b)
{
    int r = strcmp(a->dept, b->dept);
    if (r != 0) return r;
    r = strcmp(a->name, b->name);
    return r != 0 ? r : (a->id - b->id);
}

static int cmp_cgpa(const Student *a, const Student *b)
{
    if (a->cgpa > b->cgpa) return -1;
    if (a->cgpa < b->cgpa) return  1;
    return cmp_name(a, b);
}

static void merge(Student **arr, Student **tmp, int lo, int mid, int hi,
                  int (*cmp)(const Student *, const Student *))
{
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi)
        tmp[k++] = (cmp(arr[i], arr[j]) <= 0) ? arr[i++] : arr[j++];
    while (i <= mid) tmp[k++] = arr[i++];
    while (j <= hi)  tmp[k++] = arr[j++];
    for (k = lo; k <= hi; k++) arr[k] = tmp[k];
}

static void merge_sort(Student **arr, Student **tmp, int lo, int hi,
                       int (*cmp)(const Student *, const Student *))
{
    if (lo >= hi) return;
    int mid = lo + (hi - lo) / 2;
    merge_sort(arr, tmp, lo, mid, cmp);
    merge_sort(arr, tmp, mid + 1, hi, cmp);
    merge(arr, tmp, lo, mid, hi, cmp);
}

void st_print_one(const Student *s)
{
    printf("%-8d %-24s %-18s %5.2f   %d\n",
           s->id, s->name, s->dept, s->cgpa, s->semester);
}

void st_print_all(const StudentTable *t, SortKey key)
{
    if (t->count == 0) {
        printf("No students registered.\n");
        return;
    }

    Student **arr = malloc(t->count * sizeof(Student *));
    Student **tmp = malloc(t->count * sizeof(Student *));
    if (!arr || !tmp) {
        printf("Out of memory.\n");
        free(arr); free(tmp);
        return;
    }

    int n = 0;
    for (int i = 0; i < t->capacity; i++)
        for (Student *c = t->buckets[i]; c; c = c->next)
            arr[n++] = c;

    int (*cmp)(const Student *, const Student *) =
        (key == SORT_BY_NAME) ? cmp_name :
        (key == SORT_BY_DEPT) ? cmp_dept : cmp_cgpa;
    merge_sort(arr, tmp, 0, n - 1, cmp);

    printf("%-8s %-24s %-18s %-6s %s\n",
           "ID", "Name", "Department", "CGPA", "Sem");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) st_print_one(arr[i]);
    printf("Total: %d student(s)\n", n);

    free(arr);
    free(tmp);
}

StStatus st_save(const StudentTable *t, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return ST_FILE_ERROR;
    for (int i = 0; i < t->capacity; i++)
        for (Student *c = t->buckets[i]; c; c = c->next)
            fprintf(f, "%d|%s|%s|%.2f|%d\n",
                    c->id, c->name, c->dept, c->cgpa, c->semester);
    fclose(f);
    return ST_OK;
}

StStatus st_load(StudentTable *t, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return ST_FILE_ERROR;

    char line[256], name[NAME_LEN], dept[DEPT_LEN];
    int id, sem;
    float cgpa;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%d|%49[^|]|%29[^|]|%f|%d",
                   &id, name, dept, &cgpa, &sem) == 5)
            st_register(t, id, name, dept, cgpa, sem);
    }
    fclose(f);
    return ST_OK;
}
