# Computer-Scheduling-System-Simulation

It is a single-threaded program to simulate a scheduling system of a computer.

## Introduction
The scheduling system is able to admit new processes to ready queue(s), select a process from ready queue to run using a particular scheduling algorithm, move the running process to a blocked queue when it has to wait for an event like I/O completion or mutex signals and put them back to ready queue(s) when the event occurs.

## Overall Work Flow
The simulated scheduling system of a computer is implemented with only one CPU, one keyboard and one disk. Each of these devices provides service to one process at a time, so scheduling is required to coordinate the processes. The scheduling system is able to work in a loop to emulate the of behavior the CPU. Each loop is called a tick. In each loop iteration, the tick is incremented by one to emulate the elapsed internal time of the CPU.

![image](https://user-images.githubusercontent.com/80200340/141319336-f6caa2e1-6fa5-412a-bd74-85b45c83a025.png)
<p align="center">Figure 1: Overall work flow of your scheduler in each tick</p>
As shown in Fig.1, the overall work flow of the scheduler in each tick can be divided into three steps:

<ul>
<li>Step 1. At the beginning of each tick, the scheduler would check whether there are new processes to be admitted to the ready queue. If multiple processes arrive at this tick, they are enqueued in ascending order of their process IDs.</li>
<li>Step 2. There is one block queue for each I/O device and each mutex. Processes waiting for an I/O device or a mutex are inserted into the corresponding block queue. The event handler would dispatch processes from block queues in FCFS manner to the I/O devices they are waiting for if the devices become available. If a process is done with the device, it will be re-inserted to the ready queue. Besides, the event handler would also check processes waiting for mutexes. Similar to I/O, only the first process waiting in the mutex block queue is able to lock the mutex after the mutex is unlocked by another process.</li>
<li>Step 3. If there is no process running on the CPU, the scheduling system would dispatch a process to CPU from the ready queue according to one of the following scheduling algorithms: First-Come-First-Served (FCFS), Round Robin (RR) and Feedback Scheduling (FB). At the end of the tick, the scheduler should look ahead the service requested by the currently running process in the next tick to determine the action required. For example, if the service next tick is disk I/O, then blocking of the current process is required. If mutex operations (lock & unlock), which are assumed to have a time cost of zero tick, are encountered while looking ahead, they should be executed immediately and the scheduler will keep finding the next service which is not a mutex operation.</li>
<li>Finally, if the process on CPU uses up its time quantum, it is preempted and placed at the end of the ready queue (RR & FB only).</li>
</ul>

To make things simpler, when a process is dispatched to CPU or I/O devices, it can be kept in its current ready queue or block queue. In other word, there is no need to move the processes somewhere else.

## Processes
The information of processes is given in a text file with the following format.

#&#x2060;process_id arrival_time service_number

(followed by service_number lines)

service_type service_description

service_type service_description

… … … …

service_type service_description

There are 3 integers in the first line and the # marks the beginning of a process. The first number is the process ID. The second number in the arrival time of the process in number of ticks. The third number is the number of services requested by the process. Services are the resources the process needs or some particular operations that the process executes. The following 𝑠𝑒𝑟𝑣𝑖𝑐𝑒_𝑛𝑢𝑚𝑏𝑒𝑟 lines are the services requested and the description of the service. There are 5 different types of services, namely C (CPU), K (keyboard input), D (disk I/O), L (mutex lock) and U (mutex unlock).

For service type C, K and D, the service description is an integer which is the number of ticks required to complete the service. It is worth noting that the number of ticks required to complete service K or D does NOT include the waiting time in the block queue. The keyboard and the disk provide I/O service to processes in a FCFS manner. In other words, only the first process in each block queue can receive I/O service. For service type L (mutex lock) and U (mutex unlock), the service description is a string representing the name of the mutex. Similarly, if multiple processes are waiting for a mutex to unlock, after other process unlocks it, the first process waiting in the queue would get the mutex and lock it. Note that we assume the lock and unlock operation take 0 tick, which means they should be execute immediately.

#&#x2060; 0 0 8

C 2

D 6

C 3

K 5

C 4

L mtx

C 5

U mtx

Above is an example of a short process. This process with ID 0 arrives at your scheduling system at tick 0. It requires 8 services in total. To complete its task, we need to schedule 2 ticks on CPU, 6 ticks for disk I/O, then another 3 ticks on CPU, 5 ticks to wait for keyboard input, then 4 ticks on CPU, 0 tick to lock the mutex mtx, 5 ticks on CPU, and finally 0 tick to unlock the mutex mtx.

For simplicity and reasonability, the possible service type at the end of every service sequences could only be C or U. And the process IDs are assumed to be consecutive integers from 0 to N-1, if there are N processes.

## Blockings
Several events may lead to the blocking of current process, for example, I/O interrupts or mutex lock. Multiple blocked queues are implemented for efficiently storing processes that are blocked by such events. To be more exact, every kind of I/O interrupt (K and F) and every mutex should have its own blocked queue. The event handler of the scheduling system will then handle the processes in blocked queues in a FCFS manner: only the first process at each blocked queue can receive the I/O service or mutex signals.

### I/O Interrupts
Two I/O interrupts are implemented in the scheduling system: keyboard input and disk I/O. When an I/O interrupt occurs, the scheduling system will block the current process, putting it into the corresponding blocked queue and after I/O completes, put the process back to ready queue.

### Mutexes
Since the scheduling system is a single-threaded program, the mutexes from pthread.h cannot be applied to the system. Instead, mutex structure is implemented by myself to allow mutual exclusion in the simulated system. The mutex has one variable to store its status (locked or unlocked) and has two functions mutex_lock and mutex_unlock to lock or unlock the mutex, just like the ones in pthread.h. All mutexes used by the given processes would be inferred from the given text file. Additionally, when several processes are waiting for a mutex in block queue, only the one at the front can get the mutex once it is unlocked.

## Scheduling Algorithms
3 scheduling algorithms are implemented: FCFS, RR and Feedback scheduling.

## Input & Output samples
The program accepts 3 arguments from command line. The program will output a text file which stores the scheduling details of each processes. For each process, it will write two lines. The first line is the process ID of the process, i.e. process 0. The second line includes the time period that the process runs on CPU. Specifically, if a process is scheduled to run on CPU from the 𝑎-th tick to 𝑏-th tick and from 𝑐-th tick to 𝑑-th tick, then second line should be “𝑎 𝑏 𝑐 𝑑”.
