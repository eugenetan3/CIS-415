#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>

#define RANDOMDIVISOR	100000000
#define TRUE 1
#define	ENTRYLENGTH	80	// entry length
#define	NUMENTRIES	100	// entry length
#define	BUFFERSIZE	100	// buffer size
#define NUMBUFFERS	10	// number of buffers
#define MAXPUBS		50	// maximum number of publishers
#define MAXSUBS		50	// maximum number of subscribers

int	numpubs, numsubs;	// number of publishers and subscribers
int	numbuffers;		// number of buffers
int	sleepytime;		// sleep time for main thread

struct entry {
  int			id;
  struct timeval	timestamp;
  char			value[ENTRYLENGTH];
};

struct buffer {
  int		count, head, tail, inserted;
  struct entry	*entries;
};

struct buffer	buffers[NUMBUFFERS];	// buffers
pthread_t	pubs[MAXPUBS];		// thread ID for publishers
pthread_t	subs[MAXSUBS];		// thread ID for subscribers
pthread_attr_t	attr;			// thread attributes

pthread_mutex_t mutex[NUMBUFFERS];	// mutex locks for buffers
sem_t		full[NUMBUFFERS];	// full semaphores
sem_t		empty[NUMBUFFERS];	// empty semaphores

struct threadargs {
  int	id;
};
struct threadargs	pubargs[MAXPUBS];
struct threadargs	subargs[MAXPUBS];

void *publisher(void *param);		// publisher routine
void *subscriber(void *param);		// subscriber routine

void initialize() {
int i, j, k;

  // create the buffers
 for (i=0; i<NUMBUFFERS, i++) {
    buffers[i].count = 0;	// # entries in buffer now
    buffers[i].head = 0;	// head index
    buffers[i].tail = 0;	// tail index
    buffers[i].inserted = 0;	// # entries inserted in buffer overall
    buffers[i].entries = (struct entry *) malloc(sizeof(struct entry) * NUMENTRIES);
  }

  // create the buffer semaphores
  for (i=0; i<NUMBUFFERS; i++) {
    pthread_mutex_init(&(mutex[i]), NULL);
    //    sem_init(&full[i], 0, 0);
    //    sem_init(&empty[i], 0, BUFFERSIZE);
  }

  pthread_attr_init(&attr);

} // initialize()

int enqueue(int buffid) {
  
  if (buffers[buffid].count == BUFFERSIZE) {
    return(-1);		// buffer is full
  }
  buffers[buffid].count++;
  buffers[buffid].inserted++;
  buffers[buffid].tail = (buffers[buffid].tail +1) % BUFFERSIZE;
  return(0);

} // enqueue()

int dequeue(int buffid) {
  
  if (buffers[buffid].count == 0) {
    return(-1);		// buffer is empty
  }
  buffers[buffid].count--;
  buffers[buffid].head = (buffers[buffid].head +1) % BUFFERSIZE;
  return(0);

} // dequeue()

void *publisher(void *args) {
  int	bid, tid;	// buffer id and thread id
  int	randomnum;	// random number
  
  tid = ((struct threadargs *) args)->id;
  fprintf(stdout, "publisher thread id = %d, %ld\n", tid, pthread_self());
  while(TRUE) {
    for (bid=0; bid<numbuffers; bid++) {
      pthread_mutex_lock(&(mutex[bid]));
      if (enqueue(bid) == -1) {
	fprintf(stdout, "publisher %d: buffer %d is full, inserted = %d\n",
		tid, bid, buffers[bid].inserted);
	pthread_mutex_unlock(&(mutex[bid]));
	sleep(1);
      }
      else {
	pthread_mutex_unlock(&(mutex[bid]));
      }
    }
  }

} // publisher()

void *subscriber(void *args) {
  int	bid, tid;	// buffer id and thread id
  int	randomnum;	// random number
  
  tid = ((struct threadargs *) args)->id;
  fprintf(stdout, "subscriber thread id = %d, %ld\n", tid, pthread_self());
  while(TRUE) {
    for (bid=0; bid<numbuffers; bid++) {
      pthread_mutex_lock(&(mutex[bid]));
      if (dequeue(bid) == -1) {
	fprintf(stdout, "subscriber %d: buffer %d is empty, inserted = %d\n",
		tid, bid, buffers[bid].inserted);
	pthread_mutex_unlock(&(mutex[bid]));
	sleep(1);
      }
      else {
	pthread_mutex_unlock(&(mutex[bid]));
      }
    }
  }

} // subscriber()

int main(int argc, char *argv[]) {
int i;

  if(argc != 5) {
    fprintf(stderr, "USAGE: pub-sub-circular <INT> <INT> <INT> <INT>\n");
    exit(1);
  }

  sleepytime = atoi(argv[1]);	// time in seconds for main to sleep
  numpubs = atoi(argv[2]);	// number of publishers
  numsubs = atoi(argv[3]);	// number of subscribers
  numbuffers = atoi(argv[4]);	// number of buffers

  fprintf(stdout, "sleepytime = %d, numpubs = %d, numsubs = %d\n",
	  sleepytime, numpubs, numsubs);

  initialize();

  // create the producer threads
  for(i = 0; i < numpubs; i++) {
    pubargs[i].id = i;
    pthread_create(&pubs[i], &attr, publisher, (void *) &pubargs[i]);
  }
  // create the subscriber threads
  for(i = 0; i < numsubs; i++) {
    subargs[i].id = i;
    pthread_create(&subs[i], &attr, subscriber, (void *) &subargs[i]);
  }

  /* Sleep for the specified amount of time in milliseconds */
  sleep(sleepytime);

  printf("main() exiting the program\n");
  exit(0);

} // main()
