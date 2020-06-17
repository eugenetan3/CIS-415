/*
Author: Eugene Tan
Professor: Allen Malony
Course: CIS 415
Date: 6/4/2020
*/


#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>


#define URLSIZE 200 //Maximum length a URL can be within topicEntry
#define CAPSIZE 200 //Maximum caption length a caption can be within topicEntry
#define MAXTOPICS 10 //Maximum number of topics that can be created and stored
#define NAMESIZE 20 //Maximum length of a topic's name
#define MAXPUBS 20 //Maximum publisher files that can be given
#define MAXSUBS 20 //Maximum subscriber files that can be given
#define FILEBUFFER 200 //File buffer with arbritrarily large size -- maxpubs and maxsubs limit the size
#define NUMPROXIES 4 //Number of total proxy threads, half are dedicated to subscribers and the other half to publishers.

//Global conditional variables to break out of thread conditions.
int termination = 0;
int breakLoop = 0;
int breakLoop2 = 0;

//Pthread attribute variable.
pthread_attr_t attr;

//topicEntry structure to hold photoURL and photoCaptions and relevant info
typedef struct topicEntry {
  int entryNum;
  struct timeval timeStamp; //Struct to check time and age
  long int pubID; //publisher ID
  char photoURL[URLSIZE]; //photoURL
  char photoCaption[CAPSIZE]; //photoCaption
} topicEntry;

//topicQueue structure to hold an array of topicEntry items and ensure access is thread-safe, rounded-buffer.
typedef struct topicQueue {
  int qid;
  char name[NAMESIZE];
  int entryCounter;
  int head;
  int tail;
  int length;
  int maxlength;
  topicEntry *entries;
  pthread_mutex_t lock;
} topicQueue;

//htmlFile struct to keep track and store all entries that are retrieved to later be made into an html file
typedef struct htmlFile {
  int index; //Number of items
  topicEntry *store; //Array of topicEntry per topic
} htmlFile;

//htmlRegistry struct to store an array of htmlFile structs to store all getentry items into a table for later use
typedef struct htmlRegistry {
  htmlFile storage[MAXTOPICS]; //Array of htmlFile queue structs
} htmlRegistry;

//threadargs structure to pass information to and from threads to mainthread
typedef struct threadargs {
  int free; //availability flag for threads
  int index;
  char *threadfile; //file passed
} threadargs;

threadargs tid[NUMPROXIES/2]; //Pool of thread arguments for publishers
threadargs pid[NUMPROXIES/2]; //Pool of thread arguments for subscribers

//proxyPool structure to keep track of items relevant to each pool
typedef struct proxyPool {
  pthread_t thread[NUMPROXIES/2];
  char *commands[FILEBUFFER];
  int cmdCount;
  int currentJob;
} proxyPool;

//topicRegistry structure to store all topicQueues created into the topic registry
typedef struct topicRegistry {
  int length;
  topicQueue topics[MAXTOPICS];
} topicRegistry;

topicRegistry registry = {0, }; //Initialization of the topic registry

/*
Function used to initialize a new topic queue and store it into the position
queue in memory. Creates a topic queue of specified length, with given name,
and specified queue ID. Initializes the mutex locks for each queue.

Args: id: topic Queue ID
      topicname: name of the queue
      maxlength: maximum length that queue can hold
      queue: address in memory to store this initialized struct
Return: 1 always
*/
int createTopic(int id, char *topicname, int maxlength, topicQueue *queue) {
  queue->qid = id;
  strcpy(queue->name, topicname);
  queue->maxlength = maxlength;
  queue->head = -1;
  queue->tail = -1;
  queue->entryCounter = 1;
  queue->length = 0;
  queue->entries = (topicEntry *)malloc(sizeof(topicEntry) * maxlength);
  pthread_mutex_init(&(queue->lock), NULL);
  pthread_attr_init(&attr);
  return 1;
}

