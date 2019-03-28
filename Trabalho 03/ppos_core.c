#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "ppos.h"

#define STACKSIZE 32768

int current_id=0;
int userTasks=0;
task_t *tcb, *ready, *current_task;
task_t main_task, dispatcher;

// ----------
// Tasks
int id_create () {
  int result = current_id;
  current_id++;
  return result;
}

void print_elem (void *ptr) {
   task_t *elem = ptr ;

   if (!elem)
      return ;
   printf ("<%d>", elem->id) ;
}

void ppos_init () {
  setvbuf (stdout, 0, _IONBF, 0);
  main_task.id = id_create();
  getcontext (&main_task.context);
  
  queue_append((queue_t**) &tcb, (queue_t*) &main_task);
  // queue_print("fila: ", (queue_t*)tcb, print_elem);
  current_task = &main_task;

  task_create(&dispatcher, dispatcher_body, NULL);
  queue_append((queue_t**) &tcb, (queue_t*) &dispatcher);
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

  queue_append((queue_t**) &tcb, (queue_t*) task);
  makecontext(&task->context, (void*)(*start_routine), 1, arg) ;
  // queue_print("fila: ", (queue_t*)tcb, print_elem);
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
  task_t *old_task = current_task;
  current_task = &main_task;
  swapcontext(&old_task->context, &current_task->context);
}

int task_id () {
  return current_task->id;
}

// ----------
// Scheduler
task_t *scheduler () {
  return queue_remove((queue_t**) &ready, (queue_t*) &ready);
}

// ----------
// Dispatcher
void dispatcher_body () {
  while ( userTasks > 0 ) {
    next = scheduler() ;  // scheduler é uma função
    if (next) {
      // ações antes de lançar a tarefa "next", se houverem
      task_switch (next) ; // transfere controle para a tarefa "next"
      // ações após retornar da tarefa "next", se houverem
    }
  }
  task_exit(0) ; // encerra a tarefa dispatcher
}

// Devolve uma tarefa para o final da fila de prontas e 
// devolve o processador para o dispatcher
void task_yeld () {
  task_t *old_task = current_task;
  current_task = &dispatcher;

  swapcontext(&old_task->context, &current_task->context);

  // No final, devolve a execucao para o main
}