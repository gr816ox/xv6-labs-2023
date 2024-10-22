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


static char bucketname[NBUCKET][16];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct spinlock bucket_lock[NBUCKET];
  struct buf bucket_head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  // printf("start binit\n");
  initlock(&bcache.lock, "bcache");
  int bucketsize = NBUF / NBUCKET;
  int restsize = NBUF % NBUCKET;
  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; i++){
    snprintf(bucketname[i], sizeof(bucketname[i]), "bcachebucket%d", i);
    initlock(&bcache.bucket_lock[i], bucketname[i]);
    // printf("init bucket_lock[%d]\n",i);

    b = &bcache.bucket_head[i];
    for (int j = 0; j < bucketsize && (j + bucketsize * i < NBUF); j++){
      b->next = &bcache.buf[j + bucketsize * i];
      // bcache.buf[j + bucketsize * i].prev = b;
      b = b->next;      
    }
    if (restsize > 0){
      b->next = &bcache.buf[NBUF - restsize];
      // bcache.buf[NBUF - restsize].prev = b;
      restsize--;
    }
    b->next = 0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *p;
  int bucketidx = blockno % NBUCKET;

  // acquire(&bcache.lock);
  acquire(&bcache.bucket_lock[bucketidx]);

  // Is the block already cached?
  for(b = bcache.bucket_head[bucketidx].next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_lock[bucketidx]);
      // release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  //dummy version
  for (b = bcache.bucket_head[bucketidx].next; b != 0; b = b->next){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucket_lock[bucketidx]);
      // release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // release(&bcache.bucket_lock[bucketidx]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (int i = 0; i < NBUCKET; i++)
  {
    if (i == bucketidx)
      continue;
    acquire(&bcache.bucket_lock[i]);
    for (b = bcache.bucket_head[i].next; b->next != 0; b = b->next){
      if(b->next->refcnt == 0) {
        // acquire(&bcache.bucket_lock[bucketidx]);
        // printf("asdfasdfasdfsadf\n");
        for (p = bcache.bucket_head[bucketidx].next; p->next != 0; p = p->next){
          // printf("p");
        }
          ;
        p->next = b->next;
        b->next = b->next->next;
        b = p->next;
        b->next = 0;
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.bucket_lock[bucketidx]);
        release(&bcache.bucket_lock[i]);
        // release(&bcache.lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bucket_lock[i]);
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


