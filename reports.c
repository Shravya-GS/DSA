#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 1024
#define ID_SIZE 100
#define NAME_SIZE 80
#define DEPT_SIZE 60
#define EMAIL_SIZE 100
#define TITLE_SIZE 128
#define AUTHOR_SIZE 96
#define CATEGORY_SIZE 48
#define DESCRIPTION_SIZE 240

typedef struct {
    char id[ID_SIZE];
    char name[NAME_SIZE];
    char department[DEPT_SIZE];
    int year;
    float gpa;
    char email[EMAIL_SIZE];
} Student;

typedef struct {
    char id[ID_SIZE];
    char title[TITLE_SIZE];
    char author[AUTHOR_SIZE];
    int total_copies;
} Book;

typedef struct {
    unsigned long id;
    char book_id[ID_SIZE];
    char student_id[ID_SIZE];
    int active;
} Loan;

typedef struct {
    int id;
    int student_id;
    int priority;
    int status;
    unsigned long long submitted_order;
    char category[CATEGORY_SIZE];
    char description[DESCRIPTION_SIZE];
} Complaint;

typedef struct {
    unsigned long id;
    char book_id[ID_SIZE];
    char student_id[ID_SIZE];
    unsigned long long queue_order;
    int status;
} Reservation;

typedef struct {
    Student *items;
    size_t count;
    size_t capacity;
    int loaded;
} StudentList;

typedef struct {
    Book *items;
    size_t count;
    size_t capacity;
    int loaded;
} BookList;

typedef struct {
    Loan *items;
    size_t count;
    size_t capacity;
    int loaded;
} LoanList;

typedef struct {
    Complaint *items;
    size_t count;
    size_t capacity;
    int loaded;
} ComplaintList;

typedef struct {
    Reservation *items;
    size_t count;
    size_t capacity;
    int loaded;
} ReservationList;

static StudentList students;
static BookList books;
static LoanList loans;
static ComplaintList complaints;
static ReservationList reservations;

static int reserve_array(void **items, size_t *capacity, size_t needed,
                         size_t item_size)
{
    size_t next_capacity;
    void *new_items;

    if (needed <= *capacity)
        return 1;
    next_capacity = *capacity == 0 ? 16 : *capacity;
    while (next_capacity < needed) {
        if (next_capacity > (size_t)-1 / 2)
            return 0;
        next_capacity *= 2;
    }
    if (next_capacity > (size_t)-1 / item_size)
        return 0;
    new_items = realloc(*items, next_capacity * item_size);
    if (new_items == NULL)
        return 0;
    *items = new_items;
    *capacity = next_capacity;
    return 1;
}

static int read_line(FILE *file, char *line, size_t capacity)
{
    size_t length;

    if (fgets(line, (int)capacity, file) == NULL)
        return feof(file) ? 0 : -1;
    length = strlen(line);
    if (length > 0 && line[length - 1] == '\n')
        line[--length] = '\0';
    else if (!feof(file)) {
        int ch;
        while ((ch = fgetc(file)) != '\n' && ch != EOF) {
        }
        return -1;
    }
    if (length > 0 && line[length - 1] == '\r')
        line[--length] = '\0';
    return 1;
}

static int split_fields(char *line, char **fields, size_t field_count)
{
    size_t i;
    char *cursor = line;

    for (i = 0; i + 1 < field_count; ++i) {
        char *separator = strchr(cursor, '|');
        if (separator == NULL)
            return 0;
        *separator = '\0';
        fields[i] = cursor;
        cursor = separator + 1;
    }
    if (strchr(cursor, '|') != NULL)
        return 0;
    fields[field_count - 1] = cursor;
    return 1;
}

static int copy_text(char *destination, size_t capacity, const char *source)
{
    size_t length = strlen(source);

    if (length == 0 || length >= capacity)
        return 0;
    memcpy(destination, source, length + 1);
    return 1;
}

static int parse_long(const char *text, long minimum, long maximum, long *value)
{
    char *end;
    long parsed;

    if (text[0] == '\0')
        return 0;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed < minimum ||
        parsed > maximum)
        return 0;
    *value = parsed;
    return 1;
}

