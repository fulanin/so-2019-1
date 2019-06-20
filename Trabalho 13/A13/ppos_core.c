#include "ppos.h"

#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int kStackSize = 32768,
          kReady = 0,
          kRunning = 1,
          kSleeping = 2,
          kSuspended = 3,
          kTerminated = 4;

struct sigaction time_handler;
struct itimerval timer;

int available_id,
    user_tasks,
    quantum,
    time;

queue_t *ready_queue,
        *suspended_queue,
        *sleeping_queue;

task_t dispatcher,
       main_task,
       *current_task;

void print_elem (void *ptr) {
   task_t *elem = ptr ;

   if (!elem)
      return ;
   printf ("<%d>", elem->id) ;
}

void tick_handler(int signal)
{
  ++time;
  ++current_task->processor_time;
  if (!current_task->kernel_task)
  {
    if (!(--quantum))
    {
      #ifdef DEBUG
      printf("Quantum for the task %d ended. Switching to dispatcher.\n",
             current_task->id);
      #endif

      task_switch(&dispatcher);
    }
  }
}

task_t* schedule(void)
{
  task_t *next_task;
  if (!(next_task = (task_t*)ready_queue)) {
    // puts("vazio");
    return NULL;
  }
  int pass = 0;
  for (queue_t *current = ready_queue->next;
       pass < 1;
       current = current->next)
  {
    if (current->next == ready_queue)
      ++pass;

    if (((task_t*)current)->dinamic_priority < next_task->dinamic_priority)
    {
      --next_task->dinamic_priority;
      next_task = (task_t*)current;
    }
    else
    {
      --((task_t*)current)->dinamic_priority;
    }
  }
  next_task->dinamic_priority = next_task->static_priority;

  #ifdef DEBUG
  printf("Next task is %d.\n", next_task->id);
  #endif

  return (task_t*)queue_remove(&ready_queue, (queue_t*)next_task);
}

void dispatch(void)
{
  task_t *next_task;
  while (user_tasks)
  {
    if ((next_task = schedule()))
    {
      // queue_append(&ready_queue, (queue_t*)next_task);
      // queue_print("Ready list", (queue_t*)ready_queue, &print_elem);
      quantum = 20;
      task_switch(next_task);
      if (next_task->state == kTerminated)
      {
        free(next_task->stack);
        next_task->context.uc_stack.ss_sp = NULL;
        next_task->context.uc_stack.ss_size = 0;

        queue_remove(&ready_queue, (queue_t*)next_task);
        --user_tasks;
      }
    }

    if (sleeping_queue)
    {
      int pass = 0;
      queue_t *current = sleeping_queue;
      while (pass < 1)
      {
        if (current->next == sleeping_queue)
          ++pass;

        if (systime() >= ((task_t*)current)->wake_time)
        {
          queue_t *next = current->next;

          ((task_t*)current)->state = kReady;
          queue_remove(&sleeping_queue, current);
          queue_append(&ready_queue, current);

          current = next;
        }
        else
        {
          current = current->next;
        }
      }
    }
  }

  task_exit(0);
}

//----------------------------------------------------------------------------//

void ppos_init(void)
{
  setvbuf(stdout, 0, _IONBF, 0);

  time = 0;
  available_id = 0;
  user_tasks = 0;
  quantum = 20;
  ready_queue = NULL;
  suspended_queue = NULL;
  sleeping_queue = NULL;

  getcontext(&main_task.context);
  main_task.id = available_id++;
  main_task.kernel_task = 0;
  main_task.state = kRunning;
  main_task.static_priority = 0;
  main_task.dinamic_priority  = main_task.static_priority;
  main_task.creation_time = systime();
  main_task.processor_time = 0;
  main_task.activations = 0;
  main_task.waiting_for = NULL;
  main_task.wake_time = 0;
  queue_append(&ready_queue, (queue_t*)&main_task);
  ++user_tasks;
  current_task = &main_task;

  task_create(&dispatcher, (void*)&dispatch, NULL);
  dispatcher.kernel_task = 1;
  queue_remove(&ready_queue, (queue_t*)&dispatcher);
  --user_tasks;

  time_handler.sa_handler = tick_handler;
  sigemptyset(&time_handler.sa_mask);
  time_handler.sa_flags = 0;
  if (sigaction(SIGALRM, &time_handler, 0) < 0)
  {
    perror("Erro em sigaction: ");
    exit(1);
  }

  timer.it_value.tv_usec = 1000;
  timer.it_value.tv_sec = 0;
  timer.it_interval.tv_usec = 1000;
  timer.it_interval.tv_sec = 0;
  if (setitimer(ITIMER_REAL, &timer, 0) < 0)
  {
    perror("Erro em setitimer: ");
    exit(1);
  }
}

int task_create(task_t *task, void(*start_func)(void*), void *arg)
{
  if (!task || !start_func)
    return -1;

  getcontext(&task->context);
  if ((task->stack = malloc(kStackSize)))
  {
    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = kStackSize;
    task->context.uc_stack.ss_flags = 0;

    task->context.uc_link = NULL;

    task->id = available_id++;
    task->kernel_task = 0;
    task->state = kReady;
    task->static_priority = 0;
    task->dinamic_priority  = task->static_priority;
    task->creation_time = systime();
    task->processor_time = 0;
    task->activations = 0;
    task->waiting_for = NULL;
    task->wake_time = 0;

    makecontext(&task->context, (void*)start_func, 1, arg);

    queue_append(&ready_queue, (queue_t*)task);
    ++user_tasks;
  }
  else
  {
    perror("Erro na criação da pilha!");
    exit(1);
  }

  return task->id;
}

