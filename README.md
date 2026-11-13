# Library Management System — Book Reservation & Waiting List

> **Data Structures & Algorithms Course Project**  
> **Role / Module:** Book Reservation and Waiting List Management  
> **Developer:** Shreya (`shreya` branch)

---

## 📌 Role & Objectives

This subsystem implements the **Book Reservation & Waiting List** module for the Library Management System. When physical book copies are limited or completely checked out, this module ensures fair, sequential book allocation and gives administrators complete visibility and control over pending requests.

### Core Requirements Implemented:
1. **Zero-Copy Reservation:** When all copies of a book are issued, students can place a reservation for that book.
2. **Order Preservation (FIFO):** Maintains the exact arrival sequence of reservation requests using a First-In-First-Out (FIFO) queue.
3. **Auto-Allocation on Return:** When a borrowed book is returned, the system automatically offers it to the next eligible student in the waiting list and updates the records.
4. **Admin Queue Management:** Allows administrators to view the waiting list for any selected book and cancel or remove invalid/withdrawn requests.
5. **Data Persistence:** Saves and reloads waiting lists to `reservations.txt` so reservation queues persist across restarts.

---

## 🏛️ Data Structures & Design

Each book with pending reservations has an independent **FIFO Queue** represented as a dynamically allocated singly linked list with dedicated `front` and `rear` pointers. All active book queues are linked together in a master list.

```text
[Master Queue List]
       │
       ├──► [Book: BK-101] ──► Front: [Student: STU-001] ──► [Student: STU-002] ──► Rear: [Student: STU-003]
       │
       └──► [Book: BK-202] ──► Front: [Student: STU-004] ──► Rear: [Student: STU-004]
```

### Struct Declarations (`include/reservation.h`):

```c
#define MAX_ID_LEN 32

// A node representing a student waiting in line
typedef struct ReservationNode {
    char student_id[MAX_ID_LEN];
    long timestamp;                 // Time request was placed
    struct ReservationNode* next;
} ReservationNode;

// Queue structure for a specific book
typedef struct ReservationQueue {
    char book_id[MAX_ID_LEN];
    ReservationNode* front;         // O(1) dequeue / auto-offer
    ReservationNode* rear;          // O(1) enqueue
    int count;                      // Total students waiting
    struct ReservationQueue* next;  // Points to next book's queue
} ReservationQueue;
```

---

## 📂 Project Structure

```text
DSA/
├── README.md               # Documentation and execution guide
├── .gitignore              # Ignores build artifacts (.exe, .o) and runtime logs
├── include/
│   └── reservation.h       # Function prototypes and queue struct definitions
├── src/
│   └── reservation.c       # Queue implementation, auto-offer logic & interactive menu
├── tests/
│   └── test_reservation.c  # Unit test driver verifying all 4 requirements
└── data/                   # Directory for persistent data files (reservations.txt)
```

---

## ⚙️ How to Compile and Run

Make sure you are in the `DSA` directory:
```powershell
cd DSA
```

### 1. Interactive Menu Mode (`reservation.c`)
Run `reservation.c` directly to interactively add reservations, view queues, process returns, or cancel requests:

```powershell
# Compile
gcc src/reservation.c -o reservation.exe

# Run
.\reservation.exe
```

**Interactive Menu Interface:**
```text
============================================
    BOOK RESERVATION MANAGEMENT SYSTEM      
============================================

--- MENU ---
1. Reserve a Book (Add to Waiting List)
2. View Waiting List for a Book
3. View All Campus Reservations
4. Cancel a Reservation
5. Process Book Return (Auto-Offer to Next)
6. Save & Exit
--------------------------------------------
Enter your choice (1-6):
```

---

### 2. Automated Test Suite (`test_reservation.c`)
Run the automated test driver to verify all 4 specifications with sample data:

```powershell
# Compile with library flag to run the test driver's main()
gcc -DRESERVATION_LIB src/reservation.c tests/test_reservation.c -o test_reservation.exe

# Run test suite
.\test_reservation.exe
```

**Test Suite Execution Output:**
```text
=== TEST 1: Placing Reservations (FIFO) ===
[Success] Student 'STU-001' added to waiting list for Book 'BK-101' (Position: 1).
[Success] Student 'STU-002' added to waiting list for Book 'BK-101' (Position: 2).
[Success] Student 'STU-003' added to waiting list for Book 'BK-101' (Position: 3).
[Success] Student 'STU-004' added to waiting list for Book 'BK-202' (Position: 1).

=== TEST 2: Display Waiting List ===
--- Waiting List for Book ID: BK-101 (Total: 3) ---
Pos   | Student ID      | Queue Time          
--------------------------------------------
1     | STU-001         | 2026-11-13 19:43    
2     | STU-002         | 2026-11-13 19:43    
3     | STU-003         | 2026-11-13 19:43    

=== TEST 3: Admin Cancels a Middle Student ===
[Success] Cancelled reservation of student 'STU-002' for book 'BK-101'.

--- Waiting List for Book ID: BK-101 (Total: 2) ---
Pos   | Student ID      | Queue Time          
--------------------------------------------
1     | STU-001         | 2026-11-13 19:43    
2     | STU-003         | 2026-11-13 19:43    

=== TEST 4: Book Returned (Trigger Auto-Offer) ===
======================================================
[AUTO-OFFER] Book 'BK-101' returned!
Offered to next student in waiting list: STU-001
======================================================
Application successfully handed book to: STU-001

--- Waiting List for Book ID: BK-101 (Total: 1) ---
Pos   | Student ID      | Queue Time          
--------------------------------------------
1     | STU-003         | 2026-11-13 19:43    
```

---

## 🛠️ Module API Reference

| Function | Requirement Addressed | Description |
| :--- | :--- | :--- |
| `enqueue_reservation(book_id, student_id)` | Requirement 1 & 2 | Checks for duplicates, creates a node, and appends the student to the rear of the book's queue in $O(1)$. |
| `dequeue_reservation(book_id, assigned_id)` | Requirement 3 | Removes and returns the first student from the front of the queue in $O(1)$. |
| `process_book_return(book_id, next_id)` | Requirement 3 | Triggered on book return; automatically allocates the returned copy to the next waiting student. |
| `display_waiting_list(book_id)` | Requirement 4 | Formats and prints the waiting list with position, student ID, and formatted queue timestamp. |
| `cancel_reservation(book_id, student_id)` | Requirement 4 | Allows admin to cancel an arbitrary student's reservation from anywhere in the queue in $O(k)$. |
| `save_reservations(filename)` | Persistence | Writes all pending queues to disk in CSV format (`book_id,student_id,timestamp`). |
| `load_reservations(filename)` | Persistence | Reconstructs the queues into memory upon startup. |

---




