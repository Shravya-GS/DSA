#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ID_SIZE 24
#define NAME_SIZE 80
#define DEPT_SIZE 60
#define EMAIL_SIZE 100
#define DATA_FILE "students.txt"
#define LINE_SIZE 512

typedef struct {
    char id[ID_SIZE];
    char name[NAME_SIZE];
    char department[DEPT_SIZE];
    int year;
    float gpa;
    char email[EMAIL_SIZE];
} Student;

typedef struct StudentNode {
    Student student;
    struct StudentNode *next;
} StudentNode;

static StudentNode **table;
static size_t table_size;
static size_t student_count;
static void free_students(void);

static size_t hash_id_for(const char *id, size_t size)
{
    unsigned long hash = 5381;
    while (*id != '\0')
        hash = ((hash << 5) + hash) + (unsigned char)*id++;
    return (size_t)(hash % size);
}

static size_t hash_id(const char *id)
{
    return hash_id_for(id, table_size);
}

static int ensure_table_capacity(void)
{
    size_t new_size;
    StudentNode **new_table;

    if (table_size == 0) {
        new_table = (StudentNode **)calloc(101, sizeof *new_table);
        if (new_table == NULL) {
            perror("Unable to allocate student hash table");
            return 0;
        }
        table = new_table;
        table_size = 101;
        return 1;
    }
    if ((student_count + 1) * 4 <= table_size * 3)
        return 1;

    new_size = table_size * 2 + 1;
    new_table = (StudentNode **)calloc(new_size, sizeof *new_table);
    if (new_table == NULL) {
        perror("Unable to grow student hash table");
        return 0;
    }
    for (size_t bucket = 0; bucket < table_size; ++bucket) {
        StudentNode *node = table[bucket];
        while (node != NULL) {
            StudentNode *next = node->next;
            size_t new_bucket = hash_id_for(node->student.id, new_size);
            node->next = new_table[new_bucket];
            new_table[new_bucket] = node;
            node = next;
        }
    }
    free(table);
    table = new_table;
    table_size = new_size;
    return 1;
}

