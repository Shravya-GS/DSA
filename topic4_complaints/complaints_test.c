#include "complaints.h"

#include <assert.h>
#include <stdio.h>

#define TEST_DATA_FILE "topic4_test_data.dat"

static void test_priority_and_fairness(void)
{
    ComplaintStore store;
    int low_id;
    int first_high_id;
    int second_high_id;
    const Complaint *next;

    remove(TEST_DATA_FILE);
    assert(complaint_store_open(&store, TEST_DATA_FILE));
    assert(store.count == 0);
    assert(complaint_submit(
        &store, 1001, COMPLAINT_LOW, "Classroom", "Broken desk", &low_id));
    assert(complaint_submit(
        &store, 1002, COMPLAINT_HIGH, "Electrical", "Exposed wire",
        &first_high_id));
    assert(complaint_submit(
        &store, 1003, COMPLAINT_HIGH, "Water", "No water", &second_high_id));

    next = complaint_next_pending(&store);
    assert(next != NULL && next->id == first_high_id);
    assert(complaint_set_status(&store, first_high_id, COMPLAINT_IN_PROGRESS));
    next = complaint_next_pending(&store);
    assert(next != NULL && next->id == second_high_id);
    assert(complaint_set_status(&store, second_high_id, COMPLAINT_RESOLVED));
    next = complaint_next_pending(&store);
    assert(next != NULL && next->id == low_id);
    complaint_store_close(&store);
}

static void test_persistence_and_validation(void)
{
    ComplaintStore store;
    const Complaint *found;
    int created_id;

    assert(complaint_store_open(&store, TEST_DATA_FILE));
    assert(store.count == 3);
    assert(!complaint_submit(
        &store, 0, COMPLAINT_LOW, "Library", "Invalid student", NULL));
    assert(!complaint_submit(
        &store, 1004, COMPLAINT_LOW, "Library|Water", "Bad separator", NULL));
    assert(!complaint_set_status(&store, 9999, COMPLAINT_RESOLVED));
    assert(!complaint_set_status(&store, 1, (ComplaintStatus)99));
    assert(complaint_submit(
        &store, 1004, COMPLAINT_MEDIUM, "Library", "Book return desk",
        &created_id));
    assert(created_id == 4);
    complaint_store_close(&store);

    assert(complaint_store_open(&store, TEST_DATA_FILE));
    found = complaint_find(&store, created_id);
    assert(found != NULL);
    assert(found->student_id == 1004);
    assert(found->status == COMPLAINT_PENDING);
    complaint_store_close(&store);
    remove(TEST_DATA_FILE);
}

static void test_corrupt_file_rejection(void)
{
    ComplaintStore store;
    FILE *file = fopen(TEST_DATA_FILE, "w");

    assert(file != NULL);
    assert(fputs("not a complaint file\n", file) >= 0);
    assert(fclose(file) == 0);
    assert(!complaint_store_open(&store, TEST_DATA_FILE));
    remove(TEST_DATA_FILE);
}

int main(void)
{
    test_priority_and_fairness();
    test_persistence_and_validation();
    test_corrupt_file_rejection();
    puts("All fresh Topic 4 tests passed.");
    return 0;
}
