// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct spinlock locks[NBUFBUCKET];
  struct buf buf[NBUF];
  struct buf bucket[NBUFBUCKET];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUFBUCKET; i++) {
     //initsleeplock(&bcache.bucket[i].lock, "bcache.bucket");
     initlock(&bcache.locks[i], "bcache.bucket");
     bcache.bucket[i].next = &bcache.bucket[i];
     bcache.bucket[i].prev = &bcache.bucket[i];
  }

  // Create linked list of buffers
  /*
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  */

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->timestamp = ticks;
    b->refcnt = 0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *bucket = &bcache.bucket[blockno % NBUFBUCKET];
  uint time = ~0;

  //acquire(&bcache.lock);
  acquire(&bcache.locks[blockno % NBUFBUCKET]);

  // Is the block already cached?
  for(b = bucket->next; b != bucket; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[blockno % NBUFBUCKET]);
      //release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  //for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  acquire(&bcache.lock);
  for(struct buf *t = bcache.buf; t < bcache.buf+NBUF; t++){
     if (t->refcnt == 0 && t->timestamp <= time) {
        b = t;
        time = t->timestamp;
     }
     /*
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      b->next = bucket->next;
      b->prev = bucket->next->prev;
      bucket->next->prev = b;
      bucket->next = b;
      releasesleep(&bucket->lock);
      //release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
    */
  }
  if(b && b->refcnt == 0) {
     if (b->blockno != 0) {
        if (b->blockno % NBUFBUCKET != blockno % NBUFBUCKET) 
           acquire(&bcache.locks[b->blockno % NBUFBUCKET]);
        b->next->prev = b->prev;
        b->prev->next = b->next;
        if (b->blockno % NBUFBUCKET != blockno % NBUFBUCKET) 
           release(&bcache.locks[b->blockno % NBUFBUCKET]);
     }
     b->dev = dev;
     b->blockno = blockno;
     b->valid = 0;
     b->refcnt = 1;
     b->next = bucket->next;
     b->prev = bucket;
     bucket->next->prev = b;
     bucket->next = b;
     release(&bcache.lock);
     release(&bcache.locks[blockno % NBUFBUCKET]);
     acquiresleep(&b->lock);
     return b;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  //uint bid = b->blockno % NBUFBUCKET;
  //struct buf* bucket = &bcache.bucket[b->blockno % NBUFBUCKET];  

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  //acquire(&bcache.locks[bid]);
  b->refcnt--;
  b->timestamp = ticks;
  /*
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //acquiresleep(&bucket->lock);
    b->next->prev = b->prev;
    b->prev->next = b->next;
    //releasesleep(&bucket->lock);
  }
  
  //release(&bcache.lock);
  release(&bcache.locks[bid]);
  */
}

void
bpin(struct buf *b) {
  //acquire(&bcache.lock);
  uint bid = b->blockno % NBUFBUCKET;
  acquire(&bcache.locks[bid]);
  b->refcnt++;
  release(&bcache.locks[bid]);
  //release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  //acquire(&bcache.lock);
  uint bid = b->blockno % NBUFBUCKET;
  acquire(&bcache.locks[bid]);
  b->refcnt--;
  release(&bcache.locks[bid]);
  //release(&bcache.lock);
}


