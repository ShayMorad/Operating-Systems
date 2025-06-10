#include "MapReduceFramework.h"
#include "Barrier.h"

#include <pthread.h>
#include <atomic>
#include <semaphore.h>
#include <iostream>
#include <algorithm>

void *thread_start_routine (void *arg);
struct ThreadDataBlock;
struct JobDataBlock;

struct ThreadDataBlock
{
    int thread_id;
    IntermediateVec *intermediate_vec;
    JobDataBlock *job_context;
};

struct JobDataBlock
{
    const MapReduceClient *client;
    bool waiting_for_job;

    int num_of_threads;
    pthread_t *threads;
    struct ThreadDataBlock *threads_data_blocks;

    const InputVec *input_vec;
    std::vector<IntermediateVec *> *shuffled_sorted_vec;
    OutputVec *output_vec;

    std::atomic<uint64_t> *job_state;
    std::atomic<int> *map_counter;
    std::atomic<int> *total_intermediate_elems;
    std::atomic<int> *total_shuffled_elems;

    Barrier *barrier;
    pthread_mutex_t *mutex_a;
    sem_t *sem_b;

    JobDataBlock () :
        waiting_for_job (false),
        job_state (new std::atomic<uint64_t> (static_cast<uint64_t>
                                              (UNDEFINED_STAGE) << 62)),
        map_counter (new std::atomic<int> (0)),
        total_intermediate_elems (new std::atomic<int> (0)),
        total_shuffled_elems (new std::atomic<int> (0)),
        mutex_a (new pthread_mutex_t),
        sem_b (new sem_t ())
    {

      if (pthread_mutex_init (mutex_a, nullptr) != 0)
      {
        std::cout << "system error: semaphore initialization failed." <<
                  std::endl;
        // resources are automatically freed and there are no memory leaks
        // because we implemented it so that the destructor of JobDataBlock
        // frees all the memory and it is called automatically when we exit.
        exit (1);
      }

      if (sem_init (sem_b, 0, 1) != 0)
      {
        std::cout << "system error: semaphore initialization failed." <<
                  std::endl;
        // resources are automatically freed and there are no memory leaks
        // because we implemented it so that the destructor of JobDataBlock
        // frees all the memory and it is called automatically when we exit.
        exit (1);
      }

    }

    ~JobDataBlock ()
    {
      delete job_state;
      delete shuffled_sorted_vec;
      delete map_counter;
      delete total_intermediate_elems;
      delete total_shuffled_elems;
      delete barrier;

      for (int i = 0; i < num_of_threads; ++i)
      {
        delete threads_data_blocks[i].intermediate_vec;
      }

      delete[] threads;
      delete[] threads_data_blocks;

      if (pthread_mutex_destroy (mutex_a) != 0)
      {
        std::cout << "system error: mutex destruction failed." <<
                  std::endl;
        // resources are automatically freed and there are no memory leaks
        // because we implemented it so that the destructor of JobDataBlock
        // frees all the memory and it is called automatically when we exit.
        exit (1);
      }

      if (sem_destroy (sem_b) != 0)
      {
        std::cout << "system error: semaphore destruction failed."
                  << std::endl;
        // resources are automatically freed and there are no memory leaks
        // because we implemented it so that the destructor of JobDataBlock
        // frees all the memory and it is called automatically when we exit.
        exit (1);
      }
      delete mutex_a;
      delete sem_b;
    }
};

JobHandle startMapReduceJob (const MapReduceClient &client,
                             const InputVec &inputVec, OutputVec &outputVec,
                             int multiThreadLevel)
{
  auto job = new JobDataBlock ();
  job->client = &client;
  job->num_of_threads = multiThreadLevel;
  job->threads = new pthread_t[multiThreadLevel];
  job->threads_data_blocks = new ThreadDataBlock[multiThreadLevel];
  job->input_vec = &inputVec;
  job->output_vec = &outputVec;
  job->barrier = new Barrier (multiThreadLevel);

  for (int i = 0; i < multiThreadLevel; i++)
  {
    job->threads_data_blocks[i].thread_id = i;
    job->threads_data_blocks[i].intermediate_vec = new IntermediateVec ();
    job->threads_data_blocks[i].job_context = job;
    if (pthread_create (job->threads + i, nullptr, thread_start_routine,
                        &job->threads_data_blocks[i]) != 0)
    {
      std::cout << "system error: thread creation failed." << std::endl;
      // resources are automatically freed and there are no memory leaks
      // because we implemented it so that the destructor of JobDataBlock
      // frees all the memory and it is called automatically when we exit.
      exit (1);
    }
  }

  return (JobHandle) job;
}