static int parse_ulong_long(const char *text, unsigned long long *value)
{
    char *end;
    unsigned long long parsed;

    if (text[0] == '\0' || text[0] == '-')
        return 0;
    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (errno == ERANGE || *end != '\0')
        return 0;
    *value = parsed;
    return 1;
}

static int open_data_file(const char *path, const char *header, FILE **file)
{
    char line[LINE_SIZE];
    int result;

    *file = fopen(path, "r");
    if (*file == NULL) {
        if (errno == ENOENT)
            return 0;
        perror(path);
        return -1;
    }
    if (header == NULL)
        return 1;

    result = read_line(*file, line, sizeof line);
    if (result != 1 || strcmp(line, header) != 0) {
        fprintf(stderr, "%s: missing or invalid file header\n", path);
        fclose(*file);
        *file = NULL;
        return -1;
    }
    return 1;
}

static int load_students(const char *path)
{
    FILE *file;
    char line[LINE_SIZE];
    int opened = open_data_file(path, NULL, &file);
    int result;

    if (opened <= 0)
        return opened;
    while ((result = read_line(file, line, sizeof line)) == 1) {
        char *fields[6];
        char *end;
        Student item;
        float gpa;

        if (!split_fields(line, fields, 6) ||
            !copy_text(item.id, sizeof item.id, fields[0]) ||
            !copy_text(item.name, sizeof item.name, fields[1]) ||
            !copy_text(item.department, sizeof item.department, fields[2]) ||
            strlen(fields[5]) >= sizeof item.email) {
            fprintf(stderr, "%s: invalid student record\n", path);
            fclose(file);
            return -1;
        }
        memcpy(item.email, fields[5], strlen(fields[5]) + 1);
        {
            long year;
            if (!parse_long(fields[3], 1, 8, &year)) {
                fprintf(stderr, "%s: invalid student year\n", path);
                fclose(file);
                return -1;
            }
            item.year = (int)year;
        }
        errno = 0;
        gpa = strtof(fields[4], &end);
        if (errno == ERANGE || fields[4][0] == '\0' || *end != '\0' ||
            !(gpa >= 0.0f && gpa <= 10.0f)) {
            fprintf(stderr, "%s: invalid student GPA\n", path);
            fclose(file);
            return -1;
        }
        item.gpa = gpa;
        if (!reserve_array((void **)&students.items, &students.capacity,
                           students.count + 1, sizeof item)) {
            perror("Unable to allocate student report");
            fclose(file);
            return -1;
        }
        students.items[students.count++] = item;
    }
    if (result < 0 || ferror(file)) {
        fprintf(stderr, "%s: invalid or unreadable record\n", path);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        perror(path);
        return -1;
    }
    students.loaded = 1;
    return 1;
}

static int load_books(const char *path)
{
    FILE *file;
    char line[LINE_SIZE];
    int opened = open_data_file(path, "LIBRARY_BOOKS_V1", &file);
    int result;

    if (opened <= 0)
        return opened;
    while ((result = read_line(file, line, sizeof line)) == 1) {
        char *fields[4];
        Book item;
        long copies;

        if (!split_fields(line, fields, 4) ||
            !copy_text(item.id, sizeof item.id, fields[0]) ||
            !copy_text(item.title, sizeof item.title, fields[1]) ||
            !copy_text(item.author, sizeof item.author, fields[2]) ||
            !parse_long(fields[3], 1, INT_MAX, &copies)) {
            fprintf(stderr, "%s: invalid book record\n", path);
            fclose(file);
            return -1;
        }
        item.total_copies = (int)copies;
        if (!reserve_array((void **)&books.items, &books.capacity,
                           books.count + 1, sizeof item)) {
            perror("Unable to allocate book report");
            fclose(file);
            return -1;
        }
        books.items[books.count++] = item;
    }
    if (result < 0 || ferror(file)) {
        fprintf(stderr, "%s: invalid or unreadable record\n", path);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        perror(path);
        return -1;
    }
    books.loaded = 1;
    return 1;
}