int task_switch(task_t *task)
{
  if (!task)
    return -1;

  task_t *old_task = current_task;
  current_task = task;

  if (old_task->state != kTerminated)
    old_task->state = kReady;
  current_task->state = kRunning;

  ++current_task->activations;

  #ifdef DEBUG
  printf("Swithcing from task %d to task %d.\n",
         old_task->id, current_task->id);
  #endif

  swapcontext(&old_task->context, &task->context);

  return 0;
}

void task_exit(int exitCode)
{
  current_task->state = kTerminated;
  current_task->exit_code = exitCode;

  #ifdef DEBUG
  printf("Exiting task %d.\n", current_task->id);
  #endif

  printf("Task %d exit: execution time %d ms, "
         "processor time %d ms, %d activations\n",
         current_task->id, systime() - current_task->creation_time,
         current_task->processor_time, current_task->activations);

  if (current_task != &dispatcher)
  {
    if (suspended_queue)
    {
      int pass = 0;
      queue_t *current = suspended_queue;
      while (pass < 1)
      {
        if (current->next == suspended_queue)
          ++pass;

        if (((task_t*)current)->waiting_for == current_task)
        {
          queue_t *next = current->next;
          ((task_t*)current)->waiting_for = NULL;
          ((task_t*)current)->state = kReady;

          queue_remove(&suspended_queue, current);
          queue_append(&ready_queue, current);
          current = next;
        }
        else
        {
          current = current->next;
        }
      }
    }

    task_switch(&dispatcher);
  }
  else
  {
    task_switch(&main_task);
  }
}

int task_id()
{
  return current_task->id;
}

void task_yield(void)
{
  task_switch(&dispatcher);
}

void task_setprio(task_t *task, int prio)
{
  if (prio < -20 || prio > 20)
    return;

  if (!task)
    current_task->static_priority = prio;
  else
    task->static_priority = prio;
}

int task_getprio(task_t *task)
{
  if (!task)
    return current_task->static_priority;
  return task->static_priority;
}

unsigned int systime()
{
  return time;
}

int task_join(task_t *task)
{
  if (!task || task == current_task)
    return -1;

  if (task->state != kTerminated)
  {
    current_task->waiting_for = task;
    current_task->state = kSuspended;
    queue_remove(&ready_queue, (queue_t*)current_task);
    queue_append(&suspended_queue, (queue_t*)current_task);

    task_switch(&dispatcher);
  }

  return task->exit_code;
}

void task_sleep(int t)
{
  current_task->wake_time = systime() + t;
  current_task->state = kSleeping;
  queue_remove(&ready_queue, (queue_t*)current_task);
  queue_append(&sleeping_queue, (queue_t*)current_task);

  task_switch(&dispatcher);
}

int mqueue_create(mqueue_t *queue, int max, int size)
{
  if (!queue || !max || !size)
    return -1;

  queue->queue = (void*)malloc(max * size);
  queue->size = max;
  queue->head = 0;
  queue->tail = 0;
  queue->elem_size = size;
  queue->elem_count = 0;
  sem_create(&queue->empty, 0);
  sem_create(&queue->full, 0);
  sem_create(&queue->mutex, 1);

  queue->valid = 1;

  return 0;
}

int mqueue_send(mqueue_t *queue, void *msg)
{
  if (!queue || !msg || !queue->valid)
    return -1;

  sem_down(&queue->mutex);
  if (queue->elem_count >= queue->size)
  {
    sem_up(&queue->mutex);
    sem_down(&queue->full);
  }

  if (!queue->valid)
    return -1;

  memcpy((char*)(queue->queue) + queue->tail * queue->elem_size,
         msg, queue->elem_size);
  ++queue->tail;
  queue->tail %= queue->size;
  ++queue->elem_count;

  if (!(queue->elem_count - 1))
    sem_up(&queue->empty);
  sem_up(&queue->mutex);

  return 0;
}

int mqueue_recv(mqueue_t *queue, void *msg)
{
  if (!queue || !msg || !queue->valid)
    return -1;

  sem_down(&queue->mutex);

  if (!queue->elem_count)
  {
    sem_up(&queue->mutex);
    sem_down(&queue->empty);
  }

  if (!queue->valid)
    return -1;

  memcpy(msg, (char*)(queue->queue) + queue->head * queue->elem_size,
         queue->elem_size);
  ++queue->head;
  queue->head %= queue->size;
  --queue->elem_count;

  if (queue->elem_count + 1 >= queue->size)
    sem_up(&queue->full);
  sem_up(&queue->mutex);

  return 0;
}

int mqueue_destroy(mqueue_t *queue)
{
  if (!queue)
    return -1;

  sem_down(&queue->mutex);

  free(queue->queue);
  queue->size = 0;
  queue->head = 0;
  queue->tail = 0;
  queue->elem_size = 0;
  queue->elem_count = 0;

  queue->valid = 0;

  sem_up(&queue->mutex);

  sem_destroy(&queue->empty);
  sem_destroy(&queue->full);
  sem_destroy(&queue->mutex);

  return 0;
}

int mqueue_msgs(mqueue_t *queue)
{
  return queue->elem_count;
}