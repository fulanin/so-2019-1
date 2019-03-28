#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "ppos.h"

#define STACKSIZE 32768

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

}