/*
Enqueue capability to insert an item into a topic Queue, checks whether
or not a queue can handle the insertion into the queue and if successful
inserts the item and updates the queue's values.

Args: entry: item to be inserted
      queue: queue to be saved into topics

Return: 0 on failure, 1 on success
*/
int enqueue(topicEntry *entry, topicQueue *queue) {
  pthread_mutex_lock(&(queue->lock)); //Lock the queue so no other thread will modify the queue

  if (((queue->head == 0) && (queue->tail == queue->maxlength-1)) || (queue->head == queue->tail+1)) { //check if queue is full
    pthread_mutex_unlock(&queue->lock); //Unlock the queue
    return 0;
  }
  if (queue->head == -1) { //If queue is empty initialize queue values
    queue->head = 0;
    queue->tail = 0;
  } else {
    if (queue->tail == queue->maxlength-1) { //If queue's tail is at the end of the queue wrap it around
      queue->tail = 0;
    } else { //Increment the tail
      queue->tail = queue->tail + 1;
    }
  }
  //Assign the values into the respective topicEntry slot within the queue
    queue->entries[queue->tail].entryNum = queue->entryCounter;
    queue->entries[queue->tail].pubID = entry->pubID;
    gettimeofday(&(queue->entries[queue->tail].timeStamp), NULL);
    strcpy(queue->entries[queue->tail].photoURL, entry->photoURL);
    strcpy(queue->entries[queue->tail].photoCaption, entry->photoCaption);

    queue->length++;
    queue->entryCounter++;

    pthread_mutex_unlock(&(queue->lock)); //Unlock the queue

  return 1;
}

/*
Function used to retrieve an entry from a specified topicQueues
and using th epreviously retrieved index we try to retrieve the lastEntry
index + 1. Successful retrieval stores retrieved items into the specified
topicEntry item.

Args: lastentry: index we are trying to retrieve
      store: topicEntry to store results into
      queue: topicqueuee to read from

Return: 0 on failure and 1 on success
*/
int getEntry(int lastEntry, topicEntry *store, topicQueue *queue) {
  pthread_mutex_lock(&queue->lock); //Lock the queue

  if (queue->head == -1) { //If queue is empty return
    pthread_mutex_unlock(&queue->lock); //Unlock the queue
    return 0;
  }

  //Check whether or not an item in the queue has the desired index
  int flag = 0;
  int index;
  for (int i = 0; i < queue->length; i++) {
    index = (queue->head + i) % queue->maxlength;
    if (queue->entries[index].entryNum == lastEntry + 1) {
      flag = 1;
      break;
    }
  }

  //Item has been found within the queue with desired index so we store the
  //value into the entry we want retrieved info to be stord into.
  if (flag) {
    store->entryNum = queue->entries[index].entryNum;
    store->timeStamp = queue->entries[index].timeStamp;
    store->pubID = queue->entries[index].pubID;
    strcpy(store->photoURL, queue->entries[index].photoURL);
    strcpy(store->photoCaption, queue->entries[index].photoCaption);
  } else {
    //Could not locate it, so we try and find a more recent entry
    for (int i = 0; i < queue->length; i++) {
      index = (queue->head + i) % queue->maxlength;
      if (queue->entries[index].entryNum > lastEntry + 1) {
        flag = 1;
        break;
      }
    }

    if (flag == 0) { //Couldn't locate entry, return and unlock
      pthread_mutex_unlock(&queue->lock);
      return 0;
    } else { //Entry was found with more recent value so we assign retrieved
      //entry into the store topicEntry
      store->entryNum = queue->entries[index].entryNum;
      store->timeStamp = queue->entries[index].timeStamp;
      store->pubID = queue->entries[index].pubID;
      strcpy(store->photoURL, queue->entries[index].photoURL);
      strcpy(store->photoCaption, queue->entries[index].photoCaption);

      int entrynumber = queue->entries[index].entryNum;
      pthread_mutex_unlock(&queue->lock);
      return entrynumber; //Return new entrynumber if could not find it the first time but find the second
    }
  }
  pthread_mutex_unlock(&queue->lock); //Unlock the queue and return
  return 1;
}

