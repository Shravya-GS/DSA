#include "complaints.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define COMPLAINT_FILE_HEADER "CAMPUS_COMPLAINTS_V1\n"

static int valid_priority(ComplaintPriority priority)
{
    return priority >= COMPLAINT_CRITICAL && priority <= COMPLAINT_LOW;
}

static int valid_status(ComplaintStatus status)
{
    return status >= COMPLAINT_PENDING && status <= COMPLAINT_RESOLVED;
}

static int copy_field(char *destination, size_t capacity, const char *source)
{
    size_t length;

    if (destination == NULL || source == NULL || capacity == 0) {
        return 0;
    }

    length = strlen(source);
    if (length == 0 || length >= capacity || strchr(source, '|') != NULL ||
        strchr(source, '\n') != NULL || strchr(source, '\r') != NULL) {
        return 0;
    }
    memcpy(destination, source, length + 1);
    return 1;
}

static int grow_store(ComplaintStore *store, size_t needed)
{
    size_t capacity;
    Complaint *items;

    if (needed <= store->capacity) {
        return 1;
    }
    capacity = store->capacity == 0 ? 8 : store->capacity;
    while (capacity < needed) {
        if (capacity > (size_t)-1 / 2) {
            return 0;
        }
        capacity *= 2;
    }
    if (capacity > (size_t)-1 / sizeof(*items)) {
        return 0;
    }
    items = (Complaint *)realloc(store->items, capacity * sizeof(*items));
    if (items == NULL) {
        return 0;
    }
    store->items = items;
    store->capacity = capacity;
    return 1;
}

static int replace_saved_file(const char *temporary, const char *destination)
{
#ifdef _WIN32
    if (!MoveFileExA(
            temporary,
            destination,
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        remove(temporary);
        return 0;
    }
#else
    if (rename(temporary, destination) != 0) {
        remove(temporary);
        return 0;
    }
#endif
    return 1;
}

static int save_store(const ComplaintStore *store)
{
    char temporary[COMPLAINT_STORE_PATH_SIZE + 5];
    FILE *file;
    size_t i;
    int length;
    int failed = 0;

    length = snprintf(temporary, sizeof(temporary), "%s.tmp", store->file_path);
    if (length < 0 || (size_t)length >= sizeof(temporary)) {
        return 0;
    }
    file = fopen(temporary, "w");
    if (file == NULL) {
        return 0;
    }
    if (fputs(COMPLAINT_FILE_HEADER, file) == EOF) {
        failed = 1;
    }
    for (i = 0; !failed && i < store->count; ++i) {
        const Complaint *item = &store->items[i];
        if (fprintf(
                file,
                "%d|%d|%d|%d|%llu|%s|%s\n",
                item->id,
                item->student_id,
                (int)item->priority,
                (int)item->status,
                item->submitted_order,
                item->category,
                item->description) < 0) {
            failed = 1;
        }
    }
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        remove(temporary);
        return 0;
    }
    return replace_saved_file(temporary, store->file_path);
}

static int parse_positive_int(const char *text, int *value)
{
    char *end;
    long parsed;

    if (text == NULL || text[0] < '0' || text[0] > '9') {
        return 0;
    }
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed <= 0 ||
        parsed > INT_MAX) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int parse_record(char *line, Complaint *item)
{
    char *fields[7];
    char *cursor = line;
    char *separator;
    char *end;
    long parsed_priority;
    long parsed_status;
    unsigned long long parsed_order;
    size_t i;

    for (i = 0; i < 6; ++i) {
        separator = strchr(cursor, '|');
        if (separator == NULL) {
            return 0;
        }
        *separator = '\0';
        fields[i] = cursor;
        cursor = separator + 1;
    }
    if (strchr(cursor, '|') != NULL) {
        return 0;
    }
    fields[6] = cursor;

    if (!parse_positive_int(fields[0], &item->id) ||
        !parse_positive_int(fields[1], &item->student_id) ||
        fields[2][0] < '0' || fields[2][0] > '9' ||
        fields[3][0] < '0' || fields[3][0] > '9' ||
        fields[4][0] < '0' || fields[4][0] > '9') {
        return 0;
    }

    errno = 0;
    parsed_priority = strtol(fields[2], &end, 10);
    if (errno == ERANGE || *end != '\0' ||
        parsed_priority < COMPLAINT_CRITICAL ||
        parsed_priority > COMPLAINT_LOW) {
        return 0;
    }
    errno = 0;
    parsed_status = strtol(fields[3], &end, 10);
    if (errno == ERANGE || *end != '\0' ||
        parsed_status < COMPLAINT_PENDING ||
        parsed_status > COMPLAINT_RESOLVED) {
        return 0;
    }
    errno = 0;
    parsed_order = strtoull(fields[4], &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed_order == 0 ||
        parsed_order == ULLONG_MAX) {
        return 0;
    }

    item->priority = (ComplaintPriority)parsed_priority;
    item->status = (ComplaintStatus)parsed_status;
    item->submitted_order = parsed_order;
    return copy_field(item->category, sizeof(item->category), fields[5]) &&
        copy_field(item->description, sizeof(item->description), fields[6]);
}

