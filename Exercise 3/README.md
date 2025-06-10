<div align="center">

# Exercise 3 – Multi-Threaded MapReduce Framework

**MapReduce Framework** is the third assignment I completed in the *Operating Systems* course at the HUJI.

In this exercise, I implemented a **multi-threaded MapReduce framework** in C++.  
The project focused on advanced **thread synchronization**, **barrier coordination**, and efficient **parallel data processing**, inspired by the MapReduce programming model.

[**« Return to Main Repository**](https://github.com/ShayMorad/Operating-Systems)

</div>


## Running the Project

To compile and execute the MapReduce framework locally:

1. Clone the repository:
   ```bash
   git clone <repo_url>
   ```

2. Navigate to the project directory:
   ```bash
   cd Exercise 3
   ```

3. Compile the project using the provided Makefile:
   ```bash
   make
   ```

4. Run the program using your MapReduceClient.

5. To clean the build files:
   ```bash
   make clean
   ```

> The framework is modular — you can extend it by defining your own Map and Reduce logic.



## Summary of Topics

- Designed a thread-safe **MapReduce API** from scratch.
- Implemented:
  - **Thread pools** for mapper and reducer execution.
  - A custom **Barrier** class to synchronize phases.
  - **Thread-safe queues** and data structures.
- Learned the nuances of:
  - **Workload balancing**
  - **Race condition prevention**
  - **Efficient parallel processing**



## Example Highlights

- **Barrier Usage**:  
  Ensures all mapper threads complete before starting reducers.

- **Map and Reduce Phase Isolation**:  
  Data produced by mappers is sharded and sorted before reduction.

- **Real-World Parallelism**:  
  Mimics large-scale systems like Hadoop or Spark, but simplified for low-level C++.


## Contributions

Contributions are welcome!  Feel free to open an issue if you want to suggest improvements or extend the framework.



## License

This project is licensed under the [MIT License](https://choosealicense.com/licenses/mit/).

