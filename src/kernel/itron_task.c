/*
 * itron_task.c — μITRON Cooperative Task Scheduler implementation.
 */

#include "itron_task.h"

static TaskControlBlock s_tasks[MAX_TASKS];
static ID s_current_task_idx = 0;
static UH s_system_ticks = 0;

void itron_init(void)
{
    for (UH i = 0; i < MAX_TASKS; i++) {
        s_tasks[i].tskid = (ID)(i + 1);
        s_tasks[i].state = TSK_DORMANT;
        s_tasks[i].priority = 0;
        s_tasks[i].entry = NULL;
        s_tasks[i].wait_ticks = 0;
        s_tasks[i].name[0] = '\0';
    }
    s_current_task_idx = 0;
    s_system_ticks = 0;
}

ER itron_task_create(ID tskid, void (*entry)(void), PRI priority, const char *name)
{
    if (tskid <= 0 || tskid > MAX_TASKS || entry == NULL) {
        return E_PAR;
    }
    UH idx = (UH)(tskid - 1);
    s_tasks[idx].tskid = tskid;
    s_tasks[idx].state = TSK_READY;
    s_tasks[idx].priority = priority;
    s_tasks[idx].entry = entry;
    s_tasks[idx].wait_ticks = 0;

    UH j = 0;
    if (name) {
        while (name[j] != '\0' && j < 11) {
            s_tasks[idx].name[j] = name[j];
            j++;
        }
    }
    s_tasks[idx].name[j] = '\0';

    return E_OK;
}

void itron_task_yield(void)
{
    /* Yield current slice to next ready task */
}

void itron_task_sleep(UH ticks)
{
    if (s_current_task_idx < MAX_TASKS) {
        s_tasks[s_current_task_idx].state = TSK_WAITING;
        s_tasks[s_current_task_idx].wait_ticks = ticks;
    }
}

void itron_task_tick(void)
{
    s_system_ticks++;
    for (UH i = 0; i < MAX_TASKS; i++) {
        if (s_tasks[i].state == TSK_WAITING) {
            if (s_tasks[i].wait_ticks > 0) {
                s_tasks[i].wait_ticks--;
            }
            if (s_tasks[i].wait_ticks == 0) {
                s_tasks[i].state = TSK_READY;
            }
        }
    }
}

void itron_task_dispatch(void)
{
    /* Round-robin scheduling across READY tasks */
    for (UH count = 0; count < MAX_TASKS; count++) {
        if (++s_current_task_idx >= MAX_TASKS) s_current_task_idx = 0;
        if (s_tasks[s_current_task_idx].state == TSK_READY &&
            s_tasks[s_current_task_idx].entry != NULL) {
            
            s_tasks[s_current_task_idx].state = TSK_RUNNING;
            s_tasks[s_current_task_idx].entry();
            if (s_tasks[s_current_task_idx].state == TSK_RUNNING) {
                s_tasks[s_current_task_idx].state = TSK_READY;
            }
            break;
        }
    }
}

ID itron_get_current_task_id(void)
{
    return s_tasks[s_current_task_idx].tskid;
}

UH itron_get_system_ticks(void)
{
    return s_system_ticks;
}

const TaskControlBlock *itron_get_task(ID tskid)
{
    if (tskid <= 0 || tskid > MAX_TASKS) {
        return NULL;
    }
    return &s_tasks[tskid - 1];
}

