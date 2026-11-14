#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "student.h"

#define DATA_FILE "students.txt"

static int read_line(const char *prompt, char *buf, size_t size)
{
    printf("%s", prompt);
    if (!fgets(buf, (int)size, stdin)) return 0;
    buf[strcspn(buf, "\n")] = '\0';
    for (char *p = buf; *p; p++)
        if (*p == '|') *p = ' ';
    return 1;
}

static int read_int(const char *prompt, int lo, int hi)
{
    char buf[64], *end;
    for (;;) {
        if (!read_line(prompt, buf, sizeof buf)) return -1;
        long v = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && v >= lo && v <= hi) return (int)v;
        printf("  Please enter a whole number from %d to %d.\n", lo, hi);
    }
}

static float read_float(const char *prompt, float lo, float hi)
{
    char buf[64], *end;
    for (;;) {
        if (!read_line(prompt, buf, sizeof buf)) return -1.0f;
        float v = strtof(buf, &end);
        if (end != buf && *end == '\0' && v >= lo && v <= hi) return v;
        printf("  Please enter a number from %.1f to %.1f.\n", lo, hi);
    }
}

static void read_required(const char *prompt, char *buf, size_t size)
{
    do {
        if (!read_line(prompt, buf, size)) { buf[0] = '\0'; return; }
        if (buf[0] == '\0') printf("  This field cannot be empty.\n");
    } while (buf[0] == '\0');
}

static void do_register(StudentTable *t)
{
    char name[NAME_LEN], dept[DEPT_LEN];
    int id = read_int("Student ID (positive number): ", 1, 2000000000);
    if (id < 0) return;
    if (st_find(t, id)) {
        printf("A student with ID %d already exists.\n", id);
        return;
    }
    read_required("Name: ", name, sizeof name);
    read_required("Department (e.g. CSE): ", dept, sizeof dept);
    float cgpa = read_float("CGPA (0.0 - 10.0): ", 0.0f, 10.0f);
    int sem = read_int("Semester (1-8): ", 1, 8);

    StStatus r = st_register(t, id, name, dept, cgpa, sem);
    printf(r == ST_OK ? "Student registered.\n" : "Registration failed.\n");
}

static void do_update(StudentTable *t)
{
    int id = read_int("ID of student to update: ", 1, 2000000000);
    if (id < 0) return;
    Student *s = st_find(t, id);
    if (!s) { printf("No student with ID %d.\n", id); return; }

    printf("Current record:\n");
    st_print_one(s);
    printf("Press Enter to keep the current value.\n");

    char name[NAME_LEN], dept[DEPT_LEN], buf[64];
    strcpy(name, s->name);
    strcpy(dept, s->dept);
    float cgpa = s->cgpa;
    int sem = s->semester;

    char in[NAME_LEN];
    read_line("New name: ", in, sizeof in);
    if (in[0]) strcpy(name, in);

    char din[DEPT_LEN];
    read_line("New department: ", din, sizeof din);
    if (din[0]) strcpy(dept, din);

    read_line("New CGPA (0-10): ", buf, sizeof buf);
    if (buf[0]) {
        char *end; float v = strtof(buf, &end);
        if (*end == '\0' && v >= 0.0f && v <= 10.0f) cgpa = v;
        else printf("  Invalid CGPA, keeping old value.\n");
    }
    read_line("New semester (1-8): ", buf, sizeof buf);
    if (buf[0]) {
        char *end; long v = strtol(buf, &end, 10);
        if (*end == '\0' && v >= 1 && v <= 8) sem = (int)v;
        else printf("  Invalid semester, keeping old value.\n");
    }

    st_update(t, id, name, dept, cgpa, sem);
    printf("Student updated.\n");
}

static void do_remove(StudentTable *t)
{
    int id = read_int("ID of student to remove: ", 1, 2000000000);
    if (id < 0) return;
    printf(st_remove(t, id) == ST_OK ? "Student removed.\n"
                                     : "No student with that ID.\n");
}

static void do_search(const StudentTable *t)
{
    int id = read_int("Student ID to search: ", 1, 2000000000);
    if (id < 0) return;
    Student *s = st_find(t, id);
    if (!s) { printf("No student with ID %d.\n", id); return; }
    printf("%-8s %-24s %-18s %-6s %s\n", "ID", "Name", "Department", "CGPA", "Sem");
    st_print_one(s);
}

static void do_display(const StudentTable *t)
{
    printf("\nDisplay order:\n 1. By name\n 2. By department\n"
           " 3. By CGPA (highest first)\n");
    int c = read_int("Choice: ", 1, 3);
    if (c < 0) return;
    SortKey k = (c == 1) ? SORT_BY_NAME : (c == 2) ? SORT_BY_DEPT : SORT_BY_CGPA;
    st_print_all(t, k);
}

int main(void)
{
    StudentTable table;
    if (!st_init(&table)) { fprintf(stderr, "Out of memory.\n"); return 1; }

    if (st_load(&table, DATA_FILE) == ST_OK)
        printf("Loaded %d student(s) from %s.\n", table.count, DATA_FILE);

    int choice;
    do {
        printf("\n===== CampusAssist: Student Management =====\n"
               " 1. Register student\n"
               " 2. Update student\n"
               " 3. Remove student\n"
               " 4. Search by ID\n"
               " 5. Display all students (sorted)\n"
               " 0. Save and exit\n");
        choice = read_int("Choice: ", 0, 5);

        switch (choice) {
            case 1: do_register(&table); break;
            case 2: do_update(&table);   break;
            case 3: do_remove(&table);   break;
            case 4: do_search(&table);   break;
            case 5: do_display(&table);  break;
            default: break;
        }
    } while (choice > 0);

    if (st_save(&table, DATA_FILE) == ST_OK)
        printf("Saved %d student(s) to %s.\n", table.count, DATA_FILE);
    else
        printf("Warning: could not save data.\n");

    st_free(&table);
    return 0;
}
