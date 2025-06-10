// OS 24 EX1

#include <cmath>
#include <iostream>
#include "memory_latency.h"
#include "measure.h"

#define GALOIS_POLYNOMIAL ((1ULL << 63) | (1ULL << 62) | (1ULL << 60) | (1ULL << 59))
#define BASE_SIZE 100

typedef uint64_t array_element_t;

/**
 * Converts the struct timespec to time in nano-seconds.
 * @param t - the struct timespec to convert.
 * @return - the value of time in nano-seconds.
 */
uint64_t nanosectime (struct timespec t)
{
  return (uint64_t) t.tv_sec * 1000000000ULL + (uint64_t) t.tv_nsec;
}

/**
* Measures the average latency of accessing a given array in a sequential order.
* @param repeat - the number of times to repeat the measurement for and average on.
* @param arr - an allocated (not empty) array to preform measurement on.
* @param arr_size - the length of the array arr.
* @param zero - a variable containing zero in a way that the compiler doesn't "know" it in compilation time.
* @return struct measurement containing the measurement with the following fields:
*      double baseline - the average time (ns) taken to preform the measured operation without memory access.
*      double access_time - the average time (ns) taken to preform the measured operation with memory access.
*      uint64_t rnd - the variable used to randomly access the array, returned to prevent compiler optimizations.
*/
struct measurement
measure_sequential_latency (uint64_t repeat, array_element_t *arr, uint64_t arr_size, uint64_t zero)
{
    repeat =
      arr_size > repeat ? arr_size : repeat; // Make sure repeat >= arr_size

  // Baseline measurement:
  struct timespec t0;
  timespec_get (&t0, TIME_UTC);
  register uint64_t rnd = 12345;
  for (register uint64_t i = 0; i < repeat; i++)
  {
    register uint64_t index = rnd % arr_size;
    rnd ^= index & zero;
    rnd = -~rnd;
  }
  struct timespec t1;
  timespec_get (&t1, TIME_UTC);

  // Memory access measurement:
  struct timespec t2;
  timespec_get (&t2, TIME_UTC);
  rnd = (rnd & zero) ^ 12345;
  for (register uint64_t i = 0; i < repeat; i++)
  {
    register uint64_t index = rnd % arr_size;
    rnd ^= arr[index] & zero;
    rnd = -~rnd;
  }
  struct timespec t3;
  timespec_get (&t3, TIME_UTC);

  // Calculate baseline and memory access times:
  double baseline_per_cycle =
      (double) (nanosectime (t1) - nanosectime (t0)) / (repeat);
  double memory_per_cycle =
      (double) (nanosectime (t3) - nanosectime (t2)) / (repeat);
  struct measurement result;

  result.baseline = baseline_per_cycle;
  result.access_time = memory_per_cycle;
  result.rnd = rnd;
  return result;
}

/**
 * Runs the logic of the memory_latency program. Measures the access latency for random and sequential memory access
 * patterns.
 * Usage: './memory_latency max_size factor repeat' where:
 *      - max_size - the maximum size in bytes of the array to measure access latency for.
 *      - factor - the factor in the geometric series representing the array sizes to check.
 *      - repeat - the number of times each measurement should be repeated for and averaged on.
 * The program will print output to stdout in the following format:
 *      mem_size_1,offset_1,offset_sequential_1
 *      mem_size_2,offset_2,offset_sequential_2
 *              ...
 *              ...
 *              ...
 */
int main (int argc, char *argv[])
{
  if (argc != 4)
  {
    std::cerr << "Incorrect usage. Usage: ./memory_latency max_size factor "
                 "repeat" << std::endl;
    return -1;
  }

  // Parse command line arguments
  uint64_t max_size = strtoull (argv[1], nullptr, 10);
  float factor = atof (argv[2]);
  uint64_t repeat = strtoull (argv[3], nullptr, 10);
  if (max_size < 100 || factor <= 1 || repeat <= 0)
  {
    std::cerr << "One or more of the arguments were invalid. Please make sure "
                 "maxsize>=100, factor>1, repear>0" << std::endl;
    return -1;
  }

  // zero==0, but the compiler doesn't know it. Use as the zero arg of
  // measure_latency and measure_sequential_latency.
  struct timespec t_dummy;
  timespec_get (&t_dummy, TIME_UTC);
  const uint64_t zero =
      nanosectime (t_dummy) > 1000000000ull ? 0 : nanosectime (t_dummy);

  // Generate array sizes based on geometric series
  uint64_t array_size = BASE_SIZE;
  while (array_size <= max_size)
  {
    // Allocate memory for the array
    auto *arr = (array_element_t *) malloc (array_size);
    if (arr == nullptr)
    {
      std::cerr << "Memory allocation failed." << std::endl;
      return -1;
    }
    
    // Initialize array elementss
    for(uint64_t j=1; j<array_size/sizeof(array_element_t); j++){
    	arr[j] = j;
    }

    // Measure access latency for random access pattern
    struct measurement random_latency = measure_latency (repeat, arr, array_size/sizeof(array_element_t), zero);

    // Measure access latency for sequential access pattern
    struct measurement sequential_latency = measure_sequential_latency (repeat, arr, array_size/sizeof(array_element_t), zero);
	
    // Free the allocated memory
    free (arr);	
	
    // Print the results to stdout
    std::cout << array_size << ","
              << random_latency.access_time - random_latency.baseline << ","
              << sequential_latency.access_time - sequential_latency.baseline
              << std::endl;

    
    // Update array size for next iteration
    array_size = (uint64_t) ceil (array_size * factor);
  }

  return 0;
}
