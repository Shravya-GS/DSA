#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/reservation.h"

// Head of the linked list of all book reservation queues
static ReservationQueue* queue_list_head = NULL;

void init_reservation_system(void) {
    queue_list_head = NULL;
}

// Internal helper: Find or optionally create a queue for a book
static ReservationQueue* get_or_create_queue(const char* book_id, bool create_if_missing) {
    ReservationQueue* curr = queue_list_head;
    while (curr != NULL) {
        if (strcmp(curr->book_id, book_id) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    if (!create_if_missing) return NULL;

    ReservationQueue* new_q = (ReservationQueue*)malloc(sizeof(ReservationQueue));
    if (!new_q) {
        printf("[Error] Memory allocation failed for reservation queue.\n");
        return NULL;
    }
    strncpy(new_q->book_id, book_id, MAX_ID_LEN - 1);
    new_q->book_id[MAX_ID_LEN - 1] = '\0';
    new_q->front = NULL;
    new_q->rear = NULL;
    new_q->count = 0;
    new_q->next = queue_list_head;
    queue_list_head = new_q;
    return new_q;
}

// 1. Enqueue: Place a student at the end of the line (FIFO)
bool enqueue_reservation(const char* book_id, const char* student_id) {
    if (!book_id || !student_id) return false;

    ReservationQueue* q = get_or_create_queue(book_id, true);
    if (!q) return false;

    // Check if this student has already reserved this book (avoid duplicates)
    ReservationNode* check = q->front;
    while (check != NULL) {
        if (strcmp(check->student_id, student_id) == 0) {
            printf("[Notice] Student '%s' already has an active reservation for book '%s'.\n", 
                   student_id, book_id);
            return false;
        }
        check = check->next;
    }

    // Create new node
    ReservationNode* new_node = (ReservationNode*)malloc(sizeof(ReservationNode));
    if (!new_node) {
        printf("[Error] Out of memory.\n");
        return false;
    }
    strncpy(new_node->student_id, student_id, MAX_ID_LEN - 1);
    new_node->student_id[MAX_ID_LEN - 1] = '\0';
    new_node->timestamp = (long)time(NULL);
    new_node->next = NULL;

    // Enqueue at rear - O(1)
    if (q->rear == NULL) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
    q->count++;

    printf("[Success] Student '%s' added to waiting list for Book '%s' (Position: %d).\n", 
           student_id, book_id, q->count);
    return true;
}

// 2. Dequeue: Offer to next student in line - O(1)
bool dequeue_reservation(const char* book_id, char* assigned_student_id) {
    ReservationQueue* q = get_or_create_queue(book_id, false);
    if (!q || q->front == NULL) {
        return false; // Queue is empty
    }

    ReservationNode* temp = q->front;
    if (assigned_student_id) {
        strncpy(assigned_student_id, temp->student_id, MAX_ID_LEN);
    }

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL; // Queue is now empty
    }
    q->count--;
    free(temp);

    return true;
}

// 3. Process Book Return: Called automatically when a book is returned
bool process_book_return(const char* book_id, char* next_student_id) {
    char student[MAX_ID_LEN];
    if (dequeue_reservation(book_id, student)) {
        if (next_student_id) {
            strncpy(next_student_id, student, MAX_ID_LEN);
        }
        printf("\n======================================================\n");
        printf("[AUTO-OFFER] Book '%s' returned!\n", book_id);
        printf("Offered to next student in waiting list: %s\n", student);
        printf("======================================================\n");
        return true;
    }
    return false; // No pending reservations
}

// 4. Cancel or remove invalid request from anywhere in the queue - O(k)
bool cancel_reservation(const char* book_id, const char* student_id) {
    ReservationQueue* q = get_or_create_queue(book_id, false);
    if (!q || q->front == NULL) {
        printf("[Info] No reservations found for book '%s'.\n", book_id);
        return false;
    }

    ReservationNode* curr = q->front;
    ReservationNode* prev = NULL;

    while (curr != NULL) {
        if (strcmp(curr->student_id, student_id) == 0) {
            if (prev == NULL) {
                // Deleting the front node
                q->front = curr->next;
                if (q->front == NULL) q->rear = NULL;
            } else {
                prev->next = curr->next;
                if (curr == q->rear) {
                    q->rear = prev; // Updated rear
                }
            }
            free(curr);
            q->count--;
            printf("[Success] Cancelled reservation of student '%s' for book '%s'.\n", student_id, book_id);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }

    printf("[Not Found] Student '%s' not found in waiting list for book '%s'.\n", student_id, book_id);
    return false;
}

// 5. Display Waiting List for a selected book
void display_waiting_list(const char* book_id) {
    ReservationQueue* q = get_or_create_queue(book_id, false);
    if (!q || q->front == NULL) {
        printf("\n[Waiting List] No pending reservations for book ID: %s\n", book_id);
        return;
    }

    printf("\n--- Waiting List for Book ID: %s (Total: %d) ---\n", book_id, q->count);
    printf("%-5s | %-15s | %-20s\n", "Pos", "Student ID", "Queue Time");
    printf("--------------------------------------------\n");

    ReservationNode* curr = q->front;
    int pos = 1;
    while (curr != NULL) {
        char time_str[30];
        time_t t = (time_t)curr->timestamp;
        struct tm* tm_info = localtime(&t);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);

        printf("%-5d | %-15s | %-20s\n", pos++, curr->student_id, time_str);
        curr = curr->next;
    }
}

// Display all active queues across campus
void display_all_reservations(void) {
    ReservationQueue* curr = queue_list_head;
    bool found_any = false;
    while (curr != NULL) {
        if (curr->count > 0) {
            display_waiting_list(curr->book_id);
            found_any = true;
        }
        curr = curr->next;
    }
    if (!found_any) {
        printf("\n[Info] No active book reservations campus-wide.\n");
    }
}

int get_waiting_count(const char* book_id) {
    ReservationQueue* q = get_or_create_queue(book_id, false);
    return q ? q->count : 0;
}

// 6. Persistence: Save & Load
void save_reservations(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("[Error] Cannot open %s for saving reservations.\n", filename);
        return;
    }

    ReservationQueue* q = queue_list_head;
    while (q != NULL) {
        ReservationNode* node = q->front;
        while (node != NULL) {
            // Format: book_id,student_id,timestamp
            fprintf(fp, "%s,%s,%ld\n", q->book_id, node->student_id, node->timestamp);
            node = node->next;
        }
        q = q->next;
    }
    fclose(fp);
}

