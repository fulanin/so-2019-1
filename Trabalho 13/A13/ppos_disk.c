#include "ppos_disk.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "hard_disk.h"

typedef struct request_t
{
  struct request_t *prev,
                   *next;
  int job_type;
  int block;
  void *buffer;
}request_t;

extern int user_tasks;

extern queue_t *ready_queue,
               *suspended_queue;
queue_t *requests,
        *waiting_disk;

extern task_t *current_task;
task_t disk_dispatcher;

int sleeping_scheduler;
semaphore_t disk_mutex;

int disk_interruption;

disk_t disk;
struct sigaction disk_handler;

void job_handler(int signal)
{
  puts("SINAL PORA");
  disk_interruption = 1;
  disk.busy = 0;
  task_switch(&disk_dispatcher);
}

void disk_dispatch(void)
{
  while (1)
  {
    sem_down(&disk_mutex);

    if (!disk.busy && requests)
    {
      #ifdef DEBUG_DISK
      printf("Disk free.\n");
      #endif

      request_t *next = (request_t*)queue_remove(&requests, requests);

      #ifdef DEBUG_DISK
      printf("Next job: %d on block %d.\n", next->job_type, next->block);
      #endif

      disk_cmd(next->job_type, next->block, next->buffer);
      disk.busy = 1;
      free(next);
    }

    if (disk_interruption)
    {
      puts("SINAL KARAI");
      #ifdef DEBUG_DISK
      printf("Interruption occured.\n");
      #endif

      disk_interruption = 0;
      task_t *task = (task_t*)queue_remove(&waiting_disk, waiting_disk);
      queue_append(&ready_queue, (queue_t*)task);

      #ifdef DEBUG_DISK
      printf("Appended task %d to ready queue.\n", task->id);
      #endif
    }
    else
    {
      #ifdef DEBUG_DISK
      printf("No interruption occured.\n");
      #endif

      queue_remove(&ready_queue, (queue_t*)&disk_dispatcher);
      queue_append(&suspended_queue, (queue_t*)&disk_dispatcher);
      sleeping_scheduler = 1;

      #ifdef DEBUG_DISK
      printf("Returning to sleep...\n");
      #endif
    }

    #ifdef DEBUG_DISK
    printf("Exiting...\n");
    #endif

    sem_up(&disk_mutex);

    task_yield();
  }
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
  if (!numBlocks || !blockSize)
    return -1;

  current_task->kernel_task = 1;

  if (disk_cmd(DISK_CMD_INIT, 0, NULL) < 0
        || (*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL)) < 0
        || (*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, NULL)) < 0)
  {
    current_task->kernel_task = 0;
    return -1;
  }

  requests = NULL;
  waiting_disk = NULL;
  sleeping_scheduler = 1;
  sem_create(&disk_mutex, 1);

  disk_interruption = 0;
  disk.busy = 0;
  task_create(&disk_dispatcher, (void*)disk_dispatch, NULL);
  printf("Disk Dispatcher ID: %d\n", disk_dispatcher.id);
  disk_dispatcher.kernel_task = 1;
  disk_dispatcher.static_priority = -20;
  --user_tasks;
  queue_remove(&ready_queue, (queue_t*)&disk_dispatcher);
  queue_append(&suspended_queue, (queue_t*)&disk_dispatcher);

  disk_handler.sa_handler = job_handler;
  sigemptyset(&disk_handler.sa_mask);
  disk_handler.sa_flags = 0;
  if (sigaction(SIGUSR1, &disk_handler, 0) < 0)
  {
    perror("Erro em sigaction: ");
    exit(1);
  }

  current_task->kernel_task = 0;

  return 0;
}

int disk_block_read(int block, void* buffer)
{
  request_t *read_request = malloc(sizeof(request_t));
  if (read_request)
  {
    sem_down(&disk_mutex);
    current_task->kernel_task = 1;

    read_request->prev = NULL;
    read_request->next = NULL;
    read_request->job_type = DISK_CMD_READ;
    read_request->block = block;
    read_request->buffer = buffer;
    queue_append(&requests, (queue_t*)read_request);

    #ifdef DEBUG_DISK
    printf("Suspending task.\n");
    #endif

    queue_remove(&ready_queue, (queue_t*)current_task);
    queue_append(&waiting_disk, (queue_t*)current_task);

    if (sleeping_scheduler)
    {
      #ifdef DEBUG_DISK
      printf("Waking dispatcher.\n");
      #endif

      sleeping_scheduler = 0;
      queue_remove(&suspended_queue, (queue_t*)&disk_dispatcher);
      queue_append(&ready_queue, (queue_t*)&disk_dispatcher);
    }

    current_task->kernel_task = 0;
    sem_up(&disk_mutex);

    task_yield();
  }
  else
  {
    return -1;
  }

  return 0;
}

int disk_block_write(int block, void* buffer)
{
  request_t *write_request = malloc(sizeof(request_t));
  if (write_request)
  {
    sem_down(&disk_mutex);
    current_task->kernel_task = 1;

    write_request->prev = NULL;
    write_request->next = NULL;
    write_request->job_type = DISK_CMD_WRITE;
    write_request->block = block;
    write_request->buffer = buffer;
    queue_append(&requests, (queue_t*)write_request);

    #ifdef DEBUG_DISK
    printf("Suspending task.\n");
    #endif

    queue_remove(&ready_queue, (queue_t*)current_task);
    queue_append(&waiting_disk, (queue_t*)current_task);

    if (sleeping_scheduler)
    {
      #ifdef DEBUG_DISK
      printf("Waking dispatcher.\n");
      #endif

      sleeping_scheduler = 0;
      queue_remove(&suspended_queue, (queue_t*)&disk_dispatcher);
      queue_append(&ready_queue, (queue_t*)&disk_dispatcher);
    }

    current_task->kernel_task = 0;
    sem_up(&disk_mutex);

    task_yield();
  }
  else
  {
    return -1;
  }

  return 0;
}