void *thread_start_routine (void *arg)
{
  auto *thread_context = (ThreadDataBlock *) arg;
  JobDataBlock *job = thread_context->job_context;

  uint64_t expected = static_cast<uint64_t>(UNDEFINED_STAGE) << 62;
  uint64_t desired = (static_cast<uint64_t>(MAP_STAGE) << 62) |
                     (static_cast<uint64_t>(job->input_vec->size ()) << 31);
  job->job_state->compare_exchange_strong (expected, desired);

  // running Map
  unsigned long pre_count = (*(job->map_counter))++;
  while (pre_count < job->input_vec->size ())
  {
    if (pre_count >= job->input_vec->size ()){
      break;
    }
    InputPair pair = (*job->input_vec)[pre_count];
    job->client->map (pair.first, pair.second, thread_context);
    (*(job->job_state))++;
    pre_count = (*(job->map_counter))++;
    if (pre_count >= job->input_vec->size ())
    {
      break;
    }
  }

  for (unsigned int i = 0; i < thread_context->intermediate_vec->size (); i++)
  {
    (*(job->total_intermediate_elems))++;
  }

  // running Sort
  std::sort (thread_context->intermediate_vec->begin (),
             thread_context->intermediate_vec->end (),
             [] (IntermediatePair &p1, IntermediatePair &p2)
             {
                 return *(p1.first) < *(p2.first);
             });

  // running Shuffle
  job->barrier->barrier ();

  if (thread_context->thread_id == 0)
  {
    long intermediate_elems_shifted =
        static_cast<long>(job->total_intermediate_elems->load ()) << 31;
    long shuffle_stage_shifted = static_cast<long>(SHUFFLE_STAGE) << 62;
    uint64_t combined_value = static_cast<uint64_t>(intermediate_elems_shifted
                                                    | shuffle_stage_shifted);

    job->job_state->store (combined_value);
    auto *shuffled = new std::vector<IntermediateVec *> ();

    for (;;)
    {
      K2 *max_key = nullptr;
      bool any_data_left = false;

      for (int i = 0; i < job->num_of_threads; ++i)
      {
        auto &intermediate_data = job->threads_data_blocks[i].intermediate_vec;

        if (!intermediate_data->empty ())
        {
          any_data_left = true;
          K2 *cur_key = intermediate_data->back ().first;

          if (max_key == nullptr || *max_key < *cur_key)
          {
            max_key = cur_key;
          }
        }
      }

      if (!any_data_left)
      {
        break;
      }

      auto *data = new IntermediateVec ();

      for (int i = 0; i < job->num_of_threads; ++i)
      {
        auto *intermediate_data = job->threads_data_blocks[i].intermediate_vec;

        while (!intermediate_data->empty ())
        {
          K2 *cur_key = intermediate_data->back ().first;

          if (!(*cur_key < *max_key) && !(*max_key < *cur_key))
          {
            data->push_back (intermediate_data->back ());
            intermediate_data->pop_back ();
            (*(job->job_state))++;
          }
          else
          {
            break;
          }
        }
      }

      shuffled->push_back (data);
      (*(job->total_shuffled_elems))++;
    }

    job->shuffled_sorted_vec = shuffled;

    long shuffled_elems_shifted = static_cast<long>
    (job->total_shuffled_elems->load ()) << 31;
    long reduce_stage_shifted = static_cast<long>(REDUCE_STAGE) << 62;
    uint64_t combined_value_new = static_cast<uint64_t>
    (shuffled_elems_shifted | reduce_stage_shifted);

    job->job_state->store (combined_value_new);

  }
  job->barrier->barrier ();

  // running Reduce
  while (!job->shuffled_sorted_vec->empty ())
  {
    if (pthread_mutex_lock ((job->mutex_a)) != 0)
    {
      std::cout << "system error: mutex lock failed." << std::endl;
      // resources are automatically freed and there are no memory leaks
      // because we implemented it so that the destructor of JobDataBlock
      // frees all the memory and it is called automatically when we exit.
      exit (1);
    }

    if (job->shuffled_sorted_vec->empty ())
    {
      if (pthread_mutex_unlock (job->mutex_a) != 0)
      {
        std::cout << "system error: mutex unlock failed." << std::endl;
        // resources are automatically freed and there are no memory leaks
        // because we implemented it so that the destructor of JobDataBlock
        // frees all the memory and it is called automatically when we exit.
        exit (1);
      }
      break;
    }

    IntermediateVec *vec_pairs = job->shuffled_sorted_vec->back ();
    job->shuffled_sorted_vec->pop_back ();

    if (pthread_mutex_unlock (job->mutex_a) != 0)
    {
      std::cout << "system error: mutex unlock failed." << std::endl;
      // resources are automatically freed and there are no memory leaks
      // because we implemented it so that the destructor of JobDataBlock
      // frees all the memory and it is called automatically when we exit.
      exit (1);
    }

    job->client->reduce (vec_pairs, job);
    (*(job->job_state))++;
    delete vec_pairs;
  }
  return nullptr;
}

