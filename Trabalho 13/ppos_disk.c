// Codigo desenvolvido para a disciplina de Sistemas Operacionais por:
// GRR20116644 - Guilherme Lopes do Nascimento
// GRR20136059 - Matheus Rotondano de Camargo

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "ppos_disk.h"
#include "hard_disk.h"

#define TRUE  (1==1)
#define FALSE (!TRUE)

extern int userTasks;
extern queue_t *ready_q, *suspended_q;
queue_t *request_q, *waiting_disk_q;
extern task_t *current_task;
task_t disk_dispatcher;
disk_t disk;
struct sigaction disk_handler;
int sleeping_scheduler;

void signal_handler (int signal) {
  // puts("SINAL PORA");
  disk.signal = 1;
  disk.status = idle;
  task_switch(&disk_dispatcher);
}

void disk_dispatcher_body(void)
{
  while (TRUE) {
    sem_down(&disk.mutex);

    if ((disk.status == idle) && request_q) {
      request_t *next = (request_t*)queue_remove((queue_t**)&request_q, (queue_t*)request_q);
      disk_cmd(next->signal_type, next->block, next->buffer);
      disk.status = busy;
      free(next);
    }
    
    if (disk.signal) {
      // puts("SINAL KARAI");
      disk.signal = 0;
      task_t *task = (task_t*)queue_remove((queue_t**)&waiting_disk_q, (queue_t*)waiting_disk_q);
      queue_append((queue_t**)&ready_q, (queue_t*)task);
    }
    else {
      queue_remove((queue_t**)&ready_q,     (queue_t*)&disk_dispatcher);
      queue_append((queue_t**)&suspended_q, (queue_t*)&disk_dispatcher);
      sleeping_scheduler = 1;
    }

    sem_up(&disk.mutex);
    task_yield();    
  }
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {
  if (!numBlocks || !blockSize)
    return -1;

  if (disk_cmd(DISK_CMD_INIT, 0, NULL) == -1)
    return -1;

  *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);
  *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, NULL);
  if (*numBlocks == -1 || *blockSize == -1)
    return -1;
  request_q = NULL;
  waiting_disk_q = NULL;
  sleeping_scheduler = 1;
  sem_create(&disk.mutex, 1);
  disk.signal = 0;
  disk.status = idle;

  task_create(&disk_dispatcher, (void*)disk_dispatcher_body, NULL);
  task_setprio(&disk_dispatcher, -20);
  disk_dispatcher.type = kernel;
  // printf("Disk Dispatcher ID: %d\n", disk_dispatcher.id);
  --userTasks;
  queue_remove((queue_t**) &ready_q,     (queue_t*) &disk_dispatcher);
  queue_append((queue_t**) &suspended_q, (queue_t*) &disk_dispatcher);

  disk_handler.sa_handler = signal_handler;
  sigemptyset(&disk_handler.sa_mask);
  disk_handler.sa_flags = 0;
  if (sigaction(SIGUSR1, &disk_handler, 0) < 0) {
    perror("Erro em sigaction: ");
    exit(1);
  }

  current_task->type = user;

  return 0;  
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {
  if (!buffer)
    return -1;
  if (block > disk_cmd(DISK_CMD_DISKSIZE, 0, NULL))
    return -1;

  request_t *read_request = malloc(sizeof(request_t));
  if (read_request) {
    sem_down(&disk.mutex);

    current_task->type = kernel;
    read_request->prev = NULL;
    read_request->next = NULL;
    read_request->signal_type = DISK_CMD_READ;
    read_request->block = block;
    read_request->buffer = buffer;
    // puts("Criou o Request");
    queue_append((queue_t**)&request_q,      (queue_t*)read_request);
    queue_remove((queue_t**)&ready_q,        (queue_t*)current_task);
    queue_append((queue_t**)&waiting_disk_q, (queue_t*)current_task);
    // printf("Colocou %d na fila de espera de disco.\n", current_task->id);

    if (sleeping_scheduler) {
      // puts("Entra no if");
      sleeping_scheduler = 0;
      queue_remove((queue_t**)&suspended_q, (queue_t*)&disk_dispatcher);
      queue_append((queue_t**)&ready_q,     (queue_t*)&disk_dispatcher);
      // printf("Colocou %d na fila de prontos\n", disk_dispatcher.id);
    }
    current_task->type = user;

    sem_up(&disk.mutex);
    task_yield();
  }
  else
    return -1;

  return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
  if (!buffer)
    return -1;
  if (block > disk_cmd(DISK_CMD_DISKSIZE, 0, NULL))
    return -1;

  request_t *write_request = malloc(sizeof(request_t));
  if (write_request) {
    sem_down(&disk.mutex);

    current_task->type = kernel;
    write_request->prev = NULL;
    write_request->next = NULL;
    write_request->signal_type = DISK_CMD_WRITE;
    write_request->block = block;
    write_request->buffer = buffer;
    // puts("Criou o Request");
    queue_append((queue_t**)&request_q,      (queue_t*)write_request);
    queue_remove((queue_t**)&ready_q,        (queue_t*)current_task);
    queue_append((queue_t**)&waiting_disk_q, (queue_t*)current_task);
    // printf("Colocou %d na fila de espera de disco.\n", current_task->id);

    if (sleeping_scheduler) {
      // puts("Entra no if");
      sleeping_scheduler = 0;
      queue_remove((queue_t**)&suspended_q, (queue_t*)&disk_dispatcher);
      queue_append((queue_t**)&ready_q,     (queue_t*)&disk_dispatcher);
      // printf("Colocou %d na fila de prontos\n", disk_dispatcher.id);
    }
    current_task->type = user;

    sem_up(&disk.mutex);
    task_yield();
  }
  else
    return -1;

  return 0;
}
