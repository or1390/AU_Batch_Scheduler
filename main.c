/*******************************************************************************************
							PROJECT 3
						AU BATCH SCHEDULER
							Ornela Hogu
*******************************************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

/* Error Codes */
#define EINVAL       1
#define E2BIG        2
#define SIZE 		 50
#define MAX		 	 20

#define MAXMENUARGS  8
#define MAXCMDLINE   64 

#define CMD_BUF_SIZE 10 /* The size of the command queue */ 
#define NUM_OF_CMD   5  /*Number of scheduler->dispatcher operations*/ 
#define MAX_CMD_LEN  256 /* The longest commandline length */ 

pthread_mutex_t cmd_queue_lock;   /*Lock for critical sections*/ 
pthread_mutex_t cmd_run_lock;     /*Lock for critical sections*/ 
pthread_cond_t cmd_buf_not_full;  /*Condition variable for buf_not_full*/ 
pthread_cond_t cmd_buf_not_empty; /*Condition variable for buf_not_empty*/ 

/*Global shared variables*/
time_t start_program_time; 
unsigned int rear  = SIZE - 1; // Initally assumed that rear is at end
unsigned int front = 0;
int size  = 0;
int testing , count, done_index= 0;
int flag_policy = 0;  /*this flag is used to change the scheduling algorithm */

//Structure of BUFFER 	  
struct job
{
	char name[10];
	float burst_time;
	int priority;
	int job_ID;
	time_t	arrival_time;
	time_t  completion_time;
	double ta;
	double waiting_time;
};

typedef struct job job;

job newly_added;// this buffer is used in run command
job queue [SIZE]; //this buffer is used to keep track of Pending jobs
job completed[SIZE];//this buffer is used to keep track of Completed jobs
job key; //a temporal buffer to order the queue for the scheduling algorithms. This is used in switch_policy function.
job to_be_executed; //this buffer is used in the dispatcher to keep track of the running job


/*Enum for scheduling algorithms*/
typedef enum {
    FCFS,
	SJF,
    Priority,
 } Policy;

Policy policy = FCFS;

//UI functions definitions
void menu_execute(char *line, int isargs); 
int cmd_run(int nargs, char **args); 
int cmd_quit(int nargs, char **args); 
int cmd_list(int nargs, char **args);
int cmd_test(int nargs, char **args);
int cmd_dispatch(char *cmd);
void showmenu(const char *name, const char *x[]);


//UI functions to manage the policies
int cmd_fcfs(int nargs, char **args);
int cmd_sjf(int nargs, char **args);
int cmd_priority(int nargs, char **args);
void switch_policy(Policy policy);
//Functions to manage the queue
int queue_is_full();
int queue_is_empty();
void enQueue(job);
job deQueue();
//Function to generate random numbers in a range
float printRandrom (float lower, float upper);
//Function used in test and quit commands to display the same data
void display_output(int job_num, double avg_ta_time, float avg_burst_time, double  avg_waiting_time, double throughput);


//Core functions definitions
void *scheduler( void *ptr ); /*Scheduler*/
void *dispatcher( void *ptr );/* Dispatcher */

/*
This function is used to order the jobs that will be executed(Status--> Pending) in the queue 
by using three scheduling algorithms: FCFS, SJF and Priority. 
E.g. When we run from command prompt: sjf
then list it will list the jobs based on their CPU time in asceding order.
This procedure is the same for FCFS and Priority. Priority is designed to order from the highest 
to the lowest order.
*/
 
void switch_policy(Policy policy){
	int i,j;	
	switch(policy){
		case FCFS:		
			for(i=front;i<rear;i++){
				for(j=front;j<rear;j++){
					if(queue[j].arrival_time > queue[j+1].arrival_time )
					{
						key = queue[j];
						queue[j] = queue[j+1];
						queue[j+1] = key;
					}
				}
			}
			break;
		case SJF:	
			for(i=front;i<rear;i++){
				for(j=front;j<rear;j++){
					if(queue[j].burst_time > queue[j+1].burst_time )
					{
						key = queue[j];
						queue[j] = queue[j+1];
						queue[j+1] = key;
					} 
				
				}
			}
			break;

		case Priority:			
			for(i=front;i<rear;i++){
				for(j=front;j<rear;j++){
					if(queue[j].priority < queue[j+1].priority )
					{
						key = queue[j];
						queue[j] = queue[j+1];
						queue[j+1] = key;
					} 
				}
			}
			break;
		default:
			break;	
	}

}
//The run command - submit a job
int cmd_run(int nargs, char **args) {
	

	if (nargs != 4) {
		printf("Usage: run <job> <time> <pri>\n");
		return EINVAL;
	}

	pthread_mutex_lock(&cmd_run_lock);
	strcpy(newly_added.name, args[1]);
	newly_added.burst_time = atof(args[2]);
    newly_added.priority = atoi(args[3]);
    newly_added.job_ID = 1; 
    time(&newly_added.arrival_time);  
    pthread_mutex_unlock(&cmd_run_lock);


    return 0;	

	
}

