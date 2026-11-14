# CampusAssist - Student Management Module

## What this module does

This standalone C module registers, finds, updates, removes and displays students.
It can sort the report by name, department or GPA and saves records to
`students.txt` on exit (or using **Save now**).

## Build and run

Open a terminal in this folder:

```text
gcc -std=c99 -Wall -Wextra -pedantic student_management.c -o student_management
.\student_management.exe
```

With Clang, replace `gcc` with `clang`. The data file is created in the current
working directory. Keep it alongside your run location to retain records.

To run the built-in performance test (it creates synthetic records and does not
read or change `students.txt`):

```text
gcc -std=c99 -O2 -Wall -Wextra -pedantic student_management.c -o student_management
.\student_management.exe --benchmark
```

## Data structure and complexity

Each student is stored in a node of a separate-chaining hash table, indexed by
the unique student ID. The table doubles in size and rehashes records when its
load exceeds 0.75. This makes ID lookup and registration average **O(1)** over
time (insertion can take **O(n)** during a resize) and **O(n)** in the worst
case if many IDs collide.
Displaying all students takes **O(n)** time. A sorted report copies pointers to
an array and uses `qsort`, taking **O(n log n)** time and **O(n)** extra space.
The hash table and student nodes use **O(n)** space.

The table starts with 101 buckets and grows as needed to keep lookup practical
as the student count increases.

## Assumptions

- Student IDs are unique and are not changed during an update.
- Academic year is 1-8; GPA is on a 0-10 scale.
- Fields cannot contain `|`, the separator used in `students.txt`.
- Removal is immediate. The integrated group application should check for
  active loans/complaints before removing a student.
- The student lookup function is currently internal to this standalone program.
  When integrating, move the `Student` type and operations into a shared
  `student.h`/`student.c` interface and let other modules validate IDs through
  that interface.

## Operation outline

- **Register:** collect and validate fields; reject a duplicate ID; hash the ID
  and insert a node in that bucket.
- **Find/update:** hash the requested ID, scan that bucket, then display or
  replace the record's non-ID fields.
- **Remove:** locate the matching node in its bucket, unlink it and free it.
- **Sorted report:** collect record pointers, sort with the selected comparator,
  and print without changing the hash table.
- **Persistence:** load valid delimiter-separated records at startup and write
  the current records to `students.txt` when requested or exiting.

## Quick manual test

1. Register ID `S001`, name `Asha Rao`, department `CSE`, year `2`, GPA `8.5`.
2. Find `S001`; confirm its fields are shown. Search for `S999`; confirm
   “Student not found.”
3. Try registering `S001` again; confirm the duplicate is rejected.
4. Register another student and display sorted by name and GPA.
5. Update and remove a record, then exit and restart; confirm remaining data
   loads from `students.txt`.

## Performance study

The benchmark performs 500,000 successful ID lookups at each record count and
reports the average time for sorting student pointers by name. Sorting time
excludes assembling the pointer array. The following are the mean of three
benchmark runs on the development machine, compiled with GCC using `-O2`:

| Students | 500,000 ID lookups (ms) | Name sort (ms per sort) |
|---:|---:|---:|
| 100 | 16.3 | 0.162 |
| 1,000 | 18.7 | 2.040 |
| 10,000 | 39.7 | 30.733 |

Lookup grows more slowly than the number of records because the hash table
expands above a 0.75 load factor, though collisions and machine timing mean it
is not perfectly constant. Sorting grows substantially with input size, in
line with the expected **O(n log n)** trend. These timings are machine- and
compiler-dependent; rerun the included benchmark on the presentation machine
and replace the table if its measurements differ.