void load_reservations(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return; // File doesn't exist yet on first run

    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        char book_id[MAX_ID_LEN], student_id[MAX_ID_LEN];
        long timestamp = 0;

        char* token = strtok(line, ",");
        if (token) strncpy(book_id, token, MAX_ID_LEN);

        token = strtok(NULL, ",");
        if (token) strncpy(student_id, token, MAX_ID_LEN);

        token = strtok(NULL, ",");
        if (token) timestamp = atol(token);

        if (strlen(book_id) > 0 && strlen(student_id) > 0) {
            enqueue_reservation(book_id, student_id);
        }
    }
    fclose(fp);
}

// Free memory on application shutdown
void cleanup_reservation_system(void) {
    ReservationQueue* q = queue_list_head;
    while (q != NULL) {
        ReservationNode* node = q->front;
        while (node != NULL) {
            ReservationNode* temp = node;
            node = node->next;
            free(temp);
        }
        ReservationQueue* temp_q = q;
        q = q->next;
        free(temp_q);
    }
    queue_list_head = NULL;
}

#ifndef RESERVATION_LIB
// --- Standalone Interactive Console Runner for reservation.c ---
static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(void) {
    init_reservation_system();
    load_reservations("reservations.txt");

    int choice = 0;
    char book_id[MAX_ID_LEN];
    char student_id[MAX_ID_LEN];
    char next_student[MAX_ID_LEN];

    printf("\n============================================\n");
    printf("    BOOK RESERVATION MANAGEMENT SYSTEM      \n");
    printf("============================================\n");

    while (1) {
        printf("\n--- MENU ---\n");
        printf("1. Reserve a Book (Add to Waiting List)\n");
        printf("2. View Waiting List for a Book\n");
        printf("3. View All Campus Reservations\n");
        printf("4. Cancel a Reservation\n");
        printf("5. Process Book Return (Auto-Offer to Next)\n");
        printf("6. Save & Exit\n");
        printf("--------------------------------------------\n");
        printf("Enter your choice (1-6): ");

        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("[Error] Invalid input. Please enter a number between 1 and 6.\n");
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1:
                printf("Enter Book ID: ");
                if (fgets(book_id, sizeof(book_id), stdin)) {
                    book_id[strcspn(book_id, "\r\n")] = '\0';
                }
                printf("Enter Student ID: ");
                if (fgets(student_id, sizeof(student_id), stdin)) {
                    student_id[strcspn(student_id, "\r\n")] = '\0';
                }
                if (strlen(book_id) > 0 && strlen(student_id) > 0) {
                    enqueue_reservation(book_id, student_id);
                } else {
                    printf("[Error] Book ID and Student ID cannot be empty.\n");
                }
                break;

            case 2:
                printf("Enter Book ID to view waiting list: ");
                if (fgets(book_id, sizeof(book_id), stdin)) {
                    book_id[strcspn(book_id, "\r\n")] = '\0';
                }
                display_waiting_list(book_id);
                break;

            case 3:
                display_all_reservations();
                break;

            case 4:
                printf("Enter Book ID: ");
                if (fgets(book_id, sizeof(book_id), stdin)) {
                    book_id[strcspn(book_id, "\r\n")] = '\0';
                }
                printf("Enter Student ID to cancel: ");
                if (fgets(student_id, sizeof(student_id), stdin)) {
                    student_id[strcspn(student_id, "\r\n")] = '\0';
                }
                cancel_reservation(book_id, student_id);
                break;

            case 5:
                printf("Enter Returned Book ID: ");
                if (fgets(book_id, sizeof(book_id), stdin)) {
                    book_id[strcspn(book_id, "\r\n")] = '\0';
                }
                if (!process_book_return(book_id, next_student)) {
                    printf("[Info] No pending reservations for book '%s'.\n", book_id);
                }
                break;

            case 6:
                save_reservations("reservations.txt");
                printf("[Info] Reservations saved to reservations.txt.\n");
                cleanup_reservation_system();
                printf("Exiting... Goodbye!\n");
                return 0;

            default:
                printf("[Error] Invalid choice! Please select 1 to 6.\n");
                break;
        }
    }

    return 0;
}
#endif
