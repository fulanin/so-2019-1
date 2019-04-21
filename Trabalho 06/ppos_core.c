#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"

#define STACKSIZE 32768
#define QUANTUM 20

int current_id=0;
int userTasks=0;
int init=0;

unsigned int global_time=0;

task_t *tcb, *ready, *current_task;
task_t main_task, dispatcher;

struct sigaction action;
struct itimerval timer;

// ----------
// Helpers
int limit (int x, int a, int b) {
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

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

void handler (int signum) {
  ++global_time;
  ++current_task->cpu_time;
  // printf ("Recebi o sinal %d\n", signum);
  if (--current_task->quantum <= 0) {
    task_yield();
  }
}

unsigned int systime () {
  return global_time;  
}

// ----------
// Scheduler
void task_setprio (task_t *task, int prio) {
  if (!task) {
    current_task->static_prio = limit(prio, -20, 20);
    // current_task->dynamic_prio = current_task->static_prio;
    return;
  }
  task->static_prio = limit(prio, -20, 20);
  // task->dynamic_prio = task->static_prio;
  return;
}

int task_getprio (task_t *task) {
  if (!task)
    return current_task->static_prio;
  return task->static_prio;
}

task_t *scheduler () {
  task_t *next = ready;

  for (task_t *node = ready->next; node != ready; node = node->next) {
    if (init == 0) 
      node->dynamic_prio = node->static_prio;
    // printf("Dynamic: %d // Static: %d\n", node->dynamic_prio, node->static_prio);
    if (node->dynamic_prio < next->dynamic_prio) {
      next->dynamic_prio--;
      next = node;
    } else {
      node->dynamic_prio--;    
    }
  }
  init = 1;
  next->dynamic_prio = next->static_prio;
  return next;
}

// ----------
// Dispatcher
void dispatcher_body () {
  task_t *next;

  while ( userTasks > 0 ) {
    next = scheduler();
    if (next) {
      next->quantum = QUANTUM;
      // ++next->activations;
      queue_remove((queue_t**) &ready, (queue_t*) next);
      task_switch (next);
    }
  }
  task_exit(1);
}

// Devolve uma tarefa para o final da fila de prontas e 
// devolve o processador para o dispatcher
void task_yield () {
  // if (init != 0)
  //   ready = ready->next;
  // init = 1;
  queue_append((queue_t**) &ready, (queue_t*) current_task);
  // ++dispatcher.activations;
  task_switch(&dispatcher);

  // No final, devolve a execucao para o main
  // if (userTasks == 0)
  //   task_exit(1);
}

// ----------
// Tasks
void ppos_init () {
  setvbuf (stdout, 0, _IONBF, 0);

  action.sa_handler = handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  if (sigaction (SIGALRM, &action, 0) < 0) {
    perror ("Erro em sigaction: ");
    exit (1);
  }

  timer.it_value.tv_usec    = 1000;
  timer.it_value.tv_sec     = 0;
  timer.it_interval.tv_usec = 1000;
  timer.it_interval.tv_sec  = 0; 
  if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
    perror ("Erro em setitimer: ");
    exit (1);
  }

  main_task.id = id_create();
  // main_task.static_prio = main_task.dynamic_prio = 0;
  getcontext (&main_task.context);
  queue_append((queue_t**) &tcb, (queue_t*) &main_task);

  dispatcher.id = id_create();
  dispatcher.activations = 0;
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
  task_setprio(task, 0);
  task->dynamic_prio = task->static_prio;
  task->start = global_time;
  task->activations = 0;
  // task->quantum = QUANTUM;
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
  current_task->activations++;
  swapcontext(&old_task->context, &current_task->context);
  return 0;
}

void task_exit (int exitCode) {
  switch (exitCode) {
    case 0:
      // queue_print("Ready list", (queue_t*)ready, &print_elem);
      printf("Task %d exit: running time %u ms, cpu time %u ms, %u activations\n", 
             current_task->id, global_time - current_task->start, current_task->cpu_time, current_task->activations);
      --userTasks;
      // queue_remove((queue_t**) &ready, (queue_t*) current_task);
      task_switch(&dispatcher);
      break;

    case 1:
      printf("Task %d exit: running time %u ms, cpu time %u ms, %u activations\n", 
             dispatcher.id, global_time - dispatcher.start, dispatcher.cpu_time, dispatcher.activations);
      task_switch(&main_task);
      break;
  }
}

int task_id () {
  return current_task->id;
}