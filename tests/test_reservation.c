#include <stdio.h>
#include "../include/reservation.h"

int main() {
    init_reservation_system();

    printf("=== TEST 1: Placing Reservations (FIFO) ===\n");
    enqueue_reservation("BK-101", "STU-001");
    enqueue_reservation("BK-101", "STU-002");
    enqueue_reservation("BK-101", "STU-003");
    enqueue_reservation("BK-202", "STU-004");

    printf("\n=== TEST 2: Display Waiting List ===\n");
    display_waiting_list("BK-101");

    printf("\n=== TEST 3: Admin Cancels a Middle Student ===\n");
    cancel_reservation("BK-101", "STU-002");
    display_waiting_list("BK-101");

    printf("\n=== TEST 4: Book Returned (Trigger Auto-Offer) ===\n");
    char next_stu[32];
    if (process_book_return("BK-101", next_stu)) {
        printf("Application successfully handed book to: %s\n", next_stu);
    }

    display_waiting_list("BK-101");

    cleanup_reservation_system();
    return 0;
}