/*
Remove an item or multiple items from a queue should the age of the entry
deduced from comparing the current time to the entries stored timeStamp be
larger than the specified delta. It will remove all items that satisfy the
criteria of age.

Args: queue: topicqueue to remove entries from
      delta: time value to remove entries by should they be older than delta

Return: 0 for failure and 1 for success
*/
int dequeue(topicQueue * queue, suseconds_t delta) {
  pthread_mutex_lock(&queue->lock); //Lock the queue being accessed

  if (queue->head == -1) { //If the queue is empty theres nothing to remove so return and unlock.
    pthread_mutex_unlock(&queue->lock);
    return 0;
  }

  //Loop through the queue oldest to newest and check to see if number of microseconds
  //are less than delta, upon the first entry to be less than the delta means that all
  //entries up until that index are > delta and thus should be removed.
  struct timeval currentTime;
  int index = 0;
  int i;
  for (i = 0; i < queue->length; i++) {
    index = (queue->head + i) % queue->maxlength;
    gettimeofday(&currentTime, NULL);
    long int cur_ms = (long int) (currentTime.tv_sec * 1000000 + currentTime.tv_usec);
    long int ts_ms = (long int) (queue->entries[index].timeStamp.tv_sec) * 1000000 + (long int) (queue->entries[index].timeStamp.tv_usec);
    long int diff = cur_ms - ts_ms;
    if ((suseconds_t) diff < delta) {
      break;
    }
  }
  //If the index breaks on the first iteration, there's nothing in the queue that can be dequeued
  if (i == 0) {
    pthread_mutex_unlock(&queue->lock); //unlock queue and return
    return 0;
  } else if (i == queue->length) { //If the loop iterated past it means everything can be removed
    queue->head = -1;
    queue->tail = -1;
    queue->length = 0;
  } else { //Remove only up until specified index and update length
    queue->head = index;
    queue->length -= i;
  }
  pthread_mutex_unlock(&queue->lock); //Unlock the queue and return
  return 1;
}