static int load_loans(const char *path)
{
    FILE *file;
    char line[LINE_SIZE];
    int opened = open_data_file(path, "LIBRARY_LOANS_V1", &file);
    int result;

    if (opened <= 0)
        return opened;
    while ((result = read_line(file, line, sizeof line)) == 1) {
        char *fields[4];
        Loan item;
        unsigned long long id;
        long active;

        if (!split_fields(line, fields, 4) ||
            !parse_ulong_long(fields[0], &id) || id > ULONG_MAX ||
            !copy_text(item.book_id, sizeof item.book_id, fields[1]) ||
            !copy_text(item.student_id, sizeof item.student_id, fields[2]) ||
            !parse_long(fields[3], 0, 1, &active)) {
            fprintf(stderr, "%s: invalid loan record\n", path);
            fclose(file);
            return -1;
        }
        item.id = (unsigned long)id;
        item.active = (int)active;
        if (!reserve_array((void **)&loans.items, &loans.capacity,
                           loans.count + 1, sizeof item)) {
            perror("Unable to allocate loan report");
            fclose(file);
            return -1;
        }
        loans.items[loans.count++] = item;
    }
    if (result < 0 || ferror(file)) {
        fprintf(stderr, "%s: invalid or unreadable record\n", path);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        perror(path);
        return -1;
    }
    loans.loaded = 1;
    return 1;
}

static int load_complaints(const char *path)
{
    FILE *file;
    char line[LINE_SIZE];
    int opened = open_data_file(path, "CAMPUS_COMPLAINTS_V1", &file);
    int result;

    if (opened <= 0)
        return opened;
    while ((result = read_line(file, line, sizeof line)) == 1) {
        char *fields[7];
        Complaint item;
        long id;
        long student_id;
        long priority;
        long status;
        unsigned long long order;

        if (!split_fields(line, fields, 7) ||
            !parse_long(fields[0], 1, INT_MAX, &id) ||
            !parse_long(fields[1], 1, INT_MAX, &student_id) ||
            !parse_long(fields[2], 0, 3, &priority) ||
            !parse_long(fields[3], 0, 2, &status) ||
            !parse_ulong_long(fields[4], &order) || order == 0 ||
            !copy_text(item.category, sizeof item.category, fields[5]) ||
            !copy_text(item.description, sizeof item.description, fields[6])) {
            fprintf(stderr, "%s: invalid complaint record\n", path);
            fclose(file);
            return -1;
        }
        item.id = (int)id;
        item.student_id = (int)student_id;
        item.priority = (int)priority;
        item.status = (int)status;
        item.submitted_order = order;
        if (!reserve_array((void **)&complaints.items, &complaints.capacity,
                           complaints.count + 1, sizeof item)) {
            perror("Unable to allocate complaint report");
            fclose(file);
            return -1;
        }
        complaints.items[complaints.count++] = item;
    }
    if (result < 0 || ferror(file)) {
        fprintf(stderr, "%s: invalid or unreadable record\n", path);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        perror(path);
        return -1;
    }
    complaints.loaded = 1;
    return 1;
}

static int load_reservations(const char *path)
{
    FILE *file;
    char line[LINE_SIZE];
    int opened = open_data_file(path, "CAMPUS_RESERVATIONS_V1", &file);
    int result;

    if (opened <= 0)
        return opened;
    while ((result = read_line(file, line, sizeof line)) == 1) {
        char *fields[5];
        Reservation item;
        unsigned long long id;
        unsigned long long order;

        if (!split_fields(line, fields, 5) ||
            !parse_ulong_long(fields[0], &id) || id > ULONG_MAX ||
            !copy_text(item.book_id, sizeof item.book_id, fields[1]) ||
            !copy_text(item.student_id, sizeof item.student_id, fields[2]) ||
            !parse_ulong_long(fields[3], &order) ||
            (strcmp(fields[4], "WAITING") != 0 &&
             strcmp(fields[4], "OFFERED") != 0 &&
             strcmp(fields[4], "CANCELLED") != 0 &&
             strcmp(fields[4], "FULFILLED") != 0)) {
            fprintf(stderr, "%s: invalid reservation record\n", path);
            fclose(file);
            return -1;
        }
        item.id = (unsigned long)id;
        item.queue_order = order;
        item.status = strcmp(fields[4], "WAITING") == 0 ? 0 :
                      strcmp(fields[4], "OFFERED") == 0 ? 1 :
                      strcmp(fields[4], "CANCELLED") == 0 ? 2 : 3;
        if (!reserve_array((void **)&reservations.items,
                           &reservations.capacity,
                           reservations.count + 1, sizeof item)) {
            perror("Unable to allocate reservation report");
            fclose(file);
            return -1;
        }
        reservations.items[reservations.count++] = item;
    }
    if (result < 0 || ferror(file)) {
        fprintf(stderr, "%s: invalid or unreadable record\n", path);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        perror(path);
        return -1;
    }
    reservations.loaded = 1;
    return 1;
}

