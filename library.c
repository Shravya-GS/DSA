#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_BOOKS 1000
#define MAX_LOANS 10000
#define ID_SIZE 32
#define TITLE_SIZE 128
#define AUTHOR_SIZE 96
#define LINE_SIZE 512

#define BOOKS_FILE "books.txt"
#define LOANS_FILE "loans.txt"
#define BOOKS_TEMP_FILE "books.txt.tmp"
#define LOANS_TEMP_FILE "loans.txt.tmp"

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

static Book books[MAX_BOOKS];
static Loan loans[MAX_LOANS];
static size_t book_count;
static size_t loan_count;
static unsigned long next_loan_id = 1;

static void trim(char *text)
{
    char *start = text;
    size_t length;

    while (isspace((unsigned char)*start)) {
        start++;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }
}

static int read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;

    printf("%s", prompt);
    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return 0;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else if (!feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        printf("Input is too long. Please try again.\n");
        return 0;
    }

    if (length > 0 && buffer[length - 1] == '\r') {
        buffer[length - 1] = '\0';
    }
    return 1;
}

static int valid_id(const char *id)
{
    size_t i;
    size_t length = strlen(id);

    if (length == 0 || length >= ID_SIZE) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)id[i];
        if (!isalnum(ch) && ch != '-' && ch != '_') {
            return 0;
        }
    }
    return 1;
}

static int read_id(const char *prompt, char *destination)
{
    char input[LINE_SIZE];

    if (!read_line(prompt, input, sizeof(input))) {
        return 0;
    }
    trim(input);
    if (!valid_id(input)) {
        printf("Use a non-empty ID containing only letters, digits, '-' or '_'.\n");
        return 0;
    }
    strcpy(destination, input);
    return 1;
}

static int parse_int(const char *text, int minimum, int maximum, int *result)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (errno == ERANGE || end == text || *end != '\0' ||
        value < minimum || value > maximum) {
        return 0;
    }
    *result = (int)value;
    return 1;
}

static int parse_ulong(const char *text, unsigned long *result)
{
    char *end;
    unsigned long value;

    if (text[0] == '-') {
        return 0;
    }
    errno = 0;
    value = strtoul(text, &end, 10);
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (errno == ERANGE || end == text || *end != '\0' || value == 0) {
        return 0;
    }
    *result = value;
    return 1;
}

static int read_int(const char *prompt, int minimum, int maximum, int *result)
{
    char input[LINE_SIZE];

    if (!read_line(prompt, input, sizeof(input))) {
        return 0;
    }
    trim(input);
    if (!parse_int(input, minimum, maximum, result)) {
        printf("Enter a whole number from %d to %d.\n", minimum, maximum);
        return 0;
    }
    return 1;
}

static int read_ulong(const char *prompt, unsigned long *result)
{
    char input[LINE_SIZE];

    if (!read_line(prompt, input, sizeof(input))) {
        return 0;
    }
    trim(input);
    if (!parse_ulong(input, result)) {
        printf("Enter a valid positive loan ID.\n");
        return 0;
    }
    return 1;
}

static int valid_text(const char *text, size_t capacity)
{
    size_t length = strlen(text);
    return length > 0 && length < capacity && strchr(text, '|') == NULL;
}

static int read_text(const char *prompt, char *destination, size_t capacity)
{
    char input[LINE_SIZE];

    if (!read_line(prompt, input, sizeof(input))) {
        return 0;
    }
    trim(input);
    if (!valid_text(input, capacity)) {
        printf("Enter non-empty text (maximum %u characters; '|' is not allowed).\n",
               (unsigned)(capacity - 1));
        return 0;
    }
    strcpy(destination, input);
    return 1;
}