static StudentNode *find_node(const char *id)
{
    if (table_size == 0)
        return NULL;
    StudentNode *node = table[hash_id(id)];
    while (node != NULL) {
        if (strcmp(node->student.id, id) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}

static int read_line(const char *prompt, char *buffer, size_t size)
{
    size_t length;
    int ch;

    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL)
        return 0;
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    length = strlen(buffer);
    while (length > 0 && isspace((unsigned char)buffer[length - 1]))
        buffer[--length] = '\0';
    while (*buffer != '\0' && isspace((unsigned char)*buffer))
        memmove(buffer, buffer + 1, strlen(buffer));
    return 1;
}

static int read_student_details(Student *student, int include_id)
{
    char input[LINE_SIZE];
    char *end;
    long year;
    float gpa;

    if (include_id) {
        if (!read_line("Student ID: ", student->id, sizeof student->id))
            return 0;
        if (student->id[0] == '\0' || strchr(student->id, '|') != NULL) {
            puts("ID cannot be empty or contain '|'.");
            return 0;
        }
        if (find_node(student->id) != NULL) {
            puts("That student ID is already registered.");
            return 0;
        }
    }

    if (!read_line("Full name: ", student->name, sizeof student->name) ||
        !read_line("Department: ", student->department, sizeof student->department) ||
        !read_line("Academic year (1-8): ", input, sizeof input))
        return 0;
    year = strtol(input, &end, 10);
    if (input[0] == '\0' || *end != '\0' || year < 1 || year > 8) {
        puts("Invalid academic year.");
        return 0;
    }
    student->year = (int)year;

    if (!read_line("GPA (0.0-10.0): ", input, sizeof input))
        return 0;
    gpa = strtof(input, &end);
    if (input[0] == '\0' || *end != '\0' ||
        !(gpa >= 0.0f && gpa <= 10.0f)) {
        puts("Invalid GPA.");
        return 0;
    }
    student->gpa = gpa;

    if (!read_line("Email: ", student->email, sizeof student->email))
        return 0;
    if (student->name[0] == '\0' || student->department[0] == '\0' ||
        strchr(student->name, '|') != NULL ||
        strchr(student->department, '|') != NULL ||
        strchr(student->email, '|') != NULL) {
        puts("Name and department are required; fields cannot contain '|'.");
        return 0;
    }
    return 1;
}

static int add_student(const Student *student)
{
    size_t bucket;
    StudentNode *node;
    if (!ensure_table_capacity())
        return 0;
    bucket = hash_id(student->id);
    node = (StudentNode *)malloc(sizeof *node);
    if (node == NULL) {
        perror("Unable to allocate student record");
        return 0;
    }
    node->student = *student;
    node->next = table[bucket];
    table[bucket] = node;
    ++student_count;
    return 1;
}

static void display_student(const Student *student)
{
    printf("%-16s %-24s %-18s %4d %5.2f %-30s\n",
           student->id, student->name, student->department,
           student->year, student->gpa, student->email);
}

static void display_header(void)
{
    puts("ID               Name                     Department         Year   GPA Email");
    puts("-----------------------------------------------------------------------------------------------");
}

static void register_student(void)
{
    Student student = {0};
    if (read_student_details(&student, 1) && add_student(&student))
        puts("Student registered.");
}

static void search_student(void)
{
    char id[ID_SIZE];
    StudentNode *node;
    if (!read_line("Student ID: ", id, sizeof id))
        return;
    node = find_node(id);
    if (node == NULL) {
        puts("Student not found.");
        return;
    }
    display_header();
    display_student(&node->student);
}

static void update_student(void)
{
    char id[ID_SIZE];
    Student updated;
    StudentNode *node;

    if (!read_line("Student ID to update: ", id, sizeof id))
        return;
    node = find_node(id);
    if (node == NULL) {
        puts("Student not found.");
        return;
    }
    updated = node->student;
    if (read_student_details(&updated, 0)) {
        node->student = updated;
        puts("Student details updated.");
    }
}

static void remove_student(void)
{
    char id[ID_SIZE];
    StudentNode **link;
    if (!read_line("Student ID to remove: ", id, sizeof id))
        return;
    if (table_size == 0) {
        puts("Student not found.");
        return;
    }
    link = &table[hash_id(id)];
    while (*link != NULL && strcmp((*link)->student.id, id) != 0)
        link = &(*link)->next;
    if (*link == NULL) {
        puts("Student not found.");
        return;
    }
    {
        StudentNode *removed = *link;
        *link = removed->next;
        free(removed);
        --student_count;
    }
    puts("Student removed.");
}

typedef int (*StudentComparator)(const void *, const void *);

static int compare_text(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        int left = tolower((unsigned char)*a++);
        int right = tolower((unsigned char)*b++);
        if (left != right)
            return left - right;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int compare_name(const void *left, const void *right)
{
    const Student *a = *(const Student * const *)left;
    const Student *b = *(const Student * const *)right;
    int result = compare_text(a->name, b->name);
    return result != 0 ? result : strcmp(a->id, b->id);
}

static int compare_department(const void *left, const void *right)
{
    const Student *a = *(const Student * const *)left;
    const Student *b = *(const Student * const *)right;
    int result = compare_text(a->department, b->department);
    return result != 0 ? result : compare_name(left, right);
}

static int compare_gpa(const void *left, const void *right)
{
    const Student *a = *(const Student * const *)left;
    const Student *b = *(const Student * const *)right;
    if (a->gpa < b->gpa)
        return 1;
    if (a->gpa > b->gpa)
        return -1;
    return compare_name(left, right);
}

static void list_students(void)
{
    Student **students;
    size_t index = 0;
    int choice;
    char input[32];
    char *end;
    long selected;
    StudentComparator comparator = NULL;
    StudentNode *node;

    if (student_count == 0) {
        puts("No student records.");
        return;
    }
    puts("Sort by: 1) Name  2) Department  3) GPA (highest first)  4) Unsorted");
    if (!read_line("Choice: ", input, sizeof input))
        return;
    selected = strtol(input, &end, 10);
    if (input[0] == '\0' || *end != '\0' || selected < 1 || selected > 4) {
        puts("Invalid choice.");
        return;
    }
    choice = (int)selected;
    if (choice == 1)
        comparator = compare_name;
    else if (choice == 2)
        comparator = compare_department;
    else if (choice == 3)
        comparator = compare_gpa;
    else if (choice != 4) {
        puts("Invalid choice.");
        return;
    }

    students = (Student **)malloc(student_count * sizeof *students);
    if (students == NULL) {
        perror("Unable to allocate sort list");
        return;
    }
    for (size_t bucket = 0; bucket < table_size; ++bucket)
        for (node = table[bucket]; node != NULL; node = node->next)
            students[index++] = &node->student;
    if (comparator != NULL)
        qsort(students, student_count, sizeof *students, comparator);

    display_header();
    for (index = 0; index < student_count; ++index)
        display_student(students[index]);
    printf("Total students: %zu\n", student_count);
    free(students);
}

static int save_students(void)
{
    FILE *file = fopen(DATA_FILE, "w");
    if (file == NULL) {
        perror("Unable to save student data");
        return 0;
    }
    for (size_t bucket = 0; bucket < table_size; ++bucket) {
        StudentNode *node;
        for (node = table[bucket]; node != NULL; node = node->next) {
            Student *s = &node->student;
            if (fprintf(file, "%s|%s|%s|%d|%.2f|%s\n", s->id, s->name,
                        s->department, s->year, s->gpa, s->email) < 0) {
                perror("Unable to write student data");
                fclose(file);
                return 0;
            }
        }
    }
    if (fclose(file) != 0) {
        perror("Unable to close student data file");
        return 0;
    }
    return 1;
}

static void load_students(void)
{
    FILE *file = fopen(DATA_FILE, "r");
    char line[LINE_SIZE];
    size_t line_number = 0;

    if (file == NULL)
        return;
    while (fgets(line, sizeof line, file) != NULL) {
        Student student;
        ++line_number;
        if (sscanf(line, "%23[^|]|%79[^|]|%59[^|]|%d|%f|%99[^\r\n]",
                   student.id, student.name, student.department, &student.year,
                   &student.gpa, student.email) != 6 ||
            student.year < 1 || student.year > 8 ||
            !(student.gpa >= 0.0f && student.gpa <= 10.0f) ||
            find_node(student.id) != NULL || !add_student(&student)) {
            fprintf(stderr, "Skipping invalid record at %s:%zu\n",
                    DATA_FILE, line_number);
        }
    }
    if (ferror(file))
        perror("Unable to read student data");
    fclose(file);
}

static void free_students(void)
{
    for (size_t bucket = 0; bucket < table_size; ++bucket) {
        StudentNode *node = table[bucket];
        while (node != NULL) {
            StudentNode *next = node->next;
            free(node);
            node = next;
        }
    }
    free(table);
    table = NULL;
    table_size = 0;
}

static int run_benchmark(void)
{
    const size_t sizes[] = {100, 1000, 10000};
    const size_t lookup_count = 500000;
    volatile size_t hits = 0;

    puts("Student module benchmark (synthetic records; lower time is better)");
    puts("Records | Lookup: 500,000 hits (ms) | Sort by name: avg ms/sort");

    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; ++s) {
        size_t count = sizes[s];
        size_t sort_rounds = count == 100 ? 200 : (count == 1000 ? 50 : 10);
        char (*ids)[ID_SIZE] = (char (*)[ID_SIZE])malloc(count * sizeof *ids);
        Student **items = (Student **)malloc(count * sizeof *items);
        clock_t start;
        clock_t finish;
        clock_t sort_ticks = 0;
        double lookup_ms;
        double sort_ms;

        free_students();
        if (ids == NULL || items == NULL) {
            perror("Unable to allocate benchmark data");
            free(ids);
            free(items);
            free_students();
            return 1;
        }

        for (size_t i = 0; i < count; ++i) {
            Student student = {0};
            snprintf(student.id, sizeof student.id, "S%08lu",
                     (unsigned long)i);
            snprintf(student.name, sizeof student.name, "Student %08lu",
                     (unsigned long)(count - i));
            strcpy(student.department, "Computer Science");
            student.year = 2;
            student.gpa = (float)(i % 101) / 10.0f;
            strcpy(student.email, "benchmark@example.edu");
            strcpy(ids[i], student.id);
            if (!add_student(&student)) {
                free(ids);
                free(items);
                free_students();
                return 1;
            }
        }

        start = clock();
        for (size_t i = 0; i < lookup_count; ++i)
            hits += find_node(ids[(i * 7919) % count]) != NULL;
        finish = clock();
        lookup_ms = 1000.0 * (double)(finish - start) / CLOCKS_PER_SEC;

        for (size_t round = 0; round < sort_rounds; ++round) {
            size_t index = 0;
            for (size_t bucket = 0; bucket < table_size; ++bucket) {
                StudentNode *node;
                for (node = table[bucket]; node != NULL; node = node->next)
                    items[index++] = &node->student;
            }
            start = clock();
            qsort(items, count, sizeof *items, compare_name);
            sort_ticks += clock() - start;
        }
        sort_ms = 1000.0 * (double)sort_ticks / CLOCKS_PER_SEC / sort_rounds;

        printf("%7lu | %24.3f | %24.3f\n",
               (unsigned long)count, lookup_ms, sort_ms);
        free(ids);
        free(items);
    }

    free_students();
    return hits == 3 * lookup_count ? 0 : 1;
}

int main(int argc, char **argv)
{
    char input[32];
    char *end;
    long choice;
    int running = 1;

    if (argc == 2 && strcmp(argv[1], "--benchmark") == 0)
        return run_benchmark();
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--benchmark]\n", argv[0]);
        return 2;
    }

    load_students();
    printf("CampusAssist - Student Management (%zu records loaded)\n",
           student_count);

    while (running) {
        puts("\n1. Register student\n2. Find by ID\n3. Update student"
             "\n4. Remove student\n5. Display / sort students\n6. Save now"
             "\n0. Save and exit");
        if (!read_line("Select: ", input, sizeof input))
            break;
        choice = strtol(input, &end, 10);
        if (input[0] == '\0' || *end != '\0') {
            puts("Enter a menu number.");
            continue;
        }
        switch (choice) {
        case 1: register_student(); break;
        case 2: search_student(); break;
        case 3: update_student(); break;
        case 4: remove_student(); break;
        case 5: list_students(); break;
        case 6:
            if (save_students())
                puts("Student records saved.");
            break;
        case 0:
            if (save_students())
                puts("Student records saved.");
            running = 0;
            break;
        default: puts("Unknown menu option."); break;
        }
    }
    free_students();
    return 0;
}
