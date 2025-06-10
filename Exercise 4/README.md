<div align="center">

# Exercise 4 – Virtual Memory Management

**Virtual Memory Management** is the fourth assignment I completed in the *Operating Systems* course at the HUJI.

In this exercise, I implemented a **custom virtual memory management system** in C++, simulating how modern operating systems handle memory pages, frame allocation, and page replacement policies.  
The project emphasized low-level memory control, **paging**, **page faults**, and algorithmic implementations such as **LRU**, **FIFO**, and more.

[**« Return to Main Repository**](https://github.com/ShayMorad/Operating-Systems)

</div>


## Running the Project

To compile and run the virtual memory system locally:

1. Clone the repository:
   ```bash
   git clone <repo_url>
   ```

2. Navigate to the project directory:
   ```bash
   cd Exercise 4
   ```

3. Compile the project using the provided Makefile:
   ```bash
   make
   ```

4. Run a test program or your custom logic:
   ```bash
   ./virtual_memory
   ```

5. To clean the build files:
   ```bash
   make clean
   ```

> You can modify or extend the simulation parameters to test different access patterns or replacement strategies.


## Summary of Topics

- Simulated a full **virtual memory system** with:
  - Logical to physical address translation
  - Page table management
  - Frame tracking
- Implemented and compared multiple **page replacement policies**:
  - **LRU (Least Recently Used)**
  - **FIFO (First In, First Out)**
  - **Custom eviction strategies**
- Analyzed the impact of:
  - **Page fault rates**
  - **Access locality**
  - **Frame constraints**



## Key Takeaways

- **Paging vs. Segmentation**:  
  Reinforced how paging allows non-contiguous allocation of memory blocks while maintaining process isolation.

- **Page Fault Handling**:  
  Implemented logic to trap and resolve faults when a page is not loaded in physical memory.

- **Eviction Algorithms**:  
  Explored how different policies affect system performance under high memory pressure.



## Contributions

Contributions are welcome!  If you have suggestions or improvements, feel free to open an issue and start a discussion.



## License

This project is licensed under the [MIT License](https://choosealicense.com/licenses/mit/).

