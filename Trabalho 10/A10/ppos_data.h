// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev,
                 *next ;  // ponteiros para usar em filas
   int id ;  // identificador da tarefa
   ucontext_t context ;  // contexto armazenado da tarefa
   void *stack ;  // aponta para a pilha da tarefa
   int state;  // ... (outros campos serão adicionados mais tarde)
   int static_priority,
       dinamic_priority;
   int kernel_task;
   unsigned int creation_time,
                processor_time,
                activations;
   int exit_code;
   struct task_t *waiting_for;
   unsigned int wake_time;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
	int valid;
	int locks;
	queue_t *blocked;
}semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

