#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "ppos.h"

#define STACKSIZE 32768

int current_id=0;
int userTasks=0;
int init=0;
task_t *tcb, *ready, *current_task;
task_t main_task, dispatcher;

// ----------
// Helpers
void print_elem (void *ptr) {
   task_t *elem = ptr ;

   if (!elem)
      return ;
   printf ("<%d>", elem->id) ;
}

int id_create () {
  int result = current_id;
  current_id++;
  return result;
}

// ----------
// Scheduler
task_t *scheduler () {
  return ready;
}

// ----------
// Dispatcher
void dispatcher_body () {
  task_t *next;
  while ( userTasks > 0 ) {
    next = scheduler();
    if (next) {
      task_switch (next);
    }
  }
  task_exit(1);
}

// Devolve uma tarefa para o final da fila de prontas e 
// devolve o processador para o dispatcher
void task_yield () {
  if (init != 0)
    ready = ready->next;
  init = 1;
  task_switch(&dispatcher);

  // No final, devolve a execucao para o main
  if (userTasks == 0)
    task_exit(1);
}

// ----------
// Tasks
void ppos_init () {
  setvbuf (stdout, 0, _IONBF, 0);
  main_task.id = id_create();
  getcontext (&main_task.context);
  queue_append((queue_t**) &tcb, (queue_t*) &main_task);

  dispatcher.id = id_create();
  getcontext(&dispatcher.context);
  char *stack = malloc (STACKSIZE) ;
  if (stack) {
    dispatcher.context.uc_stack.ss_sp = stack ;
    dispatcher.context.uc_stack.ss_size = STACKSIZE ;
    dispatcher.context.uc_stack.ss_flags = 0 ;
    dispatcher.context.uc_link = 0 ;
    // task->stack = task->context.uc_stack;
  } else {
    perror ("Erro na criação da pilha: ") ;
    return ;
  }
  makecontext(&dispatcher.context, (void*)(*dispatcher_body), 0) ;
  queue_append((queue_t**) &tcb, (queue_t*) &dispatcher);

  current_task = &main_task;
}

int task_create (task_t *task, void (*start_routine)(void *),  void *arg) {
  task->id = id_create();
  getcontext (&task->context);
  char *stack = malloc (STACKSIZE) ;
  if (stack) {
    task->context.uc_stack.ss_sp = stack ;
    task->context.uc_stack.ss_size = STACKSIZE ;
    task->context.uc_stack.ss_flags = 0 ;
    task->context.uc_link = 0 ;
    // task->stack = task->context.uc_stack;
  } else {
    perror ("Erro na criação da pilha: ") ;
    return 0;
  }

  makecontext(&task->context, (void*)(*start_routine), 1, arg) ;
  // queue_append((queue_t**) &tcb, (queue_t*) task);
  queue_append((queue_t**) &ready, (queue_t*) task);
  ++userTasks;
  return task->id;
}

int task_switch (task_t *task) {
  if (!task) 
    return -1;

  task_t *old_task = current_task;
  current_task = task;
  swapcontext(&old_task->context, &current_task->context);
  return 0;
}

void task_exit (int exitCode) {
  switch (exitCode) {
    case 0:
      --userTasks;
      queue_remove((queue_t**) &ready, (queue_t*) ready);
      task_switch(&dispatcher);
      break;

    case 1:
      task_switch(&main_task);
      break;
  }
}

int task_id () {
  return current_task->id;
}