static int find_book(const char *id)
{
    size_t i;
    for (i = 0; i < book_count; i++) {
        if (strcmp(books[i].id, id) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static int active_loans_for_book(const char *book_id)
{
    size_t i;
    int count = 0;

    for (i = 0; i < loan_count; i++) {
        if (loans[i].active && strcmp(loans[i].book_id, book_id) == 0) {
            count++;
        }
    }
    return count;
}

static int find_active_loan(unsigned long id)
{
    size_t i;
    for (i = 0; i < loan_count; i++) {
        if (loans[i].id == id && loans[i].active) {
            return (int)i;
        }
    }
    return -1;
}

static int split_fields(char *line, char **fields, size_t expected)
{
    size_t i;
    char *separator;

    fields[0] = line;
    for (i = 1; i < expected; i++) {
        separator = strchr(fields[i - 1], '|');
        if (separator == NULL) {
            return 0;
        }
        *separator = '\0';
        fields[i] = separator + 1;
    }
    return strchr(fields[expected - 1], '|') == NULL;
}

static int read_record_line(FILE *file, char *line, size_t capacity)
{
    size_t length;

    if (fgets(line, (int)capacity, file) == NULL) {
        return 0;
    }
    length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[--length] = '\0';
    } else if (!feof(file)) {
        return -1;
    }
    if (length > 0 && line[length - 1] == '\r') {
        line[length - 1] = '\0';
    }
    return 1;
}

static int load_books(void)
{
    FILE *file = fopen(BOOKS_FILE, "r");
    char line[LINE_SIZE];
    int line_status;

    if (file == NULL) {
        if (errno == ENOENT) {
            return 1;
        }
        perror("Could not open " BOOKS_FILE);
        return 0;
    }
    line_status = read_record_line(file, line, sizeof(line));
    if (line_status != 1 || strcmp(line, "LIBRARY_BOOKS_V1") != 0) {
        fprintf(stderr, "Invalid or missing header in %s.\n", BOOKS_FILE);
        fclose(file);
        return 0;
    }

    while ((line_status = read_record_line(file, line, sizeof(line))) == 1) {
        char *fields[4];
        int copies;
        Book book;

        if (!split_fields(line, fields, 4) || !valid_id(fields[0]) ||
            !valid_text(fields[1], TITLE_SIZE) ||
            !valid_text(fields[2], AUTHOR_SIZE) ||
            !parse_int(fields[3], 1, INT_MAX, &copies) ||
            book_count == MAX_BOOKS || find_book(fields[0]) >= 0) {
            fprintf(stderr, "Invalid book record in %s.\n", BOOKS_FILE);
            fclose(file);
            return 0;
        }
        strcpy(book.id, fields[0]);
        strcpy(book.title, fields[1]);
        strcpy(book.author, fields[2]);
        book.total_copies = copies;
        books[book_count++] = book;
    }
    if (line_status < 0 || ferror(file)) {
        fprintf(stderr, "Could not read a complete record from %s.\n", BOOKS_FILE);
        fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        perror("Could not close " BOOKS_FILE);
        return 0;
    }
    return 1;
}

static int load_loans(void)
{
    FILE *file = fopen(LOANS_FILE, "r");
    char line[LINE_SIZE];
    int line_status;
    unsigned long largest_loan_id = 0;

    if (file == NULL) {
        if (errno == ENOENT) {
            return 1;
        }
        perror("Could not open " LOANS_FILE);
        return 0;
    }
    line_status = read_record_line(file, line, sizeof(line));
    if (line_status != 1 || strcmp(line, "LIBRARY_LOANS_V1") != 0) {
        fprintf(stderr, "Invalid or missing header in %s.\n", LOANS_FILE);
        fclose(file);
        return 0;
    }

    while ((line_status = read_record_line(file, line, sizeof(line))) == 1) {
        char *fields[4];
        unsigned long id;
        int active;
        size_t i;
        Loan loan;

        if (!split_fields(line, fields, 4) ||
            !parse_ulong(fields[0], &id) || !valid_id(fields[1]) ||
            !valid_id(fields[2]) || !parse_int(fields[3], 0, 1, &active) ||
            loan_count == MAX_LOANS) {
            fprintf(stderr, "Invalid loan record in %s.\n", LOANS_FILE);
            fclose(file);
            return 0;
        }
        for (i = 0; i < loan_count; i++) {
            if (loans[i].id == id) {
                fprintf(stderr, "Duplicate loan ID in %s.\n", LOANS_FILE);
                fclose(file);
                return 0;
            }
        }
        loan.id = id;
        strcpy(loan.book_id, fields[1]);
        strcpy(loan.student_id, fields[2]);
        loan.active = active;
        loans[loan_count++] = loan;
        if (id > largest_loan_id) {
            largest_loan_id = id;
        }
    }
    if (line_status < 0 || ferror(file)) {
        fprintf(stderr, "Could not read a complete record from %s.\n", LOANS_FILE);
        fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        perror("Could not close " LOANS_FILE);
        return 0;
    }
    if (largest_loan_id == ULONG_MAX) {
        next_loan_id = 0;
    } else {
        next_loan_id = largest_loan_id + 1;
    }

    for (size_t i = 0; i < loan_count; i++) {
        if (find_book(loans[i].book_id) < 0) {
            fprintf(stderr, "Loan %lu refers to missing book '%s'.\n",
                    loans[i].id, loans[i].book_id);
            return 0;
        }
    }
    for (size_t i = 0; i < book_count; i++) {
        if (active_loans_for_book(books[i].id) > books[i].total_copies) {
            fprintf(stderr, "Active loans exceed copies for book '%s'.\n", books[i].id);
            return 0;
        }
    }
    return 1;
}

static int replace_saved_file(const char *temporary, const char *target)
{
#ifdef _WIN32
    if (!MoveFileExA(temporary, target,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fprintf(stderr, "Could not replace %s (Windows error %lu).\n",
                target, (unsigned long)GetLastError());
        remove(temporary);
        return 0;
    }
#else
    if (rename(temporary, target) != 0) {
        perror("Could not replace saved data file");
        remove(temporary);
        return 0;
    }
#endif
    return 1;
}

static int save_books(void)
{
    FILE *file = fopen(BOOKS_TEMP_FILE, "w");
    size_t i;

    if (file == NULL) {
        perror("Could not create " BOOKS_TEMP_FILE);
        return 0;
    }
    if (fprintf(file, "LIBRARY_BOOKS_V1\n") < 0) {
        perror("Could not write " BOOKS_TEMP_FILE);
        fclose(file);
        remove(BOOKS_TEMP_FILE);
        return 0;
    }
    for (i = 0; i < book_count; i++) {
        if (fprintf(file, "%s|%s|%s|%d\n", books[i].id, books[i].title,
                    books[i].author, books[i].total_copies) < 0) {
            perror("Could not write " BOOKS_TEMP_FILE);
            fclose(file);
            remove(BOOKS_TEMP_FILE);
            return 0;
        }
    }
    if (fclose(file) != 0) {
        perror("Could not finish saving " BOOKS_TEMP_FILE);
        remove(BOOKS_TEMP_FILE);
        return 0;
    }
    return replace_saved_file(BOOKS_TEMP_FILE, BOOKS_FILE);
}

static int save_loans(void)
{
    FILE *file = fopen(LOANS_TEMP_FILE, "w");
    size_t i;

    if (file == NULL) {
        perror("Could not create " LOANS_TEMP_FILE);
        return 0;
    }
    if (fprintf(file, "LIBRARY_LOANS_V1\n") < 0) {
        perror("Could not write " LOANS_TEMP_FILE);
        fclose(file);
        remove(LOANS_TEMP_FILE);
        return 0;
    }
    for (i = 0; i < loan_count; i++) {
        if (fprintf(file, "%lu|%s|%s|%d\n", loans[i].id, loans[i].book_id,
                    loans[i].student_id, loans[i].active) < 0) {
            perror("Could not write " LOANS_TEMP_FILE);
            fclose(file);
            remove(LOANS_TEMP_FILE);
            return 0;
        }
    }
    if (fclose(file) != 0) {
        perror("Could not finish saving " LOANS_TEMP_FILE);
        remove(LOANS_TEMP_FILE);
        return 0;
    }
    return replace_saved_file(LOANS_TEMP_FILE, LOANS_FILE);
}

static void print_book(const Book *book)
{
    int available = book->total_copies - active_loans_for_book(book->id);
    printf("%-12s %-30s %-24s %7d %10d\n", book->id, book->title,
           book->author, book->total_copies, available);
}

static void display_catalog(void)
{
    size_t i;

    if (book_count == 0) {
        printf("The catalog is empty.\n");
        return;
    }
    printf("%-12s %-30s %-24s %7s %10s\n",
           "Book ID", "Title", "Author", "Total", "Available");
    for (i = 0; i < book_count; i++) {
        print_book(&books[i]);
    }
}

static void add_book(void)
{
    Book book;

    if (book_count == MAX_BOOKS) {
        printf("The catalog has reached its capacity.\n");
        return;
    }
    if (!read_id("Book ID: ", book.id)) {
        return;
    }
    if (find_book(book.id) >= 0) {
        printf("A book with that ID already exists.\n");
        return;
    }
    if (!read_text("Title: ", book.title, sizeof(book.title)) ||
        !read_text("Author: ", book.author, sizeof(book.author)) ||
        !read_int("Total copies: ", 1, INT_MAX, &book.total_copies)) {
        return;
    }
    books[book_count++] = book;
    if (!save_books()) {
        book_count--;
        printf("The book was not added.\n");
        return;
    }
    printf("Book added.\n");
}

static void update_book(void)
{
    char id[ID_SIZE];
    Book original;
    Book updated;
    int index;
    int active;

    if (!read_id("Book ID to update: ", id)) {
        return;
    }
    index = find_book(id);
    if (index < 0) {
        printf("Book not found.\n");
        return;
    }
    original = books[index];
    updated = original;
    if (!read_text("New title: ", updated.title, sizeof(updated.title)) ||
        !read_text("New author: ", updated.author, sizeof(updated.author)) ||
        !read_int("New total copies: ", 1, INT_MAX, &updated.total_copies)) {
        return;
    }
    active = active_loans_for_book(id);
    if (updated.total_copies < active) {
        printf("Total copies cannot be less than the %d active loan(s).\n", active);
        return;
    }
    books[index] = updated;
    if (!save_books()) {
        books[index] = original;
        printf("The update could not be saved.\n");
        return;
    }
    printf("Book updated.\n");
}

static void remove_book(void)
{
    char id[ID_SIZE];
    int index;
    Book removed;

    if (!read_id("Book ID to remove: ", id)) {
        return;
    }
    index = find_book(id);
    if (index < 0) {
        printf("Book not found.\n");
        return;
    }
    if (active_loans_for_book(id) > 0) {
        printf("This book has active loans and cannot be removed.\n");
        return;
    }
    removed = books[index];
    memmove(&books[index], &books[index + 1],
            (book_count - (size_t)index - 1) * sizeof(books[0]));
    book_count--;
    if (!save_books()) {
        memmove(&books[index + 1], &books[index],
                (book_count - (size_t)index) * sizeof(books[0]));
        books[index] = removed;
        book_count++;
        printf("The book was not removed.\n");
        return;
    }
    printf("Book removed.\n");
}

static int contains_case_insensitive(const char *text, const char *query)
{
    size_t i;
    size_t j;

    if (query[0] == '\0') {
        return 1;
    }
    for (i = 0; text[i] != '\0'; i++) {
        for (j = 0; query[j] != '\0' && text[i + j] != '\0'; j++) {
            if (tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)query[j])) {
                break;
            }
        }
        if (query[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

static void search_book(void)
{
    int choice;
    char query[LINE_SIZE];
    size_t i;
    int found = 0;

    printf("1. Search by book ID\n2. Search by title\n");
    if (!read_int("Choice: ", 1, 2, &choice)) {
        return;
    }
    if (choice == 1) {
        if (!read_id("Book ID: ", query)) {
            return;
        }
    } else {
        if (!read_line("Title or title fragment: ", query, sizeof(query))) {
            return;
        }
        trim(query);
        if (query[0] == '\0') {
            printf("Search text cannot be empty.\n");
            return;
        }
    }

    printf("%-12s %-30s %-24s %7s %10s\n",
           "Book ID", "Title", "Author", "Total", "Available");
    for (i = 0; i < book_count; i++) {
        if ((choice == 1 && strcmp(books[i].id, query) == 0) ||
            (choice == 2 && contains_case_insensitive(books[i].title, query))) {
            print_book(&books[i]);
            found = 1;
        }
    }
    if (!found) {
        printf("No matching book found.\n");
    }
}

static void issue_book(void)
{
    char student_id[ID_SIZE];
    char book_id[ID_SIZE];
    int index;
    Loan loan;

    if (loan_count == MAX_LOANS) {
        printf("The loan history has reached its capacity.\n");
        return;
    }
    if (next_loan_id == 0) {
        printf("No more loan IDs are available.\n");
        return;
    }
    if (!read_id("Student ID: ", student_id) || !read_id("Book ID: ", book_id)) {
        return;
    }
    index = find_book(book_id);
    if (index < 0) {
        printf("Book not found.\n");
        return;
    }
    if (active_loans_for_book(book_id) >= books[index].total_copies) {
        printf("No copies are currently available.\n");
        return;
    }

    loan.id = next_loan_id;
    strcpy(loan.student_id, student_id);
    strcpy(loan.book_id, book_id);
    loan.active = 1;
    loans[loan_count++] = loan;
    if (!save_loans()) {
        loan_count--;
        printf("The book was not issued.\n");
        return;
    }
    if (next_loan_id == ULONG_MAX) {
        next_loan_id = 0;
    } else {
        next_loan_id++;
    }
    printf("Book issued. Loan ID: %lu\n", loan.id);
}

static void return_book(void)
{
    unsigned long id;
    int index;
    int previous_active;

    if (!read_ulong("Loan ID to return: ", &id)) {
        return;
    }
    index = find_active_loan(id);
    if (index < 0) {
        printf("No active loan has that ID.\n");
        return;
    }
    previous_active = loans[index].active;
    loans[index].active = 0;
    if (!save_loans()) {
        loans[index].active = previous_active;
        printf("The return could not be saved.\n");
        return;
    }
    printf("Book returned. Availability has been updated.\n");
}

static void display_active_loans(void)
{
    size_t i;
    int found = 0;

    printf("%-12s %-16s %-12s %-30s\n",
           "Loan ID", "Student ID", "Book ID", "Title");
    for (i = 0; i < loan_count; i++) {
        int book_index;
        if (!loans[i].active) {
            continue;
        }
        book_index = find_book(loans[i].book_id);
        printf("%-12lu %-16s %-12s %-30s\n", loans[i].id,
               loans[i].student_id, loans[i].book_id,
               books[book_index].title);
        found = 1;
    }
    if (!found) {
        printf("There are no active loans.\n");
    }
}

static void print_menu(void)
{
    printf("\n=== Library Catalog and Book Management ===\n"
           "1. Add a book\n"
           "2. Update book details\n"
           "3. Remove a book\n"
           "4. Display catalog\n"
           "5. Search books\n"
           "6. Issue a book\n"
           "7. Return a book\n"
           "8. Display active loans\n"
           "0. Save and exit\n");
}

int main(void)
{
    int choice;

    if (!load_books() || !load_loans()) {
        fprintf(stderr, "Startup stopped because saved data could not be loaded.\n");
        return EXIT_FAILURE;
    }

    for (;;) {
        print_menu();
        if (!read_int("Select an option: ", 0, 8, &choice)) {
            if (feof(stdin)) {
                break;
            }
            continue;
        }
        switch (choice) {
        case 1:
            add_book();
            break;
        case 2:
            update_book();
            break;
        case 3:
            remove_book();
            break;
        case 4:
            display_catalog();
            break;
        case 5:
            search_book();
            break;
        case 6:
            issue_book();
            break;
        case 7:
            return_book();
            break;
        case 8:
            display_active_loans();
            break;
        case 0:
            printf("Goodbye.\n");
            return EXIT_SUCCESS;
        }
    }
    printf("\nInput closed. Goodbye.\n");
    return EXIT_SUCCESS;
}