/*Quit command - displays the performance of the jobs
and exit the program
*/
int cmd_quit(int nargs, char **args) {
	int r;
	time_t end_test_time;
	float avg_burst_time ,arrival_rate, throughput;
	double avg_ta_time, avg_waiting_time, test_time= 0;

	job job_test;
 
	for(r= 0;r < done_index; r++){

		job_test.burst_time 		+= completed[r].burst_time;
	    job_test.arrival_time 		+= completed[r].arrival_time;
	    job_test.completion_time 	+= completed[r].completion_time;
	   	double job_ta = difftime(completed[r].completion_time , completed[r].arrival_time);
	   	job_test.ta += job_ta;	   
	    job_test.waiting_time 		+= job_ta - completed[r].burst_time;	
	}
	time(&end_test_time);

	avg_burst_time   = job_test.burst_time/done_index;
	avg_ta_time      = job_test.ta/done_index;
	avg_waiting_time = job_test.waiting_time/(float)done_index;
	test_time 		 = difftime(end_test_time, start_program_time);
	throughput 		 = done_index/test_time;


	display_output(done_index, avg_ta_time,avg_burst_time,avg_waiting_time,throughput);
 	exit(0);
}


//Help menu
int cmd_helpmenu(int nargs, char **args){

	(void)nargs;
	(void)args;
	printf("\t run <job> <time><pri>: submit a job named <job>,\n\t\t\t\texecution time is <time>,\n\t\t\t\tpriority is <pri>.\n");
	printf("\t list: display the job status.\n");
	printf("\t fcfs: change the scheduling policy to FCFS.\n\t sjf: change the scheduling policy to SJF.\n\t priority:"
		" change the scheduling policy to priority.\n\t test: <benchmark> <policy> <num_of_jobs> <arrival_rate> <priority_levels> <min_CPU_time> <max_CPU_time>"
		"\n\t quit: exit AUbatch\n");
	return 0;
}
//list command
int cmd_list(int nargs, char**args){

 	int i;
	printf("*********************************************************************************\n");

	if(policy == FCFS) {
	    printf("Total number of jobs in the queue:%d\nScheduling Policy: FCFS \n", count);
	    } else if (policy == SJF) {
	   	 printf("Total number of jobs in the queue:%d\nScheduling Policy: SJF \n", count);
	    } else if (policy == Priority)
	    {
	   	 printf("Total number of jobs in the queue:%d\nScheduling Policy: Priority \n", count);
	    }

	printf("**********************************************************************************\n");
	printf("Name\tCPU_time\tPriority\tArrival_Time\tProgress\n");
	printf("**********************************************************************************\n");
	char* time_string = ctime(&to_be_executed.arrival_time);
	time_string[strlen(time_string) - 5] = '\0';
		
	for (i = 0; i < done_index; i++)
	{
	
		time_string = ctime(&completed[i].arrival_time);
		time_string[strlen(time_string) - 5] = '\0';
		printf("%s\t%4.2f\t\t%d\t\t%s\tCompleted\n", completed[i].name,completed[i].burst_time, completed[i].priority,time_string+11);
	}
	if (to_be_executed.job_ID > 0){

		printf("%s\t%4.2f\t\t%d\t\t%s\tRun\n", to_be_executed.name,to_be_executed.burst_time,to_be_executed.priority, time_string+11);
	}

	if (!queue_is_empty())	{
		for(i=front;i<=rear;i++) {
		time_string = ctime(&queue[i].arrival_time);
		time_string[strlen(time_string) - 5] = '\0';
		printf("%s\t%4.2f\t\t%d\t\t%s\tPending\n", queue[i].name,queue[i].burst_time, queue[i].priority,time_string+11);
	}
	}
	else{
	
		printf("\t\t\tNo jobs in the queue\n");

	}
	return 0;
}

