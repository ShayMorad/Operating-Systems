#ifndef _USER_THREAD_H_
#define _USER_THREAD_H_

#include "uthreads.h"
#include <cstdio>
#include <csignal>
#include <unistd.h>
#include <csetjmp>
#include <sys/time.h>
#include <cstring>
#include <iostream>


#define READY 1
#define BLOCKED 2
#define SLEEPY 3

class User_Thread
{

 public:
  sigjmp_buf env{};
  char *stack;

  // constructor
  User_Thread (int id, thread_entry_point entry_point);

  // copy constructor
  User_Thread(const User_Thread &other);

  // assignment operator
  User_Thread& operator=(const User_Thread &other);

  // destructor
  ~User_Thread ();

  int get_tid () const;

  int get_status () const;

  int get_quantums_ran () const;

  int get_sleep_time () const;

  void set_tid (int id);

  void set_status (int set_status);

  void set_sleep_time (int set_sleep_time);

  void inc_quantums_ran ();

 private:
  int status;
  int tid;
  int sleep_time;
  int quantums_ran;
  thread_entry_point initial_func;
};

#endif //_USER_THREAD_H_