/*
Subscriber function to execute any passed thread file name and parse the file
and execute any functions specified within the parsed file.

args: args: structure that contains useful information to communicate with the main thread.
*/
void *subscriber(void *args) {
  struct threadargs *td = ((struct threadargs *)args);
  while (breakLoop2 == 0) { //While the global condition is fulfilled continue this thread
    if (td->threadfile != NULL) { //If a file has been passed it wont be null and thus will enter

      printf("Proxy Thread: %ld - type: Subscribers - Acquired Thread Command File: %s - Free Flag: %d\n", pthread_self(), td->threadfile, td->free);

      //Open thee passed threadfile and begin parsing the file.
      FILE *fptr = fopen(td->threadfile, "r");
      if (fptr != NULL) {
        char *buffer = NULL; //Variables for getline
        size_t bufsize = 0;

        char *saveptr; //Variables for tokenization
        char *token;

        int condition = 0;
        int topicid;
        int sleepnum;
        int lastEntry[registry.length]; //Lastentry array for subscribers to use to keep track of indexes read
        for (int i = 0; i < registry.length; i++) { //Initialize array
          lastEntry[i] = 0;
        }

        htmlRegistry htmlRegister; //Initialize a registry to hold obtained entries via getentry
        for (int i = 0 ; i < registry.length; i++) { //Initialization of the register
          htmlRegister.storage[i].index = 0;
          htmlRegister.storage[i].store = (topicEntry *)malloc(sizeof(topicEntry) * registry.topics[i].maxlength);
        }

        topicEntry dummy;
        while (condition == 0) { //Loop through and parse the file line by and line and execute commands.
          getline(&buffer, &bufsize, fptr); //Get the line
          token = strtok_r(buffer, " \n\t", &saveptr); //Tokenize
          if (strcmp(token, "get") == 0) { //get command case
            printf("Proxy thread: %ld - type: Subscribers - Executed command: %s\n", pthread_self(), token);
            token = strtok_r(NULL, " \n\t", &saveptr);
            topicid = atoi(token);

            //Determine which topic I should reference by comparing each topic's id until a match is detected
            int i = 0;
            for (i = 0; i < registry.length; i++) {
              if (registry.topics[i].qid == topicid) {
                break;
              }
            }
            //If queue id has iterated through the whole loop no id has matched the specified id.
            if (i == registry.length) {
              printf("Error! Could not retrieve an entry from provided topic ID: %d\n", topicid);
              break;
            }

            //Otherwise we have found a legitimate queue id and index.
            int result;
            result = getEntry(lastEntry[i], &dummy, &registry.topics[i]); //Call getentry
            //If result is 1 we increment lastentry by 1 since it got lastentry+1 entry
            if (result == 1) {
              printf("Proxy thread: %ld - type: Subscribers - Subscriber got Entry from topic ID: %d - entryNum: %d photoURL: %s photoCaption: %s \n", pthread_self(), topicid, dummy.entryNum, dummy.photoURL, dummy.photoCaption);
              lastEntry[i]++;
              strcpy(htmlRegister.storage[i].store[htmlRegister.storage[i].index].photoURL, dummy.photoURL);
              strcpy(htmlRegister.storage[i].store[htmlRegister.storage[i].index].photoCaption, dummy.photoCaption);
              htmlRegister.storage[i].index++;
            } else if (result > 1) {
              //If result is greater than one we increment last entry by the number specified since it got
              //a much newer entry.
              lastEntry[i] = lastEntry[i] + result;
              strcpy(htmlRegister.storage[i].store[htmlRegister.storage[i].index].photoURL, dummy.photoURL);
              strcpy(htmlRegister.storage[i].store[htmlRegister.storage[i].index].photoCaption, dummy.photoCaption);
              htmlRegister.storage[i].index++;
              printf("Proxy thread: %ld - type: Subscribers - Subscriber got Entry from topic ID: %d - entryNum: %d photoURL: %s photoCaption: %s \n", pthread_self(), topicid, dummy.entryNum, dummy.photoURL, dummy.photoCaption);
            } else {
              //Otherwise getentry failed by returning 0.
              printf("ERROR - Unable to get last entry from topic: %d\n", topicid);
            }

          } else if (strcmp(token, "sleep") == 0) { //Sleep command case to sleep thread for specified microseconds
            printf("Proxy thread: %ld - type: Subscribers - Executed command: %s\n", pthread_self(), token);
            token = strtok_r(NULL, " \n\t", &saveptr); //tokenize the value to sleep by and convert to an int.
            sleepnum = atoi(token);
            usleep(sleepnum); //Sleep the thread by the number specified.
          } else if (strcmp(token, "stop") == 0) { //Stop command case to stop all execution and generate html filee
            printf("Proxy thread: %ld - type: Subscribers - Executed command: %s\n", pthread_self(), token);

            //Find the name of the file by using thread's passed filename
            char tempFileName[BUFSIZ];
            strcpy(tempFileName, td->threadfile);
            char *token2;
            char *saveptr2;

            token2 = strtok_r(tempFileName, ".", &saveptr2); //Tokenize to remove the .txt

            char htmlFilePtr[BUFSIZ]; //Name buffer to read.
            strcpy(htmlFilePtr, token2); //Copy the new name without the .txt
            strcat(htmlFilePtr, ".html"); //Append a .html so it is now an html file.

            //Create an html file with specified subscriber filename and enter document format.
            FILE *htmlfptr = fopen(htmlFilePtr, "w");
            fputs("<!DOCTYPE html>\n<html>\n<head>\n<title>", htmlfptr);
            fputs(token2, htmlfptr);
            fputs("</title>\n\n", htmlfptr);
            fputs("<style>\ntable, th, td {\n\tborder: 1px solid black;\n\tborder-collapse: collapse;\n}\n", htmlfptr);
            fputs("th, td {\n\tpadding: 5px;\n}\nth {\n\ttext-align:left;\n}\n</style>\n\n</head>\n", htmlfptr);
            fputs("<body>\n\n<h1>Subscriber: ", htmlfptr);
            fputs(token2, htmlfptr);
            fputs(" </h1>\n\n", htmlfptr);

            for (int i = 0; i < registry.length; i++) {
              //If no entry was recorded within the htmlRegister we skip generating a table for it
              if (htmlRegister.storage[i].index == 0) {
                continue;
              }
              //Otherwise generate the appropriate table format


              fputs("<h2>Topic Name: ", htmlfptr);
              fputs(registry.topics[i].name, htmlfptr);
              fputs("</h2>\n\n", htmlfptr);

              fputs("<table style=\"width:100%\" align=\"middle\">\n", htmlfptr);
              fputs("\t<tr>\n\t\t<th>CAPTION</th>\n\t\t<th>PHOTO-URL</th>\n\t</tr>\n", htmlfptr);

              //Loop through acquired entries and populate the html table with the collected entries.
              for (int j = 0; j < htmlRegister.storage[i].index; j++) {
                fputs("\t<tr>\n\t\t<td>", htmlfptr);
                fputs(htmlRegister.storage[i].store[j].photoCaption, htmlfptr);
                fputs("</td>\n\t\t<td>", htmlfptr);
                fputs(htmlRegister.storage[i].store[j].photoURL, htmlfptr);
                fputs("</td>\n\t</tr>\n", htmlfptr);

              }
              fputs("\n</table>\n\n", htmlfptr);

              fputs("</body>\n</html>\n", htmlfptr);
            }
            fclose(htmlfptr); //Close the file and free allocated memory.
            for (int i = 0; i < registry.length; i++) {
              free(htmlRegister.storage[i].store);
            }
            condition = 1; //Break out of the while loop.
          }
        }

      free(buffer); //Free buffer used to read file and close the thread file.
      fclose(fptr);
      } else {
        printf("Error! Unable to read from invalid file, file passed cannot be read from.\n");
      }

      td->threadfile = NULL; //Assign the thread to be free again and threadfile to be NULL
      td->free = 1;
    }
  }
}