// Function to generate random number between a range
float printRandrom (float lower, float upper){	
	return ((upper-lower) * ((float)rand() / RAND_MAX)) + lower;

}
//Test command
int cmd_test(int nargs, char **args){
	/*Local variables*/	
	char* temp[4];
	char s_time[10];
	char priority_levels [5] ;
	char policy_used [8];
	char benchmark [10];
	float min_time, max_time = 0;
	float float_time;
	time_t start_test_time, end_test_time;
	int  job_count =0;
	
	int k,r, i,j;
	int int_time, priority , levels;	
	
	float avg_burst_time ,arrival_rate, throughput;
	double avg_ta_time, avg_waiting_time, test_time= 0;

	job job_test;
	testing = 1;
	strcpy(policy_used, args[2]);

	if ( nargs != 8) {		
		printf("test <benchmark> <policy> <num_of_jobs> <arrival_rate> <priority_levels> <min_CPU_time> <max_CPU_time> \n");
		testing = 0;
		return EINVAL;
	}

	if(strcmp("batch_job",args[1])!=0)
	{
	 	printf("The benchmark used is wrong. You should use: batch_job\n");
	 	return 0;
	} 
	if(strcmp("fcfs",policy_used)!=0 && strcmp("sjf",policy_used)!=0 && strcmp("priority",policy_used) !=0)
	{
		printf("Policy is not supported!The supported policies are:'fcfs','sjf','priority'\n");
		return 0;
	}
	if(strcmp("fcfs",policy_used) == 0)
	{
		policy = FCFS;
	}

	if(strcmp("sjf",policy_used) == 0)
	{
		policy = SJF;
	}

	if(strcmp("priority",policy_used) == 0)
	{
		policy = Priority;

	}


	done_index =0 ; 
	size  = 0;
	rear  = SIZE - 1;   // Initally assumed that rear is at end
	front = 0;

	job_count 	 = atoi(args[3]);
	min_time 	 = atof(args[6]);
	max_time  	 = atof(args[7]);
	levels     	 = atoi(args[5]);
	arrival_rate = atof(args[4]);
	strcpy(policy_used, args[2]);

	printf("Testing....\n");
	time(&start_test_time);
	for (k = 0; k < job_count; k++){
		temp[0] = "r";
		temp[1] = "job";

	   	float_time = printRandrom(min_time, max_time);
	  	gcvt(float_time, 4, s_time);	
	   	temp[2] = s_time;
	   	priority = printRandrom(1, levels);
	 	sprintf(priority_levels, "%ld",priority);
	   	temp[3] = priority_levels;
	   	cmd_run(4, temp);
	   	if(arrival_rate < 1)
	   	{
	   		sleep(1/arrival_rate);

	   	}
	   	else if(arrival_rate >= 1){
			if(k % (int)arrival_rate == 0){
				sleep(1);
			}
	   	}
	 usleep(200);
	}
	
	while(count > 0 || to_be_executed.job_ID != -2){
	}
	for(r= 1;r < done_index; r++){

		job_test.burst_time 		+= completed[r].burst_time;
	    job_test.arrival_time 		+= completed[r].arrival_time;
	    job_test.completion_time 	+= completed[r].completion_time;
    	double job_ta = difftime(completed[r].completion_time , completed[r].arrival_time);
	   	job_test.ta += job_ta;	   
	    job_test.waiting_time 		+= job_ta - completed[r].burst_time;	
	}
	time(&end_test_time);
	avg_burst_time   = job_test.burst_time/job_count;
	avg_ta_time      = job_test.ta/job_count;
	avg_waiting_time = job_test.waiting_time/(float)job_count;
	test_time 		 = difftime(end_test_time, start_test_time);
	throughput 		 = job_count/test_time;

	display_output(job_count, avg_ta_time,avg_burst_time,avg_waiting_time,throughput);
	testing = 0;
 	return 0;

}

// this function is used to display data in the list and test command. 
void display_output(int job_num, double avg_ta_time, float avg_burst_time, double  avg_waiting_time, double throughput){

	printf("Total number of jobs submitted:\t%d\n", job_num);
	printf("Average turnaround time:\t%4.2f\tseconds\n", avg_ta_time);
	printf("Average CPU time:\t\t%4.2f\tseconds\n", avg_burst_time);
	printf("Average waiting time:\t\t%4.2f\tseconds\n", avg_waiting_time);
	printf("Throughput:\t\t\t%4.2f\tNo./seconds\n",throughput);

	
}