static int find_book_index(const char *id)
{
    size_t i;
    for (i = 0; i < books.count; ++i)
        if (strcmp(books.items[i].id, id) == 0)
            return (int)i;
    return -1;
}

static int validate_library_data(void)
{
    size_t i;
    size_t j;

    if (!books.loaded || !loans.loaded)
        return 1;
    for (i = 0; i < loans.count; ++i) {
        if (!loans.items[i].active)
            continue;
        if (find_book_index(loans.items[i].book_id) < 0) {
            fprintf(stderr, "Active loan %lu refers to missing book '%s'\n",
                    loans.items[i].id, loans.items[i].book_id);
            return 0;
        }
    }
    for (i = 0; i < books.count; ++i) {
        size_t active = 0;
        for (j = 0; j < loans.count; ++j)
            if (loans.items[j].active &&
                strcmp(loans.items[j].book_id, books.items[i].id) == 0)
                ++active;
        if (active > (size_t)books.items[i].total_copies) {
            fprintf(stderr, "Active loans exceed copies for book '%s'\n",
                    books.items[i].id);
            return 0;
        }
    }
    return 1;
}

static size_t active_loan_count(void)
{
    size_t i;
    size_t count = 0;
    for (i = 0; i < loans.count; ++i)
        if (loans.items[i].active)
            ++count;
    return count;
}

static size_t waiting_reservation_count(void)
{
    size_t i;
    size_t count = 0;
    for (i = 0; i < reservations.count; ++i)
        if (reservations.items[i].status == 0)
            ++count;
    return count;
}

static const char *priority_name(int priority)
{
    static const char *names[] = {"Critical", "High", "Medium", "Low"};
    return priority >= 0 && priority < 4 ? names[priority] : "Invalid";
}

static const char *status_name(int status)
{
    static const char *names[] = {"Pending", "In progress", "Resolved"};
    return status >= 0 && status < 3 ? names[status] : "Invalid";
}