/*
Publisher function to execute any passed thread file name and parse the file
and execute any functions specified within the parsed file.

args: args: structure that contains useful information to communicate with the main thread.
*/
void *publisher(void *args) {
  struct threadargs *td = ((struct threadargs *)args);
  int tiid = td->index;

  //Keep thread running until conditional is met
  while(breakLoop == 0) {
    if (td->threadfile != NULL) { //A thread has been passed, thread is no longer free and begin to parse file
        printf("Publisher Thread: %ld - Thread Command File: %s - Free Flag: %d\n", pthread_self(), td->threadfile, td->free);

        FILE *fptr = fopen(td->threadfile, "r"); //Open file for reading
        if (fptr != NULL) {
          char *buffer = NULL;
          size_t bufsize = 0;

          char *saveptr;
          char *token;
          int topicid;
          char *photoURL;
          char *caption;

          topicEntry entry; //entry to be enqueued
          int weird = 0;
          while (!weird) { //Loop until conditional is met, read the file line by line and tokenize to determine cases
            getline(&buffer, &bufsize, fptr);
            token = strtok_r(buffer, " \n\t", &saveptr);
            if (strcmp(token, "put") == 0) { //Put case to enter an item into a queue
              printf("Proxy thread: %ld - type: Publishers - Executed command: %s\n", pthread_self(), token);
              token = strtok_r(NULL, " \n\t", &saveptr);
              topicid = atoi(token);

              int i = 0; //Determine which topic I should reference by comparing each topic's id until a match is detected
              for (i = 0; i < registry.length; i++) {
                if (registry.topics[i].qid == topicid) {
                  break;
                }
              }
              //If it iterates past the length it indicates that no queue was encountered with specified topic ID
              if (i == registry.length) {
                printf("Error! Could not find the designated topic ID.\n");
                break;
              }

              //Tokenize further to determine parameters for item desired to be input
              token = strtok_r(NULL, " \"\n\t", &saveptr); //Parse photoURL
              strcpy(entry.photoURL, token);
              token = strtok_r(NULL, " \"\n\t", &saveptr); //Parse photoCaption
              strcpy(entry.photoCaption, token);
              entry.pubID = pthread_self(); //Assign publisher ID
              int result = 0;
              //Loop through 15 times and try enqueuing the item into queue
              for (int j = 0; j < 15; j++) {
                result = enqueue(&entry, &registry.topics[i]);
                if (result) { //If the enqueue succeeded we dont try again
                  printf("Proxy thread: %ld - type: Publishers - Enqueued entry to topic Queue: %d - Entry Caption: %s - Entry URL: %s\n", pthread_self(), topicid, entry.photoCaption, entry.photoURL);
                  break;
                } else { //If the enqueue fails we sleep for 500 ms and then try again.
                  printf("Error: queue is full, unable to enqueue entry into topic: %d - Sleeping for 500 ms...\n", topicid);
                }
                usleep(500);
              }

            } else if (strcmp(token, "sleep") == 0) { //case for sleep command
              printf("Proxy thread: %ld - type: Publishers - Executed command: %s\n", pthread_self(), token);
              token = strtok_r(NULL, " \"\n\t", &saveptr); //acquire amount you'd like to sleep by
              int sleepnum = atoi(token); //convert sleep to integer
              usleep(sleepnum); //sleep thread by that number

            } else if (strcmp(token, "stop") == 0) { //case for the stop command
              printf("Proxy thread: %ld - type: Publishers - Executed command: %s\n", pthread_self(), token);
              weird = 1; //Break condition to kill the while loop
            }

          }
          free(buffer); //Free buffer memory
          fclose(fptr); //Close threadfile pointer
        } else {
          printf("Error: NULL file pointer passed, unable to access file specified.\n");
        }
        td->threadfile = NULL; //Set threadfile to be NULL and thread back to free
        td->free = 1;
    }
  }

}