//fcfs policy
int cmd_fcfs(int nargs, char **args)
{
	(void)nargs;
	(void)args;
	policy = FCFS;
	
	printf("Scheduling policy is switched to FCFS. All the %d waiting jobs have been rescheduled\n", count);
	flag_policy =1;
	return 0;
}

//sjf policy
int cmd_sjf(int nargs, char **args)
{
	(void)nargs;
	(void)args;
	policy = SJF;
	
	printf("Scheduling policy is switched to SJF. All the %d waiting jobs have been rescheduled\n", count);			
	flag_policy = 1;
	return 0;
}
//priority policy
int cmd_priority(int nargs, char **args)
{
	(void)nargs;
	(void)args;
	policy = Priority;
	
	printf("Scheduling policy is switched to Priority. All the %d waiting jobs have been rescheduled\n", count);
	flag_policy = 1;
	return 0;
}
//disply menu information - Dr. Qin code
void showmenu(const char *name, const char *x[])
{
	int ct, half, i;
	printf("\n");
	printf("%s\n" , name);

	for (i=ct=0; x[i]; i++){
		ct++;
	}
	half = (ct+1)/2;

	for (i=0; i<half; i++){
		printf("  %-36s", x[i]);
		if(i+half < ct){
			printf("%s", x[i+half]);
		}
		printf("\n");
	}
	printf("\n");
}

//Command table- Dr. Qin code
static struct {
	const char *name;
	int (*func)(int nargs, char **args);
} cmdtable[] = {
	/* commands: single command must end with \n */
	{ "?\n",	cmd_helpmenu },
	{ "h\n",	cmd_helpmenu },
	{ "help\n",	cmd_helpmenu },
	{ "r",		cmd_run },
	{ "run",	cmd_run },
	{ "q\n",	cmd_quit },
	{ "quit\n",	cmd_quit },	
    {"fcfs\n", 	cmd_fcfs},
	{"sjf\n", 	cmd_sjf},
	{"priority\n", 	cmd_priority},
	{"pri\n", 	cmd_priority},
	{"test", 	cmd_test},
	{"list\n", 	cmd_list},
	{	NULL,	NULL}
	
};

//Execute a command- Dr. Qin code
int cmd_dispatch(char *cmd)
{
	time_t beforesecs, aftersecs, secs;
	u_int32_t beforensecs, afternsecs, nsecs;
	char *args[MAXMENUARGS];
	int nargs=0;
	char *word;
	char *context;
 	int i, result;

	for (word = strtok_r(cmd, " ", &context);
	     word != NULL;
	     word = strtok_r(NULL, " ", &context)) {

		if (nargs >= MAXMENUARGS) {
			printf("Command line has too many words\n");
			return E2BIG;
		}
		args[nargs++] = word;
	}

	if (nargs==0) {
		return 0;
	}

	for (i=0; cmdtable[i].name; i++) { 
		if (*cmdtable[i].name && !strcmp(args[0], cmdtable[i].name)) {
			assert(cmdtable[i].func!=NULL);
			result = cmdtable[i].func(nargs, args);
			return result;
		}
	}

	printf("%s: Command not found\n", args[0]);
	return EINVAL;
}

/*This functions are essential to manage the status of the jobs in queue.
They provide several controls if the queue is empty or full. 
In addition, when a new job is submitted we use enqueue and after the completion
we use dequeue.*/
void enQueue(job value){
      // Ensure rear never crosses array bounds
    rear = (rear + 1) % SIZE;
    // Increment queue size
    size++;
    // Enqueue new element to queue
    queue[rear] = value;

}

job deQueue(){
    job toReturn = queue[front];
    // Ensure front never crosses array bounds
    front = (front + 1) % SIZE;
    // Decrease queue size
    size--;
    return toReturn;
}

int queue_is_full(){
    return (size == SIZE);
}


int queue_is_empty(){
	 return (size == 0);
}

/*The aim of the scheduler module is to accept the submitted jobs, 
and to enforce the scheduling polices. In this module is crucial to use
conditional variables to coordinate and synchronize the execution of the jobs.
It is designed to evaluate different metrics according to the policy we are using.*/

