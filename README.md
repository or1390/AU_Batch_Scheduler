# AU_Batch_Scheduler

#### The aim of this project is to design and implement a batch scheduling system using Pthread library,conditional variables and three scheduling policy. It consists of two collaborating threads, the scheduler and the dispatcher which are created the by pthread_create() function. To synchronize and coordinate two threads we are using condition variables. The scheduling thread enforces scheduling policies, whereas the dispatcher thread has submitted job. The scheduling algorithms used are: FCFS, SJF and Priority based scheduling algorithms. These algorithms are evaluated by using various metrics: throughput, average CPU time, average waiting time and average turnaround time.



## How to run the program: 
  1. Copy the files into your system located in a folder
  2. Open this directory(which contains files) in terminal. 
  3. Compile using “make” command. Type **make**
  4. run **./aubatch**


## How to use the program
  1. run **./aubatch.<**
  2. type **‘h’** or **‘help’** or **‘?’** to open the help menu.
  3. run command to submit a job:
      * #### run<job_name><time> <prio>
  4. list command to display the status and other information about the data in the queue.
  5. test command the format is:
      * #### test <benchmark><policy><num_of_jobs><arrival_rate><priority_levels><min_CPU_time><max_CPU_time>

  
 ## The metrics used to evaluate the performance of the policies are:
   * #### Average turnaround time – is calculated as a difference of completion time and arrival time of each job to be executed, and then the summation of all of them.
   * #### Average CPU (burst) time – the summation of all burst time divided by the number of jobs.
   * #### Average waiting time – difference of turnaround time with burst time for each job and summation of all the difference. Then, we calculate the average by dividing with the number of jobs.
   * #### Throughput – the number of jobs in a unit time, which is calculated by the difference of the (ending time – start time) of the execution of the test command.
  
   #### In the folder, we have provided a list of test cases and their results. ( Note: test_cases.docx)
  
  
