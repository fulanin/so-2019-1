// Codigo desenvolvido para a disciplina de Sistemas Operacionais por:
// GRR20116644 - Guilherme Lopes do Nascimento
// GRR20136059 - Matheus Rotondano de Camargo

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
int lock[255];

unsigned int global_time=0;

task_t *tcb, *ready, *suspended, *current_task;
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
  if (--current_task->quantum <= 0) {
    task_yield();
  }
}

unsigned int systime () {
  return global_time;  
}

int queue_contains (queue_t **queue, queue_t *elem) {
  queue_t **first = queue;
  queue_t *node;

  if (queue == NULL) {
    fprintf(stderr, "Queue is NULL, aborting.\n");
    return -1;
  }

  if (elem == NULL) {
    fprintf(stderr, "Element is  NULL, aborting.\n");
    return -1;
  }
  
  for (node = (*first); node->next != (*first); node = node->next) {
    if (elem == node) {      
      return 1;
    }
  }

  // Try the remaining element on the queue
  if (elem == node) {    
    return 1;
  }

  // The element is not contained in the queue
  return 0;
}

// ----------
// Scheduler
void task_setprio (task_t *task, int prio) {
  if (!task) {
    current_task->static_prio = limit(prio, -20, 20);
    return;
  }
  task->static_prio = limit(prio, -20, 20);
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
      queue_remove((queue_t**) &ready, (queue_t*) next);
      task_switch (next);
    }
  }
  task_exit(1);
}

// Devolve uma tarefa para o final da fila de prontas e 
// devolve o processador para o dispatcher
void task_yield () {
  queue_append((queue_t**) &ready, (queue_t*) current_task);
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
  task_setprio(&main_task, 0);
  main_task.dynamic_prio = main_task.static_prio;
  main_task.start = global_time;
  main_task.activations = 0;
  main_task.waiting = NULL;
  main_task.quantum = QUANTUM;
  getcontext(&main_task.context);
  char *main_stack = malloc (STACKSIZE) ;
  if (main_stack) {
    main_task.context.uc_stack.ss_sp = main_stack ;
    main_task.context.uc_stack.ss_size = STACKSIZE ;
    main_task.context.uc_stack.ss_flags = 0 ;
    main_task.context.uc_link = 0 ;
    // task->stack = task->context.uc_stack;
  } else {
    perror ("Erro na criação da pilha: ") ;
    return ;
  }
  queue_append((queue_t**) &ready, (queue_t*) &main_task);

  dispatcher.id = id_create();
  dispatcher.activations = 0;
  getcontext(&dispatcher.context);
  char *dispatcher_stack = malloc (STACKSIZE) ;
  if (dispatcher_stack) {
    dispatcher.context.uc_stack.ss_sp = dispatcher_stack ;
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
  task->waiting = NULL;
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
  // printf("Old task: %d // New task %d\n", old_task->id, current_task->id);
  // queue_print("Ready list", (queue_t*)ready, &print_elem);
  swapcontext(&old_task->context, &current_task->context);
  return 0;
}

void task_exit (int exitCode) {
  task_t *node;

  current_task->exit_code = exitCode;
  printf("Task %d exit: running time %u ms, cpu time %u ms, %u activations\n", 
            current_task->id, global_time - current_task->start, 
            current_task->cpu_time, current_task->activations);

  if (current_task != &dispatcher) {
    if (suspended) {
      // find out which task is waiting for current task
      for (node = suspended; node->next != suspended; node = node->next) {
        if (node->waiting == current_task) {
          node->waiting = NULL;
          queue_remove((queue_t**) &suspended, (queue_t*) node);
          queue_append((queue_t**) &ready, (queue_t*) node); 
        }
      }
      if (node->waiting == current_task) {
        node->waiting = NULL;
        queue_remove((queue_t**) &suspended, (queue_t*) node);
        queue_append((queue_t**) &ready, (queue_t*) node);  
      }  
      // queue_print("Ready list", (queue_t*)ready, &print_elem);   
    --userTasks;
    task_switch(&dispatcher);
    }
  }
  else {
    task_switch(&main_task);
  }
}

int task_id () {
  return current_task->id;
}

int task_join (task_t *task) {
  if (!task || task == current_task)
    return -1;

  current_task->waiting = task;
  queue_remove((queue_t**) &ready, (queue_t*) current_task);
  queue_append((queue_t**) &suspended, (queue_t*) current_task);
  // queue_print("Suspended list", (queue_t*)suspended, &print_elem);
  task_switch(&dispatcher);
  return task->exit_code;
}