/*
Clean up thread to be executed in the background and periodically clean each topic queue
should the timegap have been passed.

args: args: contains delta within the structure.
*/
void *cleanup(void *args) {
  suseconds_t *delta = (suseconds_t *) args;

  struct timeval currentTime; //current time
  struct timeval lastClean; //last clean time

  suseconds_t timeGap = 5000000; //amount of time needed to be passed in ms before dequeue can be called.

  gettimeofday(&lastClean, NULL); //acquire time
  gettimeofday(&currentTime, NULL); //acquire time

  while (!termination) { //Conditional loop termination
    gettimeofday(&currentTime, NULL); //Update current time struct

    //Determine time difference between current time and the last clean time
    long int cur_ms = (long int) (currentTime.tv_sec * 1000000 + currentTime.tv_usec);
    long int ts_ms = (long int) (lastClean.tv_sec * 1000000 + lastClean.tv_usec);
    long int diff = cur_ms - ts_ms;
    if ((suseconds_t) diff >= timeGap) { //If the amount of time passed is greater call dequeue on each topic queue
      for (int i = 0; i < registry.length; i++) {
        int result = dequeue(&registry.topics[i], *delta); //Call dequeue on each topic queue
        if (result) { //Upon success print out message
          printf("Clean Up Thread: Clean up thread has dequeued an item(s) from topic ID: %d\n", registry.topics[i].qid);
        }
      }
      gettimeofday(&lastClean, NULL); //Update time of last clean
    } else {
      sched_yield(); //Not enough time has passed so sched_yield
    }
  }
}

/*
Function to spawn NUMPROXIES / 2 subscribers threads and initialize their
respective thread argument pools.
*/
void initializeSubscribers(proxyPool *threadpool) {
  printf("Spawning the subscriber threads...\n");
  for (int i = 0; i < NUMPROXIES/2; i++) { //Loop through and initialize thread arguments
    pid[i].free = 1;
    pid[i].index = i;
    pid[i].threadfile = NULL;
    //Create the pthread and provide the initialized struct to the subscriber thread function
    pthread_create(&(threadpool->thread[i]), &attr, (void *)&subscriber, (void *) &pid[i]);
  }
}