int complaint_store_open(ComplaintStore *store, const char *file_path)
{
    FILE *file;
    char line[COMPLAINT_CATEGORY_SIZE + COMPLAINT_DESCRIPTION_SIZE + 80];
    size_t path_length;

    if (store == NULL || file_path == NULL) {
        return 0;
    }
    memset(store, 0, sizeof(*store));
    path_length = strlen(file_path);
    if (path_length == 0 || path_length >= sizeof(store->file_path)) {
        return 0;
    }
    memcpy(store->file_path, file_path, path_length + 1);
    store->next_id = 1;
    store->next_order = 1;

    file = fopen(file_path, "r");
    if (file == NULL) {
        return errno == ENOENT;
    }
    if (fgets(line, sizeof(line), file) == NULL ||
        strcmp(line, COMPLAINT_FILE_HEADER) != 0) {
        fclose(file);
        complaint_store_close(store);
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        Complaint item;
        size_t length = strlen(line);
        size_t i;

        if (length == 0 || (line[length - 1] != '\n' && !feof(file))) {
            fclose(file);
            complaint_store_close(store);
            return 0;
        }
        if (line[length - 1] == '\n') {
            line[--length] = '\0';
        }
        if (length > 0 && line[length - 1] == '\r') {
            line[--length] = '\0';
        }
        if (!parse_record(line, &item) || item.id == INT_MAX ||
            !grow_store(store, store->count + 1)) {
            fclose(file);
            complaint_store_close(store);
            return 0;
        }
        for (i = 0; i < store->count; ++i) {
            if (store->items[i].id == item.id ||
                store->items[i].submitted_order == item.submitted_order) {
                fclose(file);
                complaint_store_close(store);
                return 0;
            }
        }
        store->items[store->count++] = item;
        if (item.id >= store->next_id) {
            store->next_id = item.id + 1;
        }
        if (item.submitted_order >= store->next_order) {
            store->next_order = item.submitted_order + 1;
        }
    }
    if (ferror(file)) {
        fclose(file);
        complaint_store_close(store);
        return 0;
    }
    fclose(file);
    return 1;
}

void complaint_store_close(ComplaintStore *store)
{
    if (store != NULL) {
        free(store->items);
        memset(store, 0, sizeof(*store));
    }
}

int complaint_submit(
    ComplaintStore *store,
    int student_id,
    ComplaintPriority priority,
    const char *category,
    const char *description,
    int *new_complaint_id)
{
    Complaint item;

    if (store == NULL || student_id <= 0 || !valid_priority(priority) ||
        store->next_id <= 0 || store->next_id == INT_MAX ||
        store->next_order == ULLONG_MAX || store->count == (size_t)-1) {
        return 0;
    }
    memset(&item, 0, sizeof(item));
    item.id = store->next_id;
    item.student_id = student_id;
    item.priority = priority;
    item.status = COMPLAINT_PENDING;
    item.submitted_order = store->next_order;
    if (!copy_field(item.category, sizeof(item.category), category) ||
        !copy_field(item.description, sizeof(item.description), description) ||
        !grow_store(store, store->count + 1)) {
        return 0;
    }

    store->items[store->count++] = item;
    ++store->next_id;
    ++store->next_order;
    if (!save_store(store)) {
        --store->count;
        --store->next_id;
        --store->next_order;
        return 0;
    }
    if (new_complaint_id != NULL) {
        *new_complaint_id = item.id;
    }
    return 1;
}

