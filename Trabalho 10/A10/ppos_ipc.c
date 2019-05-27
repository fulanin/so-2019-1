#include "ppos.h"

extern queue_t *ready_queue;
extern task_t *current_task;

int sem_create(semaphore_t *s, int value)
{
  if (!s)
    return -1;

  s->valid = 1;
  s->locks = value;
  s->blocked = NULL;

  return 0;
}

int sem_down(semaphore_t *s)
{
  if (!s || !s->valid)
    return -1;

  current_task->kernel_task = 1;

  if ((--s->locks) < 0)
  {
    queue_remove(&ready_queue, (queue_t*)current_task);
    queue_append(&s->blocked, (queue_t*)current_task);

    current_task->kernel_task = 0;

    task_yield();
  }

  current_task->kernel_task = 0;

  return 0;
}

int sem_up (semaphore_t *s)
{
  if (!s || !s->valid)
    return -1;
 
  current_task->kernel_task = 1;

  ++s->locks;
  if (s->blocked)
  {
    queue_t *task = queue_remove(&s->blocked, s->blocked);
    queue_append(&ready_queue, task);
  }

  current_task->kernel_task = 0;

  return 0;
}

int sem_destroy (semaphore_t *s)
{
  if (!s)
    return -1;

  s->valid = 0;
  if (s->blocked)
  {
    for (; s->locks < 0; ++s->locks)
    {
      queue_t *task = queue_remove(&s->blocked, s->blocked);
      queue_append(&ready_queue, task);
    }
  }
  else
  {
    s->locks = 0;
  }

  return 0;
}