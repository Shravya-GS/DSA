#include "complaints.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_demo_student(int student_id, void *context)
{
    const int *ids = (const int *)context;
    size_t i;

    for (i = 0; ids[i] != 0; ++i) {
        if (ids[i] == student_id) {
            return 1;
        }
    }
    return 0;
}

static int parse_student_id(const char *text, int *student_id)
{
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (text[0] == '\0' || *end != '\0' || errno == ERANGE ||
        parsed <= 0 || parsed > INT_MAX) {
        return 0;
    }
    *student_id = (int)parsed;
    return 1;
}

int main(int argc, char **argv)
{
    const int demo_students[] = {1001, 1002, 1003, 0};
    ComplaintStore store;

    if (argc < 2 || argc > 3 ||
        (strcmp(argv[1], "student") != 0 && strcmp(argv[1], "admin") != 0) ||
        (strcmp(argv[1], "student") == 0 && argc != 3) ||
        (strcmp(argv[1], "admin") == 0 && argc != 2)) {
        fprintf(stderr, "Usage: %s student <1001|1002|1003> | admin\n", argv[0]);
        return 2;
    }
    if (!complaint_store_open(&store, "complaints.dat")) {
        fputs("Could not load complaints.dat; check the file format and permissions.\n",
              stderr);
        return 1;
    }

    if (strcmp(argv[1], "student") == 0) {
        int student_id;
        if (!parse_student_id(argv[2], &student_id)) {
            fputs("Student ID must be a positive whole number.\n", stderr);
            complaint_store_close(&store);
            return 2;
        }
        puts("Demo student IDs: 1001, 1002, 1003");
        complaint_student_menu(
            &store,
            student_id,
            is_demo_student,
            (void *)demo_students);
    } else {
        puts("Demo only: administrator authentication belongs to the host application.");
        complaint_admin_menu(&store);
    }
    complaint_store_close(&store);
    return 0;
}
