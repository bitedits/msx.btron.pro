/*
 * itron_task.h — μITRON 3.0 Cooperative Task Scheduler for 8-bit MSX.
 */

#ifndef _ITRON_TASK_H_
#define _ITRON_TASK_H_

#include "btron_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TSK_DORMANT = 0,
    TSK_READY   = 1,
    TSK_RUNNING = 2,
    TSK_WAITING = 3
} TaskState;

typedef struct {
    ID        tskid;
    TaskState state;
    PRI       priority;
    void    (*entry)(void);
    UH        wait_ticks;
    char      name[12];
} TaskControlBlock;

void itron_init(void);
ER   itron_task_create(ID tskid, void (*entry)(void), PRI priority, const char *name);
void itron_task_yield(void);
void itron_task_sleep(UH ticks);
void itron_task_tick(void);
void itron_task_dispatch(void);
ID   itron_get_current_task_id(void);
UH   itron_get_system_ticks(void);
const TaskControlBlock *itron_get_task(ID tskid);

#ifdef __cplusplus
}
#endif

#endif /* _ITRON_TASK_H_ */