/*
Function to spawn NUMPROXIES / 2 publishers threads and initialize their
respective thread argument pools.
*/
void initializePublishers(proxyPool *threadpool) {
  printf("Spawning the publisher threads...\n");
  for (int i = 0; i < NUMPROXIES/2; i++) { //Loop through and initialize thread arguments
    tid[i].free = 1;
    tid[i].index = i;
    tid[i].threadfile = NULL;
    //Create the pthread and provide the initialized struct to the subscriber thread function
    pthread_create(&(threadpool->thread[i]), &attr, (void *)(&publisher), (void *) &tid[i]);
  }
}

/*
Function to read in standard input and perform the commands provided through standard input
and then execute the appropriate actions for each input. Uses tokenization and memory structs.
*/
int quackerInterface(proxyPool *subscribers, proxyPool *publishers, suseconds_t *delta) {
  //Initialize pools with default values
  subscribers->cmdCount = 0;
  subscribers->currentJob = 0;
  publishers->cmdCount = 0;
  publishers->currentJob = 0;

  char *buffer = NULL; //Variables for reading the file
  size_t bufsize = 0;
  int exit_condition = 1;

  int topic_id; //Variables for parameters to create topic queue
  char topic_name[NAMESIZE];
  int queue_length;

  char *filep;
  char *token;
  char *saveptr;

  while(exit_condition) {//Read the standard input and get a new line and parse that line
    getline(&buffer, &bufsize, stdin);
    char *tmpbuffer = strdup(buffer);
    if (strtok(tmpbuffer, " \n\t") == NULL) {
      free(tmpbuffer);
      continue;
    }
    free(tmpbuffer);

    //Apply tokenization structure to tokenize all of the input

    token = strtok_r(buffer, " \n\t", &saveptr);
    if (strcmp(token, "create") == 0) {
      token = strtok_r(NULL, " \n\t", &saveptr);
      if (strcmp(token, "topic") == 0) {
        token = strtok_r(NULL, " \n\t", &saveptr);
        topic_id = atoi(token);
        token = strtok_r(NULL, " \"\n\t", &saveptr);
        strcpy(topic_name, token);
        printf("%s\n", topic_name);
        token = strtok_r(NULL, " \n\t", &saveptr);
        queue_length = atoi(token);
        createTopic(topic_id, topic_name, queue_length, &registry.topics[registry.length]); //Create topic with parsed tokens
        registry.length++;
      }

    } else if (strcmp(token, "query") == 0) {
      token = strtok_r(NULL, " \n\t", &saveptr);
      if (strcmp(token, "topics") == 0) {
        //Print out all topics
        for (int i = 0; i < registry.length; i++) {
          printf("Topic ID: %d - Topic Length: %d\n", registry.topics[i].qid, registry.topics[i].maxlength);
        }
      } else if (strcmp(token, "publishers") == 0) {
        //Print out all publishers
        for (int i = 0; i < publishers->cmdCount; i++) {
          printf("Publisher: %d - Command File Names: %s\n", i, publishers->commands[i]);
        }
      } else if (strcmp(token, "subscribers") == 0) {
        //Print out all subscribers
        for (int i = 0; i < subscribers->cmdCount; i++) {
          printf("Subscriber: %d - Command File Namees: %s\n", i, subscribers->commands[i]);
        }
      }
    } else if (strcmp(token, "add") == 0) {
      token = strtok_r(NULL, " \n\t", &saveptr);
      if ((strcmp(token, "publisher") == 0) && (publishers->cmdCount <= MAXPUBS)) {
        //Add a file so we malloc a char * for the file and story it into the struct
        token = strtok_r(NULL, " \"\n\t", &saveptr);
        filep = (char *)malloc(sizeof(char) *BUFSIZ);
        strcpy(filep, token);
        publishers->commands[publishers->cmdCount] = filep;
        publishers->cmdCount++;
      } else if ((strcmp(token, "subscriber") == 0) && (subscribers->cmdCount <= MAXSUBS)) {
        //Add a file so we malloc a char * for the file and story it into the struct
        token = strtok_r(NULL, " \"\n\t", &saveptr);
        filep = (char *)malloc(sizeof(char) *BUFSIZ);
        strcpy(filep, token);
        subscribers->commands[subscribers->cmdCount] = filep;
        subscribers->cmdCount++;
      }

    } else if (strcmp(token, "delta") == 0) {
      //Designate a delta to be used for the program.
      token = strtok_r(NULL, " \n\t", &saveptr);
      int deltas = atoi(token);
      *delta = (suseconds_t) deltas * 1000000;
    } else if (strcmp(token, "start") == 0) {
      exit_condition = 0; //break from main loop
    }
  }
  free(buffer); //Free memory
  return 1;
}

