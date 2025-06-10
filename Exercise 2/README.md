<div align="center">

# Exercise 2 – User-Level Threads

**User-Level Threads** is the second assignment I completed in the *Operating Systems* course at the HUJI.

In this exercise, I implemented a **user-level threading library** in C++, exploring how to manage multiple threads in user space using mechanisms like `sigsetjmp`/`siglongjmp` and signal masking.  
This project emphasized the fundamentals of **context switching**, **signal handling**, and **thread scheduling** without kernel support.

[**« Return to Main Repository**](https://github.com/ShayMorad/Operating-Systems)

</div>



## Running the Project

To compile and run the thread library locally:

1. Clone the repository:
   ```bash
   git clone <repo_url>
   ```

2. Navigate to the project directory:
   ```bash
   cd Exercise 2
   ```

3. Compile the project using the provided Makefile:
   ```bash
   make
   ```

4. Run the test program or your own thread-based logic:
   ```bash
   ./user_threads
   ```

5. To clean the build files:
   ```bash
   make clean
   ```



## Summary of Topics

- Implemented a **user-level thread scheduler** using signal-based time slicing.
- Used **`sigsetjmp`/`siglongjmp`** for saving and restoring thread contexts.
- Explored **signal masking** to ensure atomic context switches.
- Understood advantages of user-level threads in scenarios such as:
  - Web servers handling many client requests
  - Fast context switching and minimal kernel overhead


##  Sample Q&A Highlights

- **Why `sigsetjmp` and `siglongjmp`?**  
  These functions allow saving and restoring CPU state, enabling user-level context switching.

- **Signal mask importance?**  
  Prevents race conditions during thread state transitions by temporarily blocking signals.

- **Use Case**:  
  A **web server** can use user-level threads to efficiently manage many lightweight connections.

- **Processes vs. Threads in Chrome**:  
  Each tab runs as a process for **security and isolation**, but this increases memory overhead.


##  Contributions

Contributions are welcome!  If you have suggestions or improvements, feel free to open an issue and start a discussion.


##  License

This project is licensed under the [MIT License](https://choosealicense.com/licenses/mit/).

