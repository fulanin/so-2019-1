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

task_t *tcb, *ready_q, *suspended_q, *sleeping_q, *current_task;
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
  task_t *next = ready_q;

  if (!ready_q) {
    return NULL;
  }

  for (task_t *node = ready_q->next; node != ready_q; node = node->next) {
    if (init == 0) 
      node->dynamic_prio = node->static_prio;
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
    // printf("%d ", userTasks);   
    next = scheduler();
    if (next) {
      next->quantum = QUANTUM;
      next->status = running;
      // printf("VOU PARA A TASK %d\n", next->id);
      // queue_print("Ready list", (queue_t*)ready_q, &print_elem);
      // queue_print("Suspended list", (queue_t*)suspended_q, &print_elem);
      queue_remove((queue_t**) &ready_q, (queue_t*) next);
      task_switch (next);
    }

    if (sleeping_q) {
      task_t *node, *aux;

      for (node = sleeping_q; node->next != sleeping_q;) {
        aux  = node;
        node = node->next;
        if (systime() >= aux->wake_time) {
          aux->status = ready;        
          queue_remove((queue_t**) &sleeping_q, (queue_t*) aux);
          queue_append((queue_t**) &ready_q,    (queue_t*) aux);
        }
      }
      aux  = node;
      node = node->next;
      if (systime() >= aux->wake_time) {  
        aux->status = ready;    
        queue_remove((queue_t**) &sleeping_q, (queue_t*) aux);
        queue_append((queue_t**) &ready_q,    (queue_t*) aux);
      }
    }
  }
  // puts("joia");
  task_exit(1);
}

// Devolve uma tarefa para o final da fila de prontas e 
// devolve o processador para o dispatcher
void task_yield () {
  current_task->status = ready;
  queue_append((queue_t**) &ready_q, (queue_t*) current_task);
  task_switch(&dispatcher);
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
  // main_task.status = ready;
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
  // ++userTasks;
  // queue_append((queue_t**) &ready, (queue_t*) &main_task);

  dispatcher.id = id_create();
  dispatcher.activations = 0;
  dispatcher.status = ready;
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

  main_task.status = running;
  current_task = &main_task;
  return;
}

int task_create (task_t *task, void (*start_routine)(void *),  void *arg) {
  task->id = id_create();
  task_setprio(task, 0);
  task->dynamic_prio = task->static_prio;
  task->start = global_time;
  task->activations = 0;
  task->waiting = NULL;
  task->status = ready;
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
  queue_append((queue_t**) &ready_q, (queue_t*) task);
  ++userTasks;
  return task->id;
}

int task_switch (task_t *task) {
  if (!task) 
    return -1;

  task_t *old_task = current_task;

  current_task = task;
  if (old_task->status == running)
    old_task->status = ready;
  current_task->status = running;
  current_task->activations++;
  // printf("Old task: %d // New task %d\n", old_task->id, current_task->id);
  // queue_print("Ready list", (queue_t*)ready, &print_elem);
  swapcontext(&old_task->context, &current_task->context);
  return 0;
}

void task_exit (int exitCode) {
  task_t *node, *aux;

  current_task->exit_code = exitCode;
  current_task->status = terminated;
  // queue_remove((queue_t**) &ready_q, (queue_t*) current_task);
  printf("Task %d exit: running time %u ms, cpu time %u ms, %u activations\n", 
            current_task->id, global_time - current_task->start, 
            current_task->cpu_time, current_task->activations);

  if (current_task != &dispatcher) {
    if (suspended_q) {
      // find out which task is waiting for current task
      for (node = suspended_q; node->next != suspended_q; ) {
        aux  = node;
        node = node->next;
        if (aux->waiting == current_task) {
          aux->waiting = NULL;
          aux->status = ready;
          queue_remove((queue_t**) &suspended_q, (queue_t*) aux);
          queue_append((queue_t**) &ready_q, (queue_t*) aux); 
        }
      }
      aux  = node;
      if (aux->waiting == current_task) {
        aux->waiting = NULL;
        aux->status = ready;
        queue_remove((queue_t**) &suspended_q, (queue_t*) aux);
        queue_append((queue_t**) &ready_q, (queue_t*) aux);  
      }     
    }
    // puts("depois do suspenso");
    queue_print("Ready list", (queue_t*)ready_q, &print_elem);
    queue_print("Suspended list", (queue_t*)suspended_q, &print_elem);
    --userTasks;
    printf("exit: task %d status: %d\n", current_task->id, current_task->status);
    task_switch(&dispatcher);
  }
  else {
    // puts("cabo");
    queue_print("Ready list", (queue_t*)ready_q, &print_elem);
    queue_print("Suspended list", (queue_t*)suspended_q, &print_elem);
    printf("exit: task %d status: %d\n", current_task->id, current_task->status);
    task_switch(&main_task);
  }
}

int task_id () {
  return current_task->id;
}

int task_join (task_t *task) {
  if (!task || task == current_task)
    return -1;
  if (task->status == terminated) {
    // puts("eh isto");
    // printf("status de %d: %d\n", task->id, task->status);
    return task->exit_code;
  }
  // puts("comasim?");
  // printf("status de %d: %d\n", task->id, task->status);

  current_task->waiting = task;
  printf("%d\n", task->id);
  current_task->status = suspended;
  // queue_remove((queue_t**) &ready, (queue_t*) current_task);
  queue_append((queue_t**) &suspended_q, (queue_t*) current_task);
  // queue_print("Ready list", (queue_t*)ready_q, &print_elem);
  // queue_print("Suspended list", (queue_t*)suspended_q, &print_elem);
  task_switch(&dispatcher);
  return task->exit_code;
}

void task_sleep (int t) {
  current_task->wake_time =  t + systime();
  current_task->status = sleeping;
  queue_append((queue_t**) &sleeping_q, (queue_t*) current_task);

  task_switch(&dispatcher);
}

// Semaphores
// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value) {
  if (!s)
    return -1;

  s->status  = up;
  s->counter = value;
  s->blocked = NULL;

  return 0;
}

// requisita o semáforo
int sem_down (semaphore_t *s) {
  if (!s || s->status == down)
    return -1;

  if (--s->counter < 0) {
    queue_append((queue_t**) &s->blocked, (queue_t*) current_task);
    task_yield();
  }
  return 0;
}

// libera o semáforo
int sem_up (semaphore_t *s) {
  if (!s || s->status == down) 
    return -1;

  ++s->counter;
  if (s->blocked != NULL) {
    queue_t *task = queue_remove((queue_t**) &s->blocked, (queue_t*) s->blocked);
    queue_append((queue_t**) &ready_q, (queue_t*) task);
  }
  return 0;
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {
  if (!s)
    return -1;
  
  s->status = down;
  if (s->blocked != NULL) {
    for (int i = s->counter; i < 0; ++i) {
      queue_t *task = queue_remove((queue_t**) &s->blocked, (queue_t*) s->blocked);
      queue_append((queue_t**) &ready_q, (queue_t*) task);
    }
  }
  return 0;
}