/*
Primary program driver to call all other functions and execute system properties.
*/
int main(int argc, char*argv[]) {
  suseconds_t delta = 1000000; //Default delta
  proxyPool subs, pubs; //Initialize subscriber publisher pools.

  quackerInterface(&subs, &pubs, &delta); //Parse standard input and save items into structs

  pthread_t cleanupthread; //Begin the cleanup thread
  pthread_create(&cleanupthread, NULL, cleanup, (void *)&delta);
  initializePublishers(&pubs); //Initialize publishers and subscribers
  initializeSubscribers(&subs);

  //Schedule the publishers to perform all command files with given threads
  while (pubs.currentJob < pubs.cmdCount) {
    for (int i = 0; i < NUMPROXIES/2; i++) { //Loop through all threads and if it is free it is given work
      if (pubs.currentJob >= pubs.cmdCount) {
        break;
      }
      if (tid[i].free == 1) { //Free
        tid[i].free = 0;
        tid[i].threadfile = pubs.commands[pubs.currentJob]; //Turned to busy and given work
        pubs.currentJob++;
      }
    }
  }
  //Schedule the subscribers to perform all command files with given threads
  while (subs.currentJob < subs.cmdCount) {
    for (int i = 0; i < NUMPROXIES/2; i++) { //Loop through all threads and if it is free it is given work
      if (subs.currentJob >= subs.cmdCount) {
        break;
      }
      if (pid[i].free == 1) {
        pid[i].free = 0;
        pid[i].threadfile = subs.commands[subs.currentJob]; //Turned to busy and given work
        subs.currentJob++;
      }
    }
  }

  //Wait until all publishers have completed before killing all the publishers
  while (1) {
    int f = 0;
    for (int i = 0; i < NUMPROXIES/2; i++) {
      if (tid[i].threadfile == NULL) {
        f++;
      }
    }
    if (f == NUMPROXIES/2) {
      breakLoop = 1;
      break;
    }
  }
  //Wait until all subscribers have completed before killing all the subscribers.
  while (1) {
    int f = 0;
    for (int i = 0; i < NUMPROXIES/2; i++) {
      if (pid[i].threadfile == NULL) {
        f++;
      }
    }
    if (f == NUMPROXIES/2) {
      breakLoop2 = 1;
      break;
    }
  }
  //Join all the pool threads
  for (int i = 0; i < NUMPROXIES/2; i++) {
    pthread_join(subs.thread[i], NULL);
    pthread_join(pubs.thread[i], NULL);
  }
  //All the publishers and subscribers have completed so kill the cleanupthread.
  termination = 1;
  pthread_join(cleanupthread, NULL);

  //Free all memory
  for (int i = 0; i < MAXTOPICS; i++) {
    free(registry.topics[i].entries);
  }
  for (int i = 0; i < pubs.cmdCount; i++) {
    free(pubs.commands[i]);
  }
  for (int i = 0; i < subs.cmdCount; i++) {
    free(subs.commands[i]);
  }

  return 0;


}
