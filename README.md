# Library Catalog and Book Management

A standalone, menu-driven C program implementing topic 2 of the BTCS324 project:
catalog maintenance, book search, copy availability, issuing and returning books,
and tracking which student has each active loan.

## Requirements

- A C compiler supporting C99 or later.
- Run the program from a writable directory. `books.txt` and `loans.txt` are
  created there when the first book or loan is saved.

## Build and run

With GCC or Clang:

```text
gcc -std=c99 -Wall -Wextra -pedantic library.c -o library
./library
```

With Microsoft C/C++ from a Developer Command Prompt:

```text
cl /W4 /TC library.c
library.exe
```

On Windows, run `library.exe` from the directory in which you want the data
files stored. The program does not need pre-populated input files.

## Menu

1. Add a book (unique ID, title, author, and positive total-copy count).
2. Update title, author, or total copies by book ID. The total cannot be set
   below the number of copies currently on loan.
3. Remove a book if it has no active loans.
4. Display the catalog, including total and available copies.
5. Search by exact book ID or by a case-insensitive title fragment.
6. Issue one available copy to a student. The program refuses an issue if no
   copy is available and prints the new loan ID.
7. Return a book using its loan ID.
8. Display active loans with loan ID, student ID, book ID, and title.

IDs may contain letters, digits, hyphens, and underscores. Titles and author
names may contain spaces but cannot contain the `|` character used as the data
file separator.

## Saved data

- `books.txt` stores the catalog as `book ID|title|author|total copies`.
- `loans.txt` stores the loan ID, book ID, student ID, and status (`1` for
  active, `0` for returned). Returned entries are retained as history.
- Both files include a version header and are loaded when the application
  starts. Keep both files together and do not edit their records while the
  application is running. Back them up before manual edits.

Availability is calculated as `total copies - active loans`; it is not stored
as an independently editable number. This prevents availability from going
negative and ensures issue/return actions update the displayed count.
