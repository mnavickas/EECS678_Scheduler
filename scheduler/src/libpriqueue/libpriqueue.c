/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

/**
  Initializes the priqueue_t data structure.

  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, compare_func_t comparer)
{
	q->queue_array = malloc (2 * sizeof(void*) );
	q->max_size = 2;
	q->curr_size = 0;
	q->comparer = comparer;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{

	if( q->curr_size+1 >= q->max_size )
	{
		int new_size = 2 * q->max_size;
		q->queue_array = realloc(q->queue_array, new_size * sizeof(void*) );
		q->max_size = new_size;
	}
	uint curr_idx = q->curr_size;

	//put it in the back
	q->queue_array[q->curr_size++] = ptr;

	//sift it forward
	//we aren't being paid to be efficient. so...
	while( curr_idx > 0 && 0 < q->comparer(q->queue_array[curr_idx-1], q->queue_array[curr_idx]) )
	{
		void *temp = q->queue_array[curr_idx-1];
		q->queue_array[curr_idx-1] = q->queue_array[curr_idx];
		q->queue_array[curr_idx] = temp;
		curr_idx--;
	}

	return curr_idx;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	return q->queue_array[0];
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	if(q->curr_size == 0 )
	{
		return NULL;
	}
	q->curr_size--;
	void *rtn = q->queue_array[0];
	int idx = 0;
	while( idx < q->curr_size)
	{
		q->queue_array[idx] = q->queue_array[idx+1];
		idx++;
	}
	return rtn;
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	if( index >= q->max_size )
	{
		return NULL;
	}
	return q->queue_array[index];
}


/**
  Removes all instances of ptr from the queue.

  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	int idx = 0;
	int removed = 0;

	while( idx < q->curr_size )
	{
		if ( q->queue_array[idx] == ptr )
		{
			removed++;
			q->curr_size--;
			//shift all foward
			int temp = idx;
			while( temp < q->curr_size )
			{
				q->queue_array[temp] = q->queue_array[temp+1];
				temp++;
			}
		}
		else
		{
			idx++;
		}
	}
	return removed;
}

/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	void *data = q->queue_array[index];

	int idx = index;
	while(idx < q->curr_size)
	{
		q->queue_array[idx] = q->queue_array[idx+1];
		idx++;
	}

	return data;
}

/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->curr_size;
}

/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	free( q->queue_array );
}
