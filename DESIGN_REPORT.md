# Design Report: Library Catalog and Book Management

## 1. Problem and scope

This module provides the library-management topic in the project brief. An
administrator can add, update, remove, display, and search books. A student can
be associated with a book issue; the application refuses an issue when no copy
is available and records returns. Catalog and loan records persist after the
program exits.

## 2. Assumptions

- Each book has a unique ID; each student is identified by a student ID.
- One loan record represents one physical copy.
- Every issue receives a unique loan ID. A return uses this ID, so separate
  issues of the same book can be distinguished.
- A book cannot be deleted while any copy is on loan.
- Total copies must be positive and cannot be reduced below the number of
  active loans.
- Data files are stored in the program's current working directory. IDs use
  letters, digits, hyphens, and underscores; `|` is reserved as a field
  separator.

## 3. Data representation

`Book` contains a book ID, title, author, and total copy count. `Loan` contains
a unique loan ID, book ID, student ID, and active/returned status. Fixed-size
arrays bound the catalog to 1,000 books and the loan history to 10,000 records.

The two versioned text files, `books.txt` and `loans.txt`, retain the records
between runs. A returned loan stays in the history with inactive status.
Available copies are derived, rather than duplicated in storage:

```text
available(book) = total_copies(book) - count(active loans for book)
```

The issue operation first checks that this result is greater than zero and
only then saves an active loan. Returning a loan changes its status to
returned. The same count is used for catalog display and search results.

## 4. Main operation algorithms

### Issue a book

```text
read student ID and book ID
find the book; if absent, report not found
count active loans for the book
if active loan count >= total copies, reject the issue
create a unique loan ID and active loan record
save the loan history; report the loan ID
```

### Return a book

```text
read loan ID
find the matching active loan; if absent, report not found
mark the loan returned and save the loan history
available copies increase by one because the active-loan count decreased
```

### Remove a book

```text
find the book; if absent, report not found
if it has an active loan, reject removal
remove it from the catalog and save the catalog
```

## 5. Design choices

- **Derived availability:** prevents a separately stored copy count from
  drifting out of sync with loans. The issue check enforces the non-negative
  availability invariant.
- **Unique loan IDs:** make returns unambiguous even if the same student has
  several issues of the same title.
- **Separate catalog and loan files:** each file is updated by the operations
  that own that data; version headers and startup validation detect malformed
  records and inconsistent references.
- **Bounded arrays and input buffers:** keep memory use predictable and reject
  overlong or malformed user input instead of truncating IDs.
- **Linear search:** simple for the expected coursework-sized data sets. It
  avoids the additional implementation complexity of an index or dynamic
  database.

## 6. Complexity

Let `B` be the number of books and `L` the number of loan-history records.
Fixed-size field comparisons take constant time for the configured limits.

| Operation | Time | Extra space |
|---|---:|---:|
| Add book (duplicate-ID check) | O(B) | O(1) |
| Update/remove book lookup | O(B + L) worst case | O(1) |
| Display catalog | O(B(L + 1)) | O(1) |
| Search by ID | O(B + L) | O(1) |
| Search by title | O(B(L + 1)) | O(1) |
| Issue (book lookup and availability count) | O(B + L) | O(1) |
| Return by loan ID | O(L) | O(1) |
| Display active loans | O(L(B + 1)) | O(1) |

Catalog display and title search count active loans for each matching book;
the current-loans display looks up each loan's title in the catalog. For
larger collections, precomputed availability and a book-ID index could reduce
these costs; the linear implementation is intentionally straightforward for
this project. Persistent storage is O(B + L).

## 7. Verification checklist

Use the following cases during a demonstration. The application stores files
in its current directory, so run test cases in a fresh directory or back up
the data files first.

| Case | Action | Expected result |
|---|---|---|
| Add and display | Add `B101`, title `Data Structures`, author `A. Student`, 2 copies; display catalog | Total 2, available 2 |
| Search | Search by `B101`; then search title fragment `data` | Book found both times; title search is case-insensitive |
| Issue available copy | Issue `B101` to student `S01` | Loan ID shown; available count becomes 1 |
| Issue final copy | Issue `B101` to `S02` | Another loan ID shown; available count becomes 0 |
| Prevent over-issue | Attempt to issue `B101` to `S03` | Rejected; available count remains 0 |
| Reject invalid update | With both copies loaned, set total copies to 1 | Rejected; existing record remains unchanged |
| Protect active loan | Attempt to remove `B101` while loans are active | Rejected |
| Return one issue | Return the first displayed loan ID | Return succeeds; available count becomes 1 |
| Reject duplicate return | Return the same loan ID again | Rejected because it is no longer active |
| Persist data | Exit and restart; display catalog and active loans | Catalog, counts, and outstanding loans are retained |
| Reject unknown loan | Return a loan ID not present in the file | Rejected without changing counts |

## 8. Build and demonstration

Compile and run commands and menu descriptions are in [README.md](./README.md).
For a group demonstration, add a multi-copy book, search it, issue copies to
different student IDs until it is unavailable, show the rejected extra issue,
return one loan by ID, and restart the program to demonstrate persistence.

No measured benchmark values are claimed here. The table above describes
algorithmic costs; input-size timing measurements should be recorded on the
target compiler/machine if required for the final presentation.
