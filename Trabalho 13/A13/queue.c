#include "queue.h"

#include <stdio.h>

void queue_append(queue_t **queue, queue_t *elem)
{
  if (!queue)
  {
    printf("ERRO append: queue � um ponteiro vazio\n");
    return;
  }

  if (!elem)
  {
    printf("ERRO append: elem � um ponteiro vazio\n");
    return;
  }

  if (elem->prev != NULL || elem->next != NULL)
  {
    printf("ERRO append: elem est� em outra fila\n");
    return;
  }

  if (!*queue)
  {
    elem->prev = elem;
    elem->next = elem;
    *queue = elem;

    return;
  }

  elem->prev = (*queue)->prev;
  elem->next = *queue;

  (*queue)->prev = elem;
  elem->prev->next = elem;
}

queue_t* queue_remove(queue_t **queue, queue_t *elem)
{
  if (!queue)
  {
    printf("ERRO remove: queue � um ponteiro vazio\n");
    return NULL;
  }

  if (!*queue)
  {
    printf("ERRO remove: fila est� vazia\n");
    return NULL;
  }

  if (!elem)
  {
    printf("ERRO remove: elem � um ponteiro vazio\n");
    return NULL;
  }

  int pass = 0;
  for (queue_t *current = *queue; pass < 1; current = current->next)
  {
    if (current->next == *queue)
      ++pass;

    if (current == elem)
    {
      if (current->prev == current && current->next == current)
      {
        *queue = NULL;
      }
      else
      {
        if (current == *queue)
          *queue = current->next;

        current->prev->next = current->next;
        current->next->prev = current->prev;
      }

      current->prev = NULL;
      current->next = NULL;

      return current;
    }
  }

  return NULL;
}

int queue_size(queue_t *queue)
{
  if (!queue)
  {
    printf("ERRO size: queue � um ponteiro vazio\n");
    return 0;
  }

  int pass = 0,
      elements_count = 0;
  for (queue_t *current = queue; pass < 1; current = current->next)
  {
    if (current->next == queue)
      ++pass;

    ++elements_count;
  }

  return elements_count;
}

void queue_print(char *name, queue_t *queue, void print_elem(void*))
{
  if (!queue)
  {
    printf("ERRO print: queue � um ponteiro vazio\n");
    return;
  }

  int pass = 0;

  printf ("%s : [", name) ;
  for (queue_t *current = queue; pass < 1; current = current->next)
  {
    if (current->next == queue)
      ++pass;

    print_elem((void*)current);
  }
  printf ("]\n") ;
}