static int compare_text(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        int a = tolower((unsigned char)*left++);
        int b = tolower((unsigned char)*right++);
        if (a != b)
            return a - b;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

static int compare_students_by_name(const void *left, const void *right)
{
    const Student *a = (const Student *)left;
    const Student *b = (const Student *)right;
    int result = compare_text(a->name, b->name);
    return result != 0 ? result : compare_text(a->id, b->id);
}

static int compare_students_by_gpa(const void *left, const void *right)
{
    const Student *a = (const Student *)left;
    const Student *b = (const Student *)right;
    if (a->gpa > b->gpa)
        return -1;
    if (a->gpa < b->gpa)
        return 1;
    return compare_students_by_name(left, right);
}

static int compare_books_by_title(const void *left, const void *right)
{
    const Book *a = (const Book *)left;
    const Book *b = (const Book *)right;
    int result = compare_text(a->title, b->title);
    return result != 0 ? result : compare_text(a->id, b->id);
}

static int compare_complaints_by_priority(const void *left, const void *right)
{
    const Complaint *a = (const Complaint *)left;
    const Complaint *b = (const Complaint *)right;
    if (a->priority != b->priority)
        return a->priority - b->priority;
    if (a->submitted_order < b->submitted_order)
        return -1;
    if (a->submitted_order > b->submitted_order)
        return 1;
    return 0;
}

static int compare_complaints_by_status(const void *left, const void *right)
{
    const Complaint *a = (const Complaint *)left;
    const Complaint *b = (const Complaint *)right;
    if (a->status != b->status)
        return a->status - b->status;
    return compare_complaints_by_priority(left, right);
}

static int compare_reservations_by_book_order(const void *left,
                                              const void *right)
{
    const Reservation *a = (const Reservation *)left;
    const Reservation *b = (const Reservation *)right;
    int result = compare_text(a->book_id, b->book_id);
    if (result != 0)
        return result;
    if (a->queue_order < b->queue_order)
        return -1;
    if (a->queue_order > b->queue_order)
        return 1;
    if (a->id < b->id)
        return -1;
    if (a->id > b->id)
        return 1;
    return 0;
}

static void print_data_status(const char *label, int loaded)
{
    printf("%-18s %s\n", label, loaded ? "loaded" : "not found");
}

static void report_dashboard(void)
{
    size_t i;
    size_t total_copies = 0;
    size_t available_copies = 0;
    size_t complaint_statuses[3] = {0, 0, 0};
    size_t complaint_priorities[4] = {0, 0, 0, 0};

    puts("\n=== CampusAssist summary ===");
    print_data_status("Students", students.loaded);
    print_data_status("Book catalog", books.loaded);
    print_data_status("Loan history", loans.loaded);
    print_data_status("Complaints", complaints.loaded);
    print_data_status("Reservations", reservations.loaded);

    if (students.loaded)
        printf("Registered students: %lu\n", (unsigned long)students.count);
    if (books.loaded) {
        for (i = 0; i < books.count; ++i)
            total_copies += (size_t)books.items[i].total_copies;
        printf("Book titles: %lu\n", (unsigned long)books.count);
        printf("Total book copies: %lu\n", (unsigned long)total_copies);
    }
    if (loans.loaded)
        printf("Currently issued books: %lu\n",
               (unsigned long)active_loan_count());
    if (books.loaded && loans.loaded) {
        for (i = 0; i < books.count; ++i) {
            size_t active = 0;
            size_t j;
            for (j = 0; j < loans.count; ++j)
                if (loans.items[j].active &&
                    strcmp(loans.items[j].book_id, books.items[i].id) == 0)
                    ++active;
            available_copies +=
                (size_t)books.items[i].total_copies - active;
        }
        printf("Available book copies: %lu\n",
               (unsigned long)available_copies);
    } else {
        puts("Available book copies: unavailable (catalog or loan file missing)");
    }
    if (reservations.loaded)
        printf("Waiting reservations: %lu\n",
               (unsigned long)waiting_reservation_count());
    if (complaints.loaded) {
        for (i = 0; i < complaints.count; ++i) {
            ++complaint_statuses[complaints.items[i].status];
            ++complaint_priorities[complaints.items[i].priority];
        }
        printf("Complaints: %lu total; %lu pending, %lu in progress, %lu resolved\n",
               (unsigned long)complaints.count,
               (unsigned long)complaint_statuses[0],
               (unsigned long)complaint_statuses[1],
               (unsigned long)complaint_statuses[2]);
        printf("Priority totals: Critical %lu, High %lu, Medium %lu, Low %lu\n",
               (unsigned long)complaint_priorities[0],
               (unsigned long)complaint_priorities[1],
               (unsigned long)complaint_priorities[2],
               (unsigned long)complaint_priorities[3]);
    }
}

static void report_students(int sort_by_gpa)
{
    size_t i;
    if (!students.loaded) {
        puts("Student data file is not available.");
        return;
    }
    if (students.count > 1)
        qsort(students.items, students.count, sizeof *students.items,
              sort_by_gpa ? compare_students_by_gpa :
                            compare_students_by_name);
    puts("\nID | Name | Department | Year | GPA | Email");
    for (i = 0; i < students.count; ++i) {
        const Student *item = &students.items[i];
        printf("%s | %s | %s | %d | %.2f | %s\n", item->id, item->name,
               item->department, item->year, item->gpa, item->email);
    }
    printf("Total: %lu\n", (unsigned long)students.count);
}

static void report_books(void)
{
    size_t i;
    if (!books.loaded) {
        puts("Book catalog file is not available.");
        return;
    }
    if (books.count > 1)
        qsort(books.items, books.count, sizeof *books.items,
              compare_books_by_title);
    puts("\nID | Title | Author | Total copies | Available copies");
    for (i = 0; i < books.count; ++i) {
        const Book *item = &books.items[i];
        long long available = item->total_copies;
        size_t j;
        if (!loans.loaded) {
            printf("%s | %s | %s | %d | unavailable\n",
                   item->id, item->title, item->author, item->total_copies);
            continue;
        }
        for (j = 0; j < loans.count; ++j)
            if (loans.items[j].active &&
                strcmp(loans.items[j].book_id, item->id) == 0)
                --available;
        printf("%s | %s | %s | %d | %lld\n", item->id, item->title,
               item->author, item->total_copies, available);
    }
}

static void report_reservations(void)
{
    size_t i;
    int found = 0;
    if (!reservations.loaded) {
        puts("Reservation data file is not available.");
        return;
    }
    if (reservations.count > 1)
        qsort(reservations.items, reservations.count,
              sizeof *reservations.items, compare_reservations_by_book_order);
    puts("\nWaiting reservations (ordered by book ID and queue order)");
    puts("Reservation ID | Book ID | Student ID | Queue order");
    for (i = 0; i < reservations.count; ++i) {
        const Reservation *item = &reservations.items[i];
        if (item->status != 0)
            continue;
        printf("%lu | %s | %s | %llu\n", item->id, item->book_id,
               item->student_id, item->queue_order);
        found = 1;
    }
    if (!found)
        puts("No waiting reservations.");
}

static void report_complaints(int by_status)
{
    size_t i;
    if (!complaints.loaded) {
        puts("Complaint data file is not available.");
        return;
    }
    if (complaints.count > 1)
        qsort(complaints.items, complaints.count, sizeof *complaints.items,
              by_status ? compare_complaints_by_status :
                          compare_complaints_by_priority);
    puts(by_status
         ? "\nComplaints sorted by status, then priority"
         : "\nComplaints sorted by priority (submission order breaks ties)");
    puts("ID | Student ID | Priority | Status | Category | Description");
    for (i = 0; i < complaints.count; ++i) {
        const Complaint *item = &complaints.items[i];
        printf("%d | %d | %s | %s | %s | %s\n", item->id,
               item->student_id, priority_name(item->priority),
               status_name(item->status), item->category, item->description);
    }
}

static void free_reports(void)
{
    free(students.items);
    free(books.items);
    free(loans.items);
    free(complaints.items);
    free(reservations.items);
}

int main(int argc, char **argv)
{
    const char *paths[5] = {
        "student_management/students.txt",
        "books.txt",
        "loans.txt",
        "topic4_complaints/complaints.dat",
        "reservations.txt"
    };
    int loaded[5];
    char input[32];
    char *end;
    long choice;
    int running = 1;
    size_t i;

    if (argc > 6) {
        fprintf(stderr,
                "Usage: %s [students-file [books-file [loans-file "
                "[complaints-file [reservations-file]]]]]\n", argv[0]);
        return 2;
    }
    for (i = 1; i < (size_t)argc; ++i)
        paths[i - 1] = argv[i];

    loaded[0] = load_students(paths[0]);
    loaded[1] = load_books(paths[1]);
    loaded[2] = load_loans(paths[2]);
    loaded[3] = load_complaints(paths[3]);
    loaded[4] = load_reservations(paths[4]);
    for (i = 0; i < 5; ++i) {
        if (loaded[i] < 0) {
            free_reports();
            return 1;
        }
    }
    if (!validate_library_data()) {
        free_reports();
        return 1;
    }

    puts("CampusAssist reports (reads saved files; does not modify them)");
    while (running) {
        puts("\n1. Summary dashboard\n2. Students by name\n"
             "3. Students by GPA\n4. Books by title\n"
             "5. Waiting reservations\n6. Complaints by priority\n"
             "7. Complaints by status\n0. Exit");
        printf("Select: ");
        if (fgets(input, sizeof input, stdin) == NULL)
            break;
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            puts("Enter a menu number.");
            continue;
        }
        errno = 0;
        choice = strtol(input, &end, 10);
        if (errno == ERANGE || end == input) {
            puts("Enter a menu number.");
            continue;
        }
        while (*end != '\0' && isspace((unsigned char)*end))
            ++end;
        if (*end != '\0') {
            puts("Enter a menu number.");
            continue;
        }
        switch (choice) {
        case 1: report_dashboard(); break;
        case 2: report_students(0); break;
        case 3: report_students(1); break;
        case 4: report_books(); break;
        case 5: report_reservations(); break;
        case 6: report_complaints(0); break;
        case 7: report_complaints(1); break;
        case 0: running = 0; break;
        default: puts("Unknown menu option."); break;
        }
    }
    free_reports();
    return 0;
}
