#ifndef RESERVATION_H
#define RESERVATION_H

#include <stdbool.h>

#define MAX_ID_LEN 32

// A node representing a student waiting in line
typedef struct ReservationNode {
    char student_id[MAX_ID_LEN];
    long timestamp;                 // Time request was placed (optional/for tracking)
    struct ReservationNode* next;
} ReservationNode;

// Queue structure for a specific book
typedef struct ReservationQueue {
    char book_id[MAX_ID_LEN];
    ReservationNode* front;
    ReservationNode* rear;
    int count;                      // Total students waiting
    struct ReservationQueue* next;  // Points to next book's queue (Linked list of book queues)
} ReservationQueue;

// --- Function Prototypes ---

// System management
void init_reservation_system(void);
void cleanup_reservation_system(void);

// Core Queue Operations
bool enqueue_reservation(const char* book_id, const char* student_id);
bool dequeue_reservation(const char* book_id, char* assigned_student_id);

// Admin & Cancel Operations
bool cancel_reservation(const char* book_id, const char* student_id);
void display_waiting_list(const char* book_id);
void display_all_reservations(void);
int get_waiting_count(const char* book_id);

// Integration helper: Called when Member 2's return_book() runs
// Returns true if copy was given to a waiting student, false if no one was waiting
bool process_book_return(const char* book_id, char* next_student_id);

// File Persistence
void save_reservations(const char* filename);
void load_reservations(const char* filename);

#endif // RESERVATION_H