#ifndef JOB_FARM_INCLUDE_MASTER_H_
#define JOB_FARM_INCLUDE_MASTER_H_

#define PATH_MAX 256

struct job_msg{
  long mtype;
  char path[PATH_MAX];
};

#endif /* JOB_FARM_INCLUDE_MASTER_H_ */
