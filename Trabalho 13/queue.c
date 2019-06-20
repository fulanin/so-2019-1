// Codigo desenvolvido para a disciplina de Sistemas Operacionais por:
// GRR20116644 - Guilherme Lopes do Nascimento
// GRR20136059 - Matheus Rotondano de Camargo

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "queue.h"

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem) {
  queue_t **first = queue;
  queue_t *node;

  if (queue == NULL) {
    fprintf(stderr, "Queue is NULL, aborting.\n");
    return;
  }
  if (elem == NULL) {
    fprintf(stderr, "Element is  NULL, aborting.\n");
    return;
  }

  // Element is isoleted (not in any other queue)
  if (elem->next == NULL && elem->prev == NULL) {
    // Empty queue // Non-empty queue
    if ((*queue) == NULL) {
      // printf("Inserindo elemento em uma fila vazia");
      (*queue) = elem;
      (*queue)->next = (*queue);
      (*queue)->prev = (*queue);
    } else {
      // printf("Inserindo elemento em uma fila populada");
      (*first)->prev = elem;
      elem->next = (*first);
      for (node = (*first); node->next != (*first); node = node->next);
      node->next = elem;
      elem->prev = node;
    }
  }
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem) {
  queue_t **first = queue;
  queue_t *node;

  // There is no queue
  if (queue == NULL) {
    fprintf(stderr, "Queue is NULL, aborting.\n");
    return NULL;
  }

  // The queue is empty
  if ((*queue) == NULL) {
    fprintf(stderr, "Queue is empty, aborting.\n");
    return NULL;
  }

  // There is no element
  if (elem == NULL) {
    fprintf(stderr, "Element is  NULL, aborting.\n");
    return NULL;
  }

  // Queue with only one element
  if ((*first)->next == *(first) && (*first)->prev == *(first)) {
    if (elem == (*first)) {      
      (*queue) = NULL;
      elem->next = NULL;
      elem->prev = NULL;
      return elem;
    }
  }

  for (node = (*first); node->next != (*first); node = node->next) {
    if (elem == node) {      
      if (elem == (*first))
        (*queue) = node->next;
      node->prev->next = node->next;
      node->next->prev = node->prev;
      node->next = NULL;
      node->prev = NULL;
      return node;
    }
  }

  // Try the remaining element on the queue
  if (elem == node) {    
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    return node;
  }

  // The element is not contained in the queue
  return NULL;
}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {
  int count = 0;
  queue_t *i;
  queue_t *first = queue;

  if (queue == NULL)
    return count;
  ++count;
  for (i = first; i->next != first; i = i->next, ++count);
  return count;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca.
//
// Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
  queue_t *node;
  queue_t *first = queue;

  printf("%s ", name);
  if (queue == NULL) {
    // fprintf(stderr, "Fila vazia.\n");
    printf("[]\n");
    return;
  }

  printf("[");
  for (node = first; node->next != first; node = node->next) {
    print_elem(node);
    printf(" ");
  }
  print_elem(node);
  printf("]\n");
}