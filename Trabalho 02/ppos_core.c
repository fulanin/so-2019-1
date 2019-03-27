#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "ppos.h"

#define STACKSIZE 32768

int current_id=0;
task_t *tcb, *current_task;
task_t main_task;

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
  // setvbuf (stdout, 0, _IONBF, 0);
  main_task.id = id_create();
  getcontext (&main_task.context);
  
  queue_append((queue_t**) &tcb, (queue_t*) &main_task);
  // queue_print("fila: ", (queue_t*)tcb, print_elem);
  current_task = &main_task;
}

// int id_create () {
//   task_t *first = (task_t*)tcb;
//   srand(time(NULL));
//   int r = (rand() % STACKSIZE) + 1;

//   // Empty tcb
//   if (first == NULL)
//     return r;

//   // tcb contains one element
//   if (first->next == first) {
//     while (first->id == r)
//       r = (rand() % STACKSIZE) + 1;
//     return r;
//   }

//   for (task_t* node = first; node->next != first; node = node->next) {
//     if (node->id == r) {
//       r = (rand() % STACKSIZE) + 1;
//       node = first;
//     }
//     if (node->next->next == first) {
//       if (node->next->id == r) {
//         r = (rand() % STACKSIZE) + 1;
//         node = first;
//       }
//     }
//   }
//   return r;
// }

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