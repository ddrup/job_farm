#include "../include/master.h"

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

void cleanup(int mq, FILE *fp){
  if( fp ) fclose(fp);
  if( mq >= 0 )  msgctl(mq, IPC_RMID, NULL);
}

int send_message(int mq, FILE *fp) {
  char buf[PATH_MAX];
  struct job_msg msg;
  int count_str = 0;
  msg.mtype = 1;

  while(fgets(buf, sizeof(buf), fp) != NULL) {
    buf[strcspn(buf, "\n")] = '\0';
    strncpy(msg.path, buf, PATH_MAX);

    if( msgsnd(mq, &msg, sizeof(msg.path), 0) == -1 ){
      perror("msgsnd");
      return -1;
    }

    ++count_str;
  }

  return count_str;
}


int send_message_stop(int mq, int count_workers) {
  struct job_msg msg;
  msg.mtype = 2;
  
  for(int i = 0; i < count_workers; ++i) {
    if( msgsnd(mq, &msg, sizeof(msg.path), 0) == -1 ){
      perror("msgsnd");
      return -1;
    }   
  }
  
  return 0;
}

pid_t* xmalloc(size_t size) {
  pid_t *pids = (pid_t *)malloc(size); // cast можно убрать в си
  if( !pids ) {
    perror("malloc");
    return NULL;
  }
  memset(pids, 0, size);
  return pids;
}

void worker_loop(int i, int qid) {
  ssize_t sz;
  struct job_msg msg;
  
  printf("Worker %d, PID=%d\n", i, getpid());
  for(;;) {
    
    do{
      sz = msgrcv(qid, &msg, sizeof(msg.path), 0, MSG_NOERROR);
    } while( sz == -1 && errno == EINTR );

    if( sz == -1 ) {
      perror("msgrcv");
      exit(1);
    }
    
    if( msg.mtype == 2 ){
      break;
    }

     printf("message recived: path = %s\n", msg.path);
     sleep(2);
  }
}

int main(int argc, char *argv[]) {
  if(argc != 2) {
    fprintf(stderr, "Incorrect number of parametrs");
    return 1;
  } 
  
  FILE *fp = fopen(argv[1], "r");
  if( !fp ){
    perror("File opening failed");
    return 1;
  }

  key_t qkey = ftok(argv[1], 'J');
  if( qkey == (key_t)-1 ){
    fclose(fp);
    perror("ftok");
    return 1;
  }

  int mq = msgget(qkey, IPC_CREAT | 0666);
  if( mq == -1 ) {
    fclose(fp);
    perror("msgget");
    return 1;
  }

  int workers = sysconf(_SC_NPROCESSORS_ONLN);
  if( workers < 1 ) {
    cleanup(mq, fp);
    return 1;
  } 

  if( send_message(mq, fp) == -1 ) {
    cleanup(mq, fp);
    return 1;
  }

  pid_t *pids = xmalloc(workers * sizeof *pids);
  if( !pids ){
    cleanup(mq, fp);
    return 1;
  }

  for(int i = 0; i < workers; ++i) {
    pid_t pid = fork();
    if( pid < 0 ) {
      perror("fork");
      cleanup(mq, fp);
      return 1;
    } else if( pid == 0 ) {
      worker_loop(i, mq);
      exit(0);
    } else if( pid > 0 ) {
      pids[i] = pid;
    }
  }

  if( send_message_stop(mq, workers) == -1 ){
    cleanup(mq, fp);
    return 1;
  }

  int status;
  pid_t w;
  for(int i = 0; i < workers; ++i) {
    w = waitpid(pids[i], &status, 0);
    if( w == -1 ){
      perror("waitpid");
    }

  }
  
  free(pids);
  cleanup(mq, fp);
  return 0;
}
