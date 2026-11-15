# Topic 4: Complaint Registration and Processing

This is a fresh, standalone C implementation of complaint registration and
processing for CampusAssist.

## Features

- Students submit facility complaints with a category, description, and
  Critical, High, Medium, or Low priority.
- The student menu validates the student's ID through a callback, allowing
  the host application to connect its own student registry.
- Administrators view all or pending complaints, select the next pending item,
  and set its status to Pending, In progress, or Resolved.
- Priority selection is Critical-to-Low; complaints at equal priority are
  selected in their original submission order.
- Complaint records and status changes persist in `complaints.dat`.

The demo has sample student IDs `1001`, `1002`, and `1003`. Its administrator
menu is not an authentication system; the host application must authenticate
administrators before calling it.

## Build and run

Use GCC or Clang with C11 support:

```text
gcc -std=c11 -Wall -Wextra -Wpedantic complaints.c complaints_demo.c -o complaints_demo
./complaints_demo student 1001
./complaints_demo admin
```

On Windows, run `complaints_demo.exe student 1001` or
`complaints_demo.exe admin`. The data file is created in the program's current
working directory.

## Tests and benchmark

```text
gcc -std=c11 -Wall -Wextra -Wpedantic complaints.c complaints_test.c -o complaints_test
./complaints_test

gcc -O2 -std=c11 -Wall -Wextra -Wpedantic complaints.c complaints_benchmark.c -o complaints_benchmark
./complaints_benchmark
```

## Connecting to the group application

Open one `ComplaintStore` at application startup with
`complaint_store_open(&store, "complaints.dat")` and close it with
`complaint_store_close(&store)` at shutdown. After a student is authenticated,
call `complaint_student_menu` with that student's ID and an adapter that checks
the shared student registry. Call `complaint_admin_menu` only after
administrator authentication succeeds. Compile `complaints.c` with the
application; do not compile `complaints_demo.c` into the application's own
executable because it defines a separate `main`.

## Data structure and complexity

Complaints are stored in a geometrically growing dynamic array. Appending is
amortized O(1) in memory; finding a complaint or selecting the next pending
complaint is O(n). Selection scans for the best priority and oldest submission
order. Sorted reports are not required for this queue operation. Saving
rewrites the versioned file, so a successful submission or status update takes
O(n) file I/O. The store uses O(n) memory.
