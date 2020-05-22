# **LFQ - lockfree queue**

The current implementation of a **lock-free queue** is based on the paper
and sample code of Ruslan Nikolaev. See https://github.com/rusnikola/lfqueue
for details. The paper describes various different methods.

The code of lpq is based on the scqd method and assembled, so that it can
be easily used in your own project, without adding new libraries. See the **example.c** code for its usage.

A lot has been written about lock-free queues and their implementations.
It's a rather tricky topic to get it right. Many still lack a proper
implementation.

## lfq usage:

Copy the files in the **src** directory into your project folder. 
Include the header in your project:

`#include <scqd.h>`

`queue_t *lfq_init(size_t order);`

Initialize a new queue. The size of the queue is 2^order. Returns: 
Pointer to new queue.  

`void *lfq_enqueue(queue_t *q, void *val);`  

Puts the object *val into the queue. Returns:  
**LFQ_OK** if successful  
**LFQ_FULL** if unsuccessful  
**LFQ_CLOSED** if queue is closed.  

`void *lfq_dequeue(queue_t *q);`

Gets an object from the queue. Returns:  
The object, if successful  
**LFQ_EMPTY** if no objects are available  
**LFQ_CLOSED** if queue is closed.  

`void lfq_close(queue_t *q);`

Closes the queue. No more objects can be put in the queue.
Remaining objects can be retrieved, until the queue is empty.

`void lfq_free(queue_t *q);`

Frees the memory from the queue.