void *scheduler(void *ptr){  
	job job_scheduler;	
	int i, j;
    while(1){ 
       	pthread_mutex_lock(&cmd_queue_lock);
       	while (queue_is_full())	{
       		printf("Queue is full\n");
       		pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
	     }  
       	pthread_mutex_lock(&cmd_run_lock);
	     if(newly_added.job_ID != 0) {

	     	enQueue(newly_added); 
	       	count++;
	     	switch_policy(policy);
  	
	   
			if ((testing == 0 )){

				printf("\nJob %s was submitted\n",newly_added.name);
				printf("Total number of jobs in the queue:%d\n", count);
				printf("Expected waiting time:%.f\n",to_be_executed.burst_time);
			    printf("Scheduling Policy: ");
			    if( policy == FCFS) {
			    	printf("FCFS\n");
			    } else if (policy == SJF) {
			    	printf("SJF\n");

			    } else {
			    	printf("Priority\n");

			    }	
	
			}
		   	
	   
	      	pthread_cond_signal(&cmd_buf_not_empty);  	
	       	newly_added.job_ID = 0;
  	        	    
		}
		if(flag_policy == 1){			
			switch_policy(policy);
			flag_policy = 0;
		}

		pthread_mutex_unlock(&cmd_run_lock);
		pthread_mutex_unlock(&cmd_queue_lock);	
	}
		
}
/*The aim of the dispatcher module is to accept the submitted jobs, 
and it launches a benchmark called 'batch_job'. Even here is crucial
the usage of the conditional variables to save critical sessions*/

void *dispatcher(void *ptr){
    time_t now;
   	pid_t pid;	
   

     while(1){      
       	pthread_mutex_lock(&cmd_queue_lock);
       	while (queue_is_empty())
       	{
       		pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
       	}

     	// this means there is a job to get
     	to_be_executed = deQueue();   
     	count--;
      	pthread_mutex_unlock(&cmd_queue_lock);		
	
		switch ((pid = fork()))
		{
			case -1:
				perror("fork failed");
				break;
			case 0:;
				char* args[3] ;
				char float_num [10];
				args[0] = "./batch_job";
				gcvt(to_be_executed.burst_time, 4, float_num);	
				args[1] = float_num;
				args[2] = NULL;
				execv("./batch_job", args);
				perror("Fail\n");
				exit(0);
				
				break;
			default:
			wait(NULL);
			char* priority[4] ;
			char int_priority[10];
		    sprintf(int_priority, "%ld",to_be_executed.priority);
			priority[3] = int_priority;
			time(&to_be_executed.completion_time);
			double ta = difftime(to_be_executed.completion_time , to_be_executed.arrival_time);
			double waiting_time = ta - to_be_executed.burst_time;
			to_be_executed.waiting_time = waiting_time;
			to_be_executed.ta = ta;
			completed[done_index] = to_be_executed;
			to_be_executed.job_ID = -2;
			done_index++;			

		}
		
	pthread_cond_signal(&cmd_buf_not_full);

  }  
         
}


int main(){
	//front = rear = -1;
	time(&start_program_time);
	char *buffer;
    size_t bufsize = 64;
    newly_added.job_ID = 0;

    int iret0, iret1;
	pthread_t scheduler_thread, dispatcher_thread; /* Two concurrent threads */  
    char *message0 = "Scheduler Thread";
    char *message1 = "Dispatcher Thread";

    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_mutex_init(&cmd_run_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);    

/* Create two independent threads:scheduler and dispatcher */
    iret0 = pthread_create(&scheduler_thread, NULL, scheduler, (void*) message0);
	iret1 = pthread_create(&dispatcher_thread, NULL, dispatcher, (void*) message1);

	if (iret0 != 0 || iret1 !=0)
	{
		printf("Threads creation failed!\n");
	}
 	
 	pthread_detach(scheduler_thread);
 	pthread_detach(dispatcher_thread);

    printf("Welcome to batch job scheduler\n");
	printf("Type `help` to find more about AUbatch commands\n");
	printf("***********************************************\n");
	buffer = (char*) malloc(bufsize * sizeof(char));
        if (buffer == NULL) {
 		perror("malloc buffer failed");
 		exit(1);
 	}

	while (1) {
		printf(">[? for menu]:");
		getline(&buffer, &bufsize, stdin);
		cmd_dispatch(buffer);
	}

	pthread_join(scheduler_thread,NULL);
	pthread_join(dispatcher_thread,NULL);
 	return 0;

}

