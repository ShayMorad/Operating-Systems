#include "uthreads.h"
#include "user_thread.h"
#include <deque>
#include <vector>
#include <iostream>
#include <map>

// constants.
#define FAILURE (-1)
#define SUCCESS 0
#define AVAILABLE true
#define TAKEN false
#define INIT_ERR "thread library error: quantums must be positive."
#define SC_SET_TIMER_ERR "system error: the setitimer system call has failed."
#define MAX_THREADS_ERR "thread library error: maximum amount of threads \
achieved."
#define MEM_ALLOC_ERR "thread library error: failed to allocate memory when \
spawning a new thread."
#define NULL_ERR "thread library error: provided null argument."
#define INCORRECT_TID_ERR "thread library error: thread id is not valid."
#define NONEXISTENT_THREAD_ERR "thread library error: thread with the \
provided id does not exist."
#define SC_MASK_ERR "system error: the sigprocmask system call has failed."
#define SC_SIG_ACTION_ERR "system error: the sigaction system call has failed."
#define SC_SIGSETJMP_ERR "system error: the sigsetjmp system call has failed."
#define SC_SIGLONGJMP_ERR "system error: the siglongjmp system call has \
failed."
#define BLOCK_MAIN_ERR "thread library error: the main thread should not be \
blocked."
#define INCORRECT_SLEEP_QUANTUMS_ERR "thread library error: sleep num of \
quantums must be positive."
#define SLEEP_MAIN_THREAD_ERR "thread library error: the main thread should \
not be asleep."

// added helper funcs declarations implemented at the end.
int available_tid ();
void reset_timer ();
bool is_tid_valid (int tid);
bool does_thread_exist (int tid);
void clear_memory (int cond);
void self_termination_context_switch ();
void timer_handler (int sig);
void block_alarm_signal ();
void unblock_alarm_signal ();
void wake_sleepy_threads ();
void check_delete_thread ();

std::deque<User_Thread *> ready_queue;
std::map<int, User_Thread *> threads;
int total_threads = 1;
int total_ran_quantums = 0;
struct itimerval timer;
struct sigaction sa = {0};
User_Thread *main_thread;
User_Thread *cur_thread;
User_Thread *thread_to_term = nullptr;
bool existing_tids[MAX_THREAD_NUM] = {AVAILABLE};
bool need_to_exit = false;

void timer_handler (int sig)
{
  block_alarm_signal ();
  check_delete_thread ();
  if (cur_thread->get_status () == READY && cur_thread->get_sleep_time () == 0)
  {
    ready_queue.push_back (cur_thread);
  }
//  std::cout<< "Threads in the ready queue BEFORE waking sleepies: ";
//  for(auto it = ready_queue.begin() ; it!=ready_queue.end(); ++it){
//    if((*it) != nullptr){
//      std::cout << (*it)->get_tid();
//    }
//  }
  if (cur_thread != nullptr)
  {
    int ret = sigsetjmp(cur_thread->env, 1);
    if (ret < 0)
    {
      std::cerr << SC_SIGSETJMP_ERR << std::endl;
      clear_memory (1);
    }
    if (ret == 0)
    {
      cur_thread = ready_queue.front ();
      ready_queue.pop_front ();
      cur_thread->inc_quantums_ran ();
      wake_sleepy_threads ();
      total_ran_quantums++;
      reset_timer ();
      unblock_alarm_signal ();
      siglongjmp (cur_thread->env, 1);
      std::cerr << SC_SIGLONGJMP_ERR << std::endl;
      clear_memory (1);
    }
    if(need_to_exit){
      clear_memory (0);
    }
  }
  unblock_alarm_signal ();
}

void wake_sleepy_threads ()
{
  auto it = threads.begin ();
  while (it != threads.end ())
  {
    if (it->second->get_sleep_time () == 1 && it->second->get_status () != BLOCKED)
    {
      if (it->second->get_tid () != cur_thread->get_tid ())
      {
        it->second->set_status (READY);
        ready_queue.push_back (it->second);
      }
    }
    // decrement sleep time by 1
    if (it->second->get_sleep_time () > 0)
    {
      it->second->set_sleep_time (it->second->get_sleep_time () - 1);
    }
    ++it;
  }
}

void reset_timer ()
{
  // start a virtual timer. it counts down whenever this process is executing.
  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr) < 0)
  {
    std::cerr << SC_SET_TIMER_ERR << std::endl;
    clear_memory (1);
  }
}

