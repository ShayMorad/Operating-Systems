#include "user_thread.h"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

address_t translate_address (address_t addr)
{
  address_t ret;
  asm volatile("xor    %%fs:0x30,%0\n"
               "rol    $0x11,%0\n"
      : "=g" (ret)
      : "0" (addr));
  return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#endif

User_Thread::User_Thread (int id, thread_entry_point entry_point) :
    status (READY), tid (id),  sleep_time (0),
    quantums_ran (0), initial_func (entry_point)
{
  this->stack = nullptr;
  if(this->tid != 0)
  {
    stack = new char[STACK_SIZE];
    auto sp = (address_t) stack + STACK_SIZE - sizeof (address_t);
    auto pc = (address_t) entry_point;
    sigsetjmp(env, 1);
    env->__jmpbuf[JB_SP] = translate_address (sp);
    env->__jmpbuf[JB_PC] = translate_address (pc);
    sigemptyset (&env->__saved_mask);
  }
}



User_Thread::~User_Thread ()
{
  if(this->stack != nullptr && this->tid != 0){
    delete[] this->stack;
    stack = nullptr;
  }
}
int User_Thread::get_tid () const
{
  return this->tid;
}
int User_Thread::get_status () const
{
  return this->status;
}
int User_Thread::get_quantums_ran () const
{
  return this->quantums_ran;
}
int User_Thread::get_sleep_time () const
{
  return this->sleep_time;
}
void User_Thread::set_status (int set_status)
{
  this->status = set_status;
}
void User_Thread::set_sleep_time (int set_sleep_time)
{
  this->sleep_time = set_sleep_time;
}
User_Thread::User_Thread (const User_Thread &other)
    : status(other.status), tid(other.tid),
    sleep_time(other.sleep_time), quantums_ran(other.quantums_ran),
    initial_func(other.initial_func) {
  if(other.get_tid() != 0){
    this->stack = new char[STACK_SIZE];
    std::memcpy(stack, other.stack, STACK_SIZE);
  }
  std::memcpy(&env, &other.env, sizeof(sigjmp_buf));
}

User_Thread &User_Thread::operator= (const User_Thread &other)
{
  if (this != &other) { // Avoid self-assignment
    // Copy the member variables from the other object
    this->status = other.status;
    this->tid = other.tid;
    this->sleep_time = other.sleep_time;
    this->quantums_ran = other.quantums_ran;
    this->initial_func = other.initial_func;
    if(stack != nullptr){
      delete[] stack;
      stack = nullptr;
    }
    if(other.get_tid() != 0){
      this->stack = new char[STACK_SIZE];
      std::memcpy(this->stack, other.stack, STACK_SIZE);
    }
    std::memcpy(&env, &other.env, sizeof(sigjmp_buf));
  }
  return *this;
}
void User_Thread::set_tid (int id)
{
  this->tid = id;
}
void User_Thread::inc_quantums_ran ()
{
  this->quantums_ran++;
}
