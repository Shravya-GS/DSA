# Topic 4 Design Report: Complaint Registration and Processing

## Problem and scope

This module lets registered students submit complaints about campus facilities
and lets administrators review, prioritize, and update them. Each complaint is
associated with the student's ID. Student registration and administrator
authentication remain the responsibility of the host application.

## Assumptions

- Student IDs are positive integers and are validated by the host application's
  student registry before a student enters the complaint menu.
- Priority order is Critical, High, Medium, Low.
- Equal-priority pending complaints are handled in submission order.
- Status values are Pending, In progress, and Resolved.
- Category and description are nonempty bounded text. The pipe delimiter and
  line breaks are rejected so records can be safely stored as text.
- Resolved complaints are retained for history.

## Data representation

`Complaint` stores an ID, student ID, priority, status, submission sequence,
category, and description. `ComplaintStore` owns a dynamic array and tracks its
capacity and the next identifiers. Capacity doubles when needed, providing
amortized constant-time append without choosing a fixed maximum record count.

The queue is implemented by scanning pending records and choosing the
smallest priority value, then the earliest submission sequence on ties. This
keeps fairness explicit and avoids maintaining a second heap/index that could
become inconsistent with status updates. A versioned text file preserves the
records between runs. Successful changes are written to a temporary file
before replacing the saved file.

## Pseudocode

### Submit

```text
validate registered student, priority, category, and description
assign the next complaint ID and submission sequence
set status to Pending and append to the dynamic array
write the complete store to a temporary file
if saving fails, undo the append and report failure
otherwise replace the saved file and return the new ID
```

### Select next pending

```text
best = none
for each complaint:
    skip it unless status is Pending
    choose it if its priority outranks best
    on equal priority, choose it only if it was submitted earlier
return best
```

### Update status

```text
find the complaint by ID and validate the new status
save the old status and assign the new status
write the complete store
if saving fails, restore the old status
```

## Complexity

Let `n` be the number of saved complaints.

| Operation | Time | Extra space |
|---|---:|---:|
| Append in memory | Amortized O(1) | O(1) amortized |
| Find by complaint ID | O(n) | O(1) |
| List complaints for a student | O(n) | O(1) |
| Select next pending | O(n) | O(1) |
| Update status | O(n) lookup + O(n) file write | O(1) |
| Load and validate file | O(n^2) | O(n) |
| Store records | O(n) | O(n) |

Load validation checks duplicate IDs and submission sequence numbers with
linear scans, resulting in O(n^2) worst-case loading. For a larger system,
those checks and complaint lookup could use hash indexes or a database.

## Testing

`complaints_test.c` checks fresh-store initialization, priority ordering,
first-in-first-out handling for equal priority, exclusion of in-progress and
resolved complaints from next-pending selection, persistence after reopening,
input validation, invalid status/ID rejection, and corrupted-file rejection.

Built with GCC 16.2.0 (MinGW-w64, x86-64) using C11, `-Wall -Wextra
-Wpedantic -Werror`. The test executable printed:

```text
All fresh Topic 4 tests passed.
```

The demo was also exercised end-to-end: a student submitted a complaint, the
administrator resolved it, and the student saw the resolved status after the
program was restarted.

## Performance study

`complaints_benchmark.c` measures lookup of the final record and selection of
the next pending complaint using synthetic stores with 100, 1,000, and 10,000
records. The benchmark was built with GCC 16.2.0 (MinGW-w64, x86-64), `-O2`,
on Windows. Values below are medians of five runs in microseconds per
operation. The benchmark uses `clock()`, so short timings are approximate and
may vary with timer resolution and system load. It measures in-memory
operations, not file I/O.

| Records | Lookup (µs/op) | Next pending (µs/op) |
|---:|---:|---:|
| 100 | 0.040 | 0.120 |
| 1,000 | 0.400 | 0.800 |
| 10,000 | 3.000 | 9.000 |

Both operations scan the dynamic array, and the measurements generally grow
with the record count, as expected for O(n) operations. Re-run the benchmark on
the presentation machine for hardware-specific comparisons.

## Integration

`ComplaintStudentExists` is the adapter boundary to the shared student module.
The host application should pass the authenticated student ID and implement
the callback using the real student registry. The demo uses sample IDs only.
Administrator authentication belongs to the host application and must happen
before calling the administrator menu.