int uthread_init (int quantum_usecs)
{
  if (quantum_usecs <= 0)
  {
    std::cerr << INIT_ERR << std::endl;
    return FAILURE;
  }
  sa = {0};
  for (bool &existing_tid: existing_tids)
  {
    existing_tid = AVAILABLE;
  }
  // init the timer with input intervals.
  timer.it_value.tv_sec = quantum_usecs / 1000000;
  timer.it_value.tv_usec = quantum_usecs % 1000000;
  timer.it_interval = timer.it_value;
  sa.sa_handler = &timer_handler;
  if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
  {
    std::cerr << SC_SIG_ACTION_ERR << std::endl;
    clear_memory (1);
  }
  main_thread = new User_Thread (0, nullptr);
  threads.insert({0,main_thread});
  cur_thread = main_thread;
  existing_tids[0] = TAKEN;
  reset_timer ();
  timer_handler (0);
  return SUCCESS;
}

int uthread_spawn (thread_entry_point entry_point)
{
  block_alarm_signal ();
  check_delete_thread ();
  if (entry_point == nullptr)
  {
    std::cerr << NULL_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  if (total_threads == MAX_THREAD_NUM)
  {
    std::cerr << MAX_THREADS_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  int available_next_tid = available_tid ();
  // debugging
  if (available_next_tid < 0)
  {
    std::cout << "ERROR LOOKING FOR AVAILABLE TID, CHECK CODE\n";
    unblock_alarm_signal ();
    return FAILURE;
  }
  User_Thread *thread;
  // try to allocate memory to a new thread.
  try
  {
    thread = new User_Thread (available_next_tid, entry_point);
  }
  catch (const std::exception &)
  {
    delete thread;
    std::cerr << MEM_ALLOC_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  threads.insert({available_next_tid,thread});
  ready_queue.push_back (thread);
  existing_tids[available_next_tid] = TAKEN;
  total_threads++;
  unblock_alarm_signal ();
  return available_next_tid;
}

int uthread_terminate (int tid)
{
  block_alarm_signal ();

  check_delete_thread ();

  if (!is_tid_valid (tid))
  {
    std::cerr << INCORRECT_TID_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  if (!does_thread_exist (tid))
  {
    std::cerr << NONEXISTENT_THREAD_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  if (tid == 0)
  {
    if(cur_thread->get_tid() != 0){
      cur_thread = main_thread;
      need_to_exit = true;
      siglongjmp (cur_thread->env, 1);
    }
    clear_memory (0);
    return SUCCESS;
  }
  existing_tids[tid] = AVAILABLE;
  total_threads--;
  if (tid == cur_thread->get_tid ())
  {
    cur_thread->set_status (BLOCKED);
    thread_to_term = cur_thread;
    cur_thread = nullptr;
    threads.erase (tid);
    self_termination_context_switch ();
    return SUCCESS;
  }

  // search and erase from the ready_queue
  for (auto it = ready_queue.begin (); it != ready_queue.end ();)
  {
    if ((*it)->get_tid () == tid)
    {
      it = ready_queue.erase (it);
    }
    else
    {
      ++it;  // Only increment if no erasure occurred
    }
  }
  // delete and erase from threads map
  auto it = threads.find (tid);
  if (it != threads.end ())
  {
    threads.erase (it);
    delete (it->second);
  }
  unblock_alarm_signal ();
  return SUCCESS;
}

int uthread_block (int tid)
{
  block_alarm_signal ();
  check_delete_thread ();
  if (!is_tid_valid (tid))
  {
    std::cerr << INCORRECT_TID_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  if (!does_thread_exist (tid))
  {
    std::cerr << NONEXISTENT_THREAD_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  if (tid == 0)
  {
    std::cerr << BLOCK_MAIN_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  auto it = threads.find (tid);
  if (it != threads.end () && it->second != nullptr)
  {

    it->second->set_status (BLOCKED);
  }

  // if thread to block is the current running thread
  if (cur_thread->get_tid () == tid)
  {
    cur_thread->set_status (BLOCKED);
    timer_handler (0);
    unblock_alarm_signal ();
    return SUCCESS;
  }

  for (auto it = ready_queue.begin (); it != ready_queue.end (); ++it)
  {
    if ((*it)->get_tid () == tid)
    {
      (*it)->set_status (BLOCKED);
      ready_queue.erase (it);
      unblock_alarm_signal ();
      return SUCCESS;
    }
  }
  unblock_alarm_signal ();
  return FAILURE;
}

int uthread_resume (int tid)
{
  block_alarm_signal ();
  check_delete_thread ();
  if (!is_tid_valid (tid))
  {
    std::cerr << INCORRECT_TID_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  if (!does_thread_exist (tid))
  {
    std::cerr << NONEXISTENT_THREAD_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  auto it = threads.find (tid);
  if (it != threads.end () && it->second != nullptr)
  {
    if(it->second->get_status() == BLOCKED && it->second->get_sleep_time()
    == 0){
      ready_queue.push_back (it->second);
    }
    it->second->set_status (READY);
  }
  unblock_alarm_signal ();
  return SUCCESS;
}

int uthread_sleep (int num_quantums)
{
  block_alarm_signal ();
  //check_delete_thread();
  if (num_quantums < 0)
  {
    std::cerr << INCORRECT_SLEEP_QUANTUMS_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  if (cur_thread->get_tid () == 0)
  {
    std::cerr << SLEEP_MAIN_THREAD_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  cur_thread->set_status (SLEEPY);
  cur_thread->set_sleep_time (num_quantums);
  // search and erase from the ready_queue
  for (auto it = ready_queue.begin (); it != ready_queue.end ();)
  {
    if ((*it)->get_tid () == cur_thread->get_tid ())
    {
      it = ready_queue.erase (it);
    }
    else
    {
      ++it;  // Only increment if no erasure occurred
    }
  }
  timer_handler (0);
  unblock_alarm_signal ();
  return SUCCESS;

}

int uthread_get_tid ()
{
  return cur_thread->get_tid ();
}

int uthread_get_total_quantums ()
{
  return total_ran_quantums;
}

int uthread_get_quantums (int tid)
{
  block_alarm_signal ();
  check_delete_thread ();
  if (!is_tid_valid (tid))
  {
    std::cerr << INCORRECT_TID_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }

  if (!does_thread_exist (tid))
  {
    std::cerr << NONEXISTENT_THREAD_ERR << std::endl;
    unblock_alarm_signal ();
    return FAILURE;
  }
  unblock_alarm_signal ();
  auto it = threads.find (tid);
  if (it != threads.end () && it->second != nullptr)
  {
    return it->second->get_quantums_ran ();
  }
  return FAILURE;
}

int available_tid ()
{
  for (int tid = 0; tid < MAX_THREAD_NUM; tid++)
  {
    if (existing_tids[tid])
    {
      return tid;
    }
  }
  return FAILURE;
}

bool is_tid_valid (int tid)
{
  return 0 <= tid && tid < MAX_THREAD_NUM;
}

bool does_thread_exist (int tid)
{
  return existing_tids[tid] == TAKEN;
}

void clear_memory (int cond)
{
  ready_queue.clear ();  // clear ready the container.
  auto it = threads.begin ();
  while (it != threads.end ())
  {
    if (it->second != nullptr && it->second->get_tid () != 0)
    {
      delete it->second;
    }
    ++it;
  }
  threads.clear ();
  delete main_thread;
  delete thread_to_term;
  exit (cond);
}

void self_termination_context_switch ()
{
  cur_thread = ready_queue.front ();
  ready_queue.pop_front ();
  cur_thread->inc_quantums_ran ();
  total_ran_quantums++;
  reset_timer ();
  unblock_alarm_signal ();
  siglongjmp (cur_thread->env, 1);
  std::cerr << SC_SIGLONGJMP_ERR << std::endl;
  clear_memory (1);
}

void block_alarm_signal ()
{
  sigset_t set;
  sigemptyset (&set);
  sigaddset (&set, SIGVTALRM);
  if (sigprocmask (SIG_BLOCK, &set, nullptr) == -1)
  {
    std::cerr << SC_MASK_ERR << std::endl;
    clear_memory (1);
  }
}

void unblock_alarm_signal ()
{
  sigset_t set;
  sigemptyset (&set);
  sigaddset (&set, SIGVTALRM);
  if (sigprocmask (SIG_UNBLOCK, &set, nullptr) == -1)
  {
    std::cerr << SC_MASK_ERR << std::endl;
    clear_memory (1);
  }
}

void check_delete_thread ()
{
  if (thread_to_term != nullptr)
  {
    threads.erase (thread_to_term->get_tid ());
    delete thread_to_term;
    thread_to_term = nullptr;
  }
}