const Complaint *complaint_find(const ComplaintStore *store, int complaint_id)
{
    size_t i;

    if (store == NULL || complaint_id <= 0) {
        return NULL;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == complaint_id) {
            return &store->items[i];
        }
    }
    return NULL;
}

int complaint_set_status(
    ComplaintStore *store,
    int complaint_id,
    ComplaintStatus status)
{
    size_t i;
    ComplaintStatus previous;

    if (store == NULL || complaint_id <= 0 || !valid_status(status)) {
        return 0;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == complaint_id) {
            previous = store->items[i].status;
            store->items[i].status = status;
            if (!save_store(store)) {
                store->items[i].status = previous;
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

const Complaint *complaint_next_pending(const ComplaintStore *store)
{
    const Complaint *next = NULL;
    size_t i;

    if (store == NULL) {
        return NULL;
    }
    for (i = 0; i < store->count; ++i) {
        const Complaint *candidate = &store->items[i];
        if (candidate->status != COMPLAINT_PENDING) {
            continue;
        }
        if (next == NULL || candidate->priority < next->priority ||
            (candidate->priority == next->priority &&
             candidate->submitted_order < next->submitted_order)) {
            next = candidate;
        }
    }
    return next;
}

static const char *priority_label(ComplaintPriority priority)
{
    static const char *labels[] = {"Critical", "High", "Medium", "Low"};
    return valid_priority(priority) ? labels[priority] : "Invalid";
}

static const char *status_label(ComplaintStatus status)
{
    static const char *labels[] = {"Pending", "In progress", "Resolved"};
    return valid_status(status) ? labels[status] : "Invalid";
}

void complaint_show(const Complaint *complaint)
{
    if (complaint == NULL) {
        return;
    }
    printf(
        "#%d | Student %d | %s | %s | %s\n  %s\n",
        complaint->id,
        complaint->student_id,
        priority_label(complaint->priority),
        status_label(complaint->status),
        complaint->category,
        complaint->description);
}

void complaint_show_all(const ComplaintStore *store)
{
    size_t i;

    if (store == NULL || store->count == 0) {
        puts("No complaints are recorded.");
        return;
    }
    for (i = 0; i < store->count; ++i) {
        complaint_show(&store->items[i]);
    }
}

void complaint_show_pending(const ComplaintStore *store)
{
    size_t i;
    int found = 0;

    if (store == NULL) {
        return;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].status == COMPLAINT_PENDING) {
            complaint_show(&store->items[i]);
            found = 1;
        }
    }
    if (!found) {
        puts("No pending complaints.");
    }
}

void complaint_show_student(const ComplaintStore *store, int student_id)
{
    size_t i;
    int found = 0;

    if (store == NULL || student_id <= 0) {
        return;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].student_id == student_id) {
            complaint_show(&store->items[i]);
            found = 1;
        }
    }
    if (!found) {
        puts("No complaints found for this student.");
    }
}

static int read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;
    int character;

    printf("%s", prompt);
    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return 0;
    }
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else {
        character = getchar();
        if (character != '\n' && character != EOF) {
            while (character != '\n' && character != EOF) {
                character = getchar();
            }
            puts("Input is too long; try again.");
            return -1;
        }
    }
    if (length > 0 && buffer[length - 1] == '\r') {
        buffer[--length] = '\0';
    }
    return 1;
}

static int read_number(const char *prompt, int *value)
{
    char buffer[64];
    char *end;
    long parsed;
    int result = read_line(prompt, buffer, sizeof(buffer));

    if (result != 1) {
        return result;
    }
    errno = 0;
    parsed = strtol(buffer, &end, 10);
    if (end == buffer || errno == ERANGE || parsed < INT_MIN ||
        parsed > INT_MAX) {
        puts("Enter a valid whole number.");
        return -1;
    }
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    if (*end != '\0') {
        puts("Enter a valid whole number.");
        return -1;
    }
    *value = (int)parsed;
    return 1;
}