void emit2 (K2 *key, V2 *value, void *context)
{
  auto *thread_context = static_cast<ThreadDataBlock *>(context);
  thread_context->intermediate_vec->emplace_back (key, value);
}

void emit3 (K3 *key, V3 *value, void *context)
{
  auto *job = static_cast<JobDataBlock *>(context);
  if (sem_wait (job->sem_b) != 0)
  {
    std::cout << "system error: semaphore wait failed." << std::endl;
    // resources are automatically freed and there are no memory leaks
    // because we implemented it so that the destructor of JobDataBlock
    // frees all the memory and it is called automatically when we exit.
    exit (1);
  }
  job->output_vec->emplace_back (key, value);
  if (sem_post (job->sem_b) != 0)
  {
    std::cout << "system error: semaphore post failed." << std::endl;
    // resources are automatically freed and there are no memory leaks
    // because we implemented it so that the destructor of JobDataBlock
    // frees all the memory and it is called automatically when we exit.
    exit (1);
  }
}

void waitForJob (JobHandle job)
{
  auto *jobb = static_cast<JobDataBlock *>(job);
  if (jobb == nullptr)
  {
    return;
  }
  if (jobb->waiting_for_job)
  {
    return;
  }
  else
  {
    jobb->waiting_for_job = true;
  }
  for (int i = 0; i < jobb->num_of_threads; i++)
  {
    if (pthread_join (jobb->threads[i], nullptr) != 0)
    {
      std::cout << "system error: thread join failed." << std::endl;
      // resources are automatically freed and there are no memory leaks
      // because we implemented it so that the destructor of JobDataBlock
      // frees all the memory and it is called automatically when we exit.
      exit (1);
    }
  }
}

void getJobState (JobHandle job, JobState *state)
{
  auto *jobb = static_cast<JobDataBlock *>(job);
  uint64_t job_state = jobb->job_state->load ();
  stage_t current_stage = static_cast<stage_t>(job_state >> 62);
  state->stage = current_stage;
  unsigned long total = job_state >> 31 & 0x7FFFFFFF;
  unsigned long current = job_state & 0x7FFFFFFF;
  state->percentage =
      static_cast<float>(current) / static_cast<float>(total) * 100;
}

void closeJobHandle (JobHandle job)
{
  auto *jobb = static_cast<JobDataBlock *>(job);
  if (jobb == nullptr)
  {
    return;
  }
  waitForJob (jobb);

  // deleting jobb manually releases all the memory, it is not needed
  // because we implemented a destructor but by doing so making sure the
  // pointer itself is also freed. this is just a safe measure to make sure
  // all the memory is freed.
  delete jobb;
}


