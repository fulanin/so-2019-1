// Codigo desenvolvido para a disciplina de Sistemas Operacionais por:
// GRR20116644 - Guilherme Lopes do Nascimento
// GRR20136059 - Matheus Rotondano de Camargo

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

enum sem_status {up, down};
enum task_status {created, ready, running, waiting, suspended, sleeping, terminated};

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em 
   unsigned int start, cpu_time, activations;
   int id, exit_code;				// identificador da tarefa
   int static_prio, dynamic_prio;
   int quantum, wake_time;
   struct task_t *waiting;
   enum task_status status;
   ucontext_t context ;			// contexto armazenado da tarefa
   void *stack ;			// aponta para a pilha da tarefa
   // ... (outros campos serão adicionados mais tarde)
} task_t;

// estrutura que define um semáforo
typedef struct
{
  int counter;
  enum sem_status status;
  queue_t *blocked;
} semaphore_t ;

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
  void *msgs; // fila de mensagens do tipo void*
  // void *msgs[512];
  int head, tail;
  int max_msgs, msg_size, cur_size;
  semaphore_t empty, full, mutex;
  enum sem_status status;
} mqueue_t ;

#endif