static int choose_priority(ComplaintPriority *priority)
{
    int choice;
    int result = read_number(
        "Priority (1 Critical, 2 High, 3 Medium, 4 Low): ",
        &choice);

    if (result != 1) {
        return result;
    }
    if (choice < 1 || choice > 4) {
        puts("Invalid priority.");
        return -1;
    }
    *priority = (ComplaintPriority)(choice - 1);
    return 1;
}

static int choose_status(ComplaintStatus *status)
{
    int choice;
    int result = read_number(
        "Status (1 Pending, 2 In progress, 3 Resolved): ",
        &choice);

    if (result != 1) {
        return result;
    }
    if (choice < 1 || choice > 3) {
        puts("Invalid status.");
        return -1;
    }
    *status = (ComplaintStatus)(choice - 1);
    return 1;
}

static void submit_from_menu(ComplaintStore *store, int student_id)
{
    ComplaintPriority priority;
    char category[COMPLAINT_CATEGORY_SIZE];
    char description[COMPLAINT_DESCRIPTION_SIZE];
    int complaint_id;

    if (choose_priority(&priority) != 1 ||
        read_line("Facility/category: ", category, sizeof(category)) != 1 ||
        read_line("Description: ", description, sizeof(description)) != 1) {
        return;
    }
    if (!complaint_submit(
            store,
            student_id,
            priority,
            category,
            description,
            &complaint_id)) {
        puts("Complaint was not saved. Check the fields and storage.");
        return;
    }
    printf("Complaint #%d submitted.\n", complaint_id);
}

void complaint_student_menu(
    ComplaintStore *store,
    int student_id,
    ComplaintStudentExists student_exists,
    void *student_context)
{
    int choice;

    if (store == NULL || student_id <= 0 || student_exists == NULL ||
        !student_exists(student_id, student_context)) {
        puts("A registered student's ID is required.");
        return;
    }
    for (;;) {
        int result;
        puts("\n--- Student complaints ---");
        puts("1. Submit complaint");
        puts("2. Check my complaints");
        puts("0. Return");
        result = read_number("Choice: ", &choice);
        if (result == 0 || (result == 1 && choice == 0)) {
            return;
        }
        if (result != 1) {
            continue;
        }
        if (choice == 1) {
            submit_from_menu(store, student_id);
        } else if (choice == 2) {
            complaint_show_student(store, student_id);
        } else {
            puts("Invalid menu choice.");
        }
    }
}

void complaint_admin_menu(ComplaintStore *store)
{
    int choice;

    if (store == NULL) {
        puts("Complaint storage is unavailable.");
        return;
    }
    for (;;) {
        int result;
        puts("\n--- Administrator complaints ---");
        puts("1. View all complaints");
        puts("2. View pending complaints");
        puts("3. Select next complaint by priority");
        puts("4. Update complaint status");
        puts("0. Return");
        result = read_number("Choice: ", &choice);
        if (result == 0 || (result == 1 && choice == 0)) {
            return;
        }
        if (result != 1) {
            continue;
        }
        if (choice == 1) {
            complaint_show_all(store);
        } else if (choice == 2) {
            complaint_show_pending(store);
        } else if (choice == 3) {
            const Complaint *next = complaint_next_pending(store);
            if (next == NULL) {
                puts("No pending complaints.");
            } else {
                puts("Next complaint to handle:");
                complaint_show(next);
            }
        } else if (choice == 4) {
            int complaint_id;
            ComplaintStatus status;
            const Complaint *item;

            if (read_number("Complaint ID: ", &complaint_id) != 1) {
                continue;
            }
            item = complaint_find(store, complaint_id);
            if (item == NULL) {
                puts("Complaint ID not found.");
                continue;
            }
            complaint_show(item);
            if (choose_status(&status) != 1) {
                continue;
            }
            if (!complaint_set_status(store, complaint_id, status)) {
                puts("Could not save the status update.");
            } else {
                puts("Complaint status updated.");
            }
        } else {
            puts("Invalid menu choice.");
        }
    }
}
