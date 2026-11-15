#ifndef TOPIC4_COMPLAINTS_H
#define TOPIC4_COMPLAINTS_H

#include <stddef.h>

#define COMPLAINT_CATEGORY_SIZE 48
#define COMPLAINT_DESCRIPTION_SIZE 240
#define COMPLAINT_STORE_PATH_SIZE 260

typedef enum {
    COMPLAINT_CRITICAL = 0,
    COMPLAINT_HIGH,
    COMPLAINT_MEDIUM,
    COMPLAINT_LOW
} ComplaintPriority;

typedef enum {
    COMPLAINT_PENDING = 0,
    COMPLAINT_IN_PROGRESS,
    COMPLAINT_RESOLVED
} ComplaintStatus;

typedef struct {
    int id;
    int student_id;
    ComplaintPriority priority;
    ComplaintStatus status;
    unsigned long long submitted_order;
    char category[COMPLAINT_CATEGORY_SIZE];
    char description[COMPLAINT_DESCRIPTION_SIZE];
} Complaint;

typedef struct {
    Complaint *items;
    size_t count;
    size_t capacity;
    int next_id;
    unsigned long long next_order;
    char file_path[COMPLAINT_STORE_PATH_SIZE];
} ComplaintStore;

typedef int (*ComplaintStudentExists)(int student_id, void *context);

int complaint_store_open(ComplaintStore *store, const char *file_path);
void complaint_store_close(ComplaintStore *store);

int complaint_submit(
    ComplaintStore *store,
    int student_id,
    ComplaintPriority priority,
    const char *category,
    const char *description,
    int *new_complaint_id
);
int complaint_set_status(
    ComplaintStore *store,
    int complaint_id,
    ComplaintStatus status
);
const Complaint *complaint_find(
    const ComplaintStore *store,
    int complaint_id
);
const Complaint *complaint_next_pending(const ComplaintStore *store);

void complaint_show(const Complaint *complaint);
void complaint_show_all(const ComplaintStore *store);
void complaint_show_pending(const ComplaintStore *store);
void complaint_show_student(const ComplaintStore *store, int student_id);

void complaint_student_menu(
    ComplaintStore *store,
    int student_id,
    ComplaintStudentExists student_exists,
    void *student_context
);
void complaint_admin_menu(ComplaintStore *store);

#endif
