# DSA

## CampusAssist: Student Management

The standalone student information management module is in
[`student_management/`](./student_management/). It supports registering,
searching, updating, removing, displaying, and sorting student records, with
file persistence and a built-in lookup/sorting benchmark.

Build and run from that directory:

```text
gcc -std=c99 -Wall -Wextra -pedantic student_management.c -o student_management
.\student_management.exe
```

See [`student_management/README.md`](./student_management/README.md) for data
structure choices, complexity analysis, test steps, performance results, and
the benchmark command.