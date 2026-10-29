/*
 * test_itron_scheduler.c — Unit tests for μITRON 3.0 Real-Time Scheduler.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../../src/kernel/itron_task.h"

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { fprintf(stderr, "  [FAIL] %s\n", msg); return 1; } while(0)

static int s_task1_runs = 0;
static int s_task2_runs = 0;
static int s_task3_runs = 0;
static bool s_task1_should_sleep = false;

static void task1_func(void)
{
    s_task1_runs++;
    if (s_task1_should_sleep) {
        itron_task_sleep(5);
        s_task1_should_sleep = false;
    }
}
static void task2_func(void) { s_task2_runs++; }
static void task3_func(void) { s_task3_runs++; }

int main(void)
{
    printf("=== Test: uITRON 3.0 Real-Time Task Scheduler ===\n");

    /* 1. Initialization */
    itron_init();
    if (itron_get_system_ticks() != 0) {
        TEST_FAIL("Initial system ticks non-zero");
    }
    TEST_PASS("Scheduler initialization");

    /* 2. Task Registration */
    ER err = itron_task_create(1, task1_func, 1, "Task1");
    if (err != E_OK) TEST_FAIL("Task 1 creation");

    err = itron_task_create(2, task2_func, 2, "Task2");
    if (err != E_OK) TEST_FAIL("Task 2 creation");

    err = itron_task_create(3, task3_func, 3, "Task3");
    if (err != E_OK) TEST_FAIL("Task 3 creation");

    /* Error cases: duplicate or out-of-range ID */
    err = itron_task_create(0, task1_func, 1, "BadZero");
    if (err != E_PAR) TEST_FAIL("Allowed invalid task ID 0");

    err = itron_task_create(MAX_TASKS + 1, task1_func, 1, "BadMax");
    if (err != E_PAR) TEST_FAIL("Allowed task ID exceeding MAX_TASKS");

    err = itron_task_create(4, NULL, 1, "BadNull");
    if (err != E_PAR) TEST_FAIL("Allowed task with NULL entry");

    TEST_PASS("Task registration and parameter boundary checking");

    /* 3. Task Dispatching (3 dispatch slices for 3 tasks) */
    for (int i = 0; i < 3; i++) itron_task_dispatch();
    if (s_task1_runs != 1 || s_task2_runs != 1 || s_task3_runs != 1) {
        fprintf(stderr, "Runs: T1=%d, T2=%d, T3=%d\n", s_task1_runs, s_task2_runs, s_task3_runs);
        TEST_FAIL("Task dispatching cycle");
    }
    TEST_PASS("Cooperative cyclic dispatching");

    /* 4. Task Sleep and Wakeup via Ticks */
    s_task1_should_sleep = true;
    for (int i = 0; i < 3; i++) itron_task_dispatch(); /* Task 1 runs and calls itron_task_sleep(5) */
    const TaskControlBlock *t1 = itron_get_task(1);
    if (!t1 || t1->state != TSK_WAITING || t1->wait_ticks != 5) {
        TEST_FAIL("Task sleep state transition");
    }

    /* Dispatch while sleeping: Task 1 should NOT run (run count stays 2) */
    for (int i = 0; i < 3; i++) itron_task_dispatch();
    if (s_task1_runs != 2) {
        TEST_FAIL("Sleeping task was executed");
    }
    TEST_PASS("Task sleep prevention");

    /* Tick 4 times (remains waiting: 1 tick left) */
    for (int i = 0; i < 4; i++) itron_task_tick();
    for (int i = 0; i < 3; i++) itron_task_dispatch();
    if (s_task1_runs != 2) {
        TEST_FAIL("Task woke prematurely");
    }

    /* 5th tick: Task 1 should wake up to READY */
    itron_task_tick();
    if (t1->state != TSK_READY) {
        TEST_FAIL("Task failed to transition to READY on timer expiration");
    }
    for (int i = 0; i < 3; i++) itron_task_dispatch();
    if (s_task1_runs != 3) {
        TEST_FAIL("Awakened task did not execute");
    }
    TEST_PASS("Timer tick wake-up and state machine");

    printf("=== All uITRON Scheduler Tests PASSED ===\n\n");
    return 0;
}
