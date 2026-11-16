# Topic 5: Reports and Application Records

## Problem and scope

Topic 5 provides an application-level view of saved records from the student,
library, reservation, and complaint topics. The reports program reads the
modules' persistent files and displays a dashboard and sorted reports. It is
read-only: the owning modules remain responsible for creating, updating, and
saving their records.

The project modules currently keep their records in separate files. The
reports program therefore accepts file paths rather than changing the
modules' data structures or writing duplicate records. The reservation file
format is specified below and must match the format used by the reservation
module.

## Assumptions and file formats

- The reports executable is run from the project root by default.
- Missing files are reported as unavailable, not represented as empty data.
- Malformed files or inconsistent active book loans stop report generation
  with an error message.
- Saved student, book, loan, and complaint formats follow their owning
  modules. The reservation report uses:
  - Header: `CAMPUS_RESERVATIONS_V1`
  - Record: `reservation ID|book ID|student ID|queue order|status`
  - Status: `WAITING`, `OFFERED`, `CANCELLED`, or `FULFILLED`
- The reservation module must use this format, or its report parser must be
  updated to match the module's actual format.
- Book availability is calculated from total copies minus active loans.
  Returned loans remain in the loan history and are not counted as issued.
- Student and library IDs are displayed as saved; complaint records use the
  integer student IDs defined by the complaints module.

## Design and data representation

`reports.c` loads each file into a dynamically growing array of its record
type. A record is parsed and validated before it is appended. It checks the
library invariant that active loans refer to existing books and do not exceed
the book's total copies. The program does not save these arrays: the source
files remain the persistent application records.

The menu provides:

1. A summary of registered students, book titles and copies, issued and
   available copies, waiting reservations, and complaint counts by status and
   priority.
2. Student reports sorted by name or GPA (highest first).
3. A book catalog sorted by title with total and available copies.
4. Waiting reservations sorted by book ID and queue order.
5. Complaint reports sorted by priority and submission order, or by status.

Sorted views use `qsort` on the in-memory arrays. They do not change the order
of records in the saved files.

## Pseudocode

### Load and validate reports

```text
for each configured data file:
    open the file and verify its version header, if it has one
    parse each record
    reject malformed records and report the file and error
    append each valid record to its dynamic array
if both book and loan files are available:
    reject active loans that refer to missing books
    reject active-loan counts above total copies
display menu only after all available files have loaded successfully
```

### Calculate available book copies

```text
for each book:
    active = number of active loans referring to this book
    available = total copies - active
    display book, total copies, and available copies
```

### Produce a sorted report

```text
select the appropriate record array and comparator
sort the array by the requested key
display each record in sorted order
```

## Complexity analysis

Let `S`, `B`, `L`, `R`, and `C` be the numbers of students, books, loan
records, reservations, and complaints.

| Operation | Time | Extra space |
|---|---:|---:|
| Load all available files | O(S + B + L + R + C) | O(S + B + L + R + C) |
| Validate active loans against the catalog | O(B * L) | O(1) |
| Student report sort | O(S log S) | O(1) beyond the loaded array |
| Book report sort | O(B log B) | O(1) beyond the loaded array |
| Complaint report sort | O(C log C) | O(1) beyond the loaded array |
| Reservation report sort | O(R log R) | O(1) beyond the loaded array |
| Dashboard availability calculation | O(B * L + C + R) | O(1) |
| Total report storage | - | O(S + B + L + R + C) |

The source files are parsed once at startup. The report arrays consume memory
proportional to the total record count. The current validation and availability
calculation scan loans for each book; indexes could reduce that cost for much
larger collections.

## Persistence

The report program never writes source data. The student, library, and
complaint modules continue to save and reload their own records. Run those
modules from consistent directories or supply the corresponding paths to
`reports` so the report reads the files that those modules actually saved.

Example with explicit paths:

```text
reports.exe student_management\students.txt books.txt loans.txt topic4_complaints\complaints.dat reservations.txt
```

Keep related files together and back them up before manual edits. A report
generated from missing data files is marked unavailable rather than shown as
a misleading zero.

## Testing checklist

| Case | Expected result |
|---|---|
| Run with all five valid files | Dashboard loads and shows totals from each file |
| Omit a data file | That data source is marked unavailable; no fabricated count is shown |
| Use an invalid header or malformed record | Program identifies the file and exits with an error |
| Book has multiple copies and one active loan | Available count equals total copies minus one |
| Active loan refers to an unknown book | Report generation fails with an explicit consistency error |
| Active loans exceed a book's copies | Report generation fails with an explicit consistency error |
| Submit/cancel/fulfill reservation records | Only `WAITING` entries appear in the waiting-list report |
| Equal-priority complaints have different submission order | Earlier complaint appears first |
| Sort students, books, and complaints | Output follows the selected key without modifying saved records |
| Restart the owning modules, then run reports | The reports reflect the persisted records |

## Comparative performance study

The repository already includes measurements for student lookup and sorting,
book title search, and complaint lookup/next-pending selection in the
topic-specific reports. The following values are transcribed from those
module reports, not newly measured by the Topic 5 executable:

| Operation | Record count | Reported time |
|---|---:|---:|
| Student: 500,000 ID lookups | 100 | 16.3 ms |
| Student: 500,000 ID lookups | 1,000 | 18.7 ms |
| Student: 500,000 ID lookups | 10,000 | 39.7 ms |
| Student: name sort | 100 | 0.162 ms |
| Student: name sort | 1,000 | 2.040 ms |
| Student: name sort | 10,000 | 30.733 ms |
| Book: title search | 100 | 0.03693 ms/search |
| Book: title search | 500 | 0.10487 ms/search |
| Book: title search | 1,000 | 0.17302 ms/search |
| Complaint: lookup | 100 | 0.040 µs/op |
| Complaint: lookup | 1,000 | 0.400 µs/op |
| Complaint: lookup | 10,000 | 3.000 µs/op |
| Complaint: next pending | 100 | 0.120 µs/op |
| Complaint: next pending | 1,000 | 0.800 µs/op |
| Complaint: next pending | 10,000 | 9.000 µs/op |

Student ID lookup grows much more slowly than the record count because the
student module uses a hash table. Student sorting and book searching take
longer as input grows, consistent with O(n log n) sorting and linear search.
Complaint lookup and next-pending selection scan the complaint array, so their
times also increase with the number of records. These measurements came from
different module reports and environments, so compare trends within an
operation rather than treating absolute timings across modules as directly
comparable.

For the final group submission, rerun the included benchmark commands on the
same machine with the same compiler and optimization flags. Record the
compiler/version, flags, record sizes, number of repetitions, statistic used
(mean or median), and whether file loading or output is included. Replace the
transcribed values if the group obtains new measurements.

## Build and run

From the project root:

```text
gcc -std=c99 -Wall -Wextra -Wpedantic reports.c -o reports
./reports
```

On Windows, run `reports.exe`. The default paths and optional path arguments
are described in the root README.
