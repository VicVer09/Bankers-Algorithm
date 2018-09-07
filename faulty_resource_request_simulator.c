


#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <unistd.h>



/*


PART 1:

	Input:
	number of processes
	number of district resources
	amount of each resource in the system
	maximum resource claim per process / resource 

	Goal:
	Simulate each process using a thread:
		- all threads run all resources are used then terminate
		- each time requests i j instances of each resource j
			-> i is randomly selected as a value between 0 and NEED
		- checks if granting the resource request will result in a safe state
			-> if yes, resources granted and process continues (by sleeping for 3 seconds)
			-> else, process blocked.
	Us mutex/semaphores

	Program flow:

	1. Processes begin

	2. while a process executes, a resource request vector is generated for that process.

	This request vector contains i j instances of each resource j, where i is randomly
	selected as a value between 0 and the remaining amount of needed resources to
	terminate.

	3. A deadlock avoidance algorithm is then run (Bankers) in order to check whether
	allocating these resources will result in a safe state.

	4. if allocation is safe, then the process acquires the resources and the can continue
	executing.

	5. it checks at this point if there are remaining requests to be made by that process, if
	not, then the process can terminate. If there are remaining requests, then simulating
	continued execution is done by sleeping for 3 seconds and then making a new request.

	6. if allocation is unsafe, then the process blocks until another process finishes and
	relinquishes its resources.


PART 2:

Introduce faulty resources to the system

Faulty thread:
	- run every 10 seconds
		-> create a fault a random resources with a probability of 50%
		if a fault occurs for a particular resource, then avail decreased by 1

	-  Banker’s algorithm may not be able to avoid deadlock in this case
		-> still use Banker’s algorithm to allocate resources, deadlock might still occur. 
		-> need to detect deadlock

Deadlock detection:
	- will be run every 10 seconds
		-> check if process needs higher avail for all processes
	- if no process can acquire all the resources it needsto complete, then deadlock
		-> output the following and exit:
		Deadlock will occur as processes request more resources, exiting...


*/

// Global Banker's Algorithm Variables
int num_processes;
int num_resources;
int **max;
int **need;
int **hold;
int *avail; 

pthread_mutex_t lock;

void printAll() {

	int i,j;
	printf("\nAvailable resources: \n");
	for (j = 0; j < num_resources; j++) printf("%i ", avail[j]);
	printf("\n\n");

	printf("Allocated resources table (hold): \n");
	for (i = 0; i < num_processes; i++) {
		for (j = 0; j < num_resources; j++) {
			printf("%i ", hold[i][j]);
		}
		printf("\n");
	}
	printf("\n");

	printf("Maximum claim table (max): \n");
	for (i = 0; i < num_processes; i++) {
		for (j = 0; j < num_resources; j++) {
			printf("%i ", max[i][j]);
		}
		printf("\n");
	}
	printf("\n");


	printf("Need table (need): \n");
	for (i = 0; i < num_processes; i++) {
		for (j = 0; j < num_resources; j++) {
			printf("%i ", need[i][j]);
		}
		printf("\n");
	}
	printf("\n\n");

}




/*
Implementation of isSafe() as described in the slides
*/
int isSafe(){

	int *work = malloc(num_resources * sizeof(int));
	int *finish = malloc(num_processes * sizeof(int));
	int i, j;

	// Step 1: Initialize
	for (j = 0; j < num_resources; j++) work[j] = avail[j];
	for (i = 0; i < num_processes; i++) finish[i] = 0;

	while (1) {

		// Step 2: 
		int unfinished_process = 0;
		int resource_to_be_granted = 0;
		int found_unfinished_process = 0;
		int go_to_step_4 = 1;
		for (i = 0; i < num_processes; i++) {
			for (j = 0; j < num_processes; j++) {

				if (finish[i] == 0 && need[i][j] <= work[j]) {
					unfinished_process = i;
					resource_to_be_granted = j;
					found_unfinished_process = 1;
					go_to_step_4 = 0;
				}

			}
			if (found_unfinished_process == 1) break;
		}

		// Step 3: 
		int go_to_step_2 = 0;
		if (go_to_step_4 == 0){
			work[resource_to_be_granted] += hold[i][j];
			finish[unfinished_process] = 1;
			go_to_step_2 = 1;
		}

		// Step 4:
		if (go_to_step_4 == 1) {
			int true_for_all_i = 1;
			for (i = 0; i < num_processes; i++) if (finish[i] == 0) true_for_all_i = 0;
			return true_for_all_i;
		}
	}

}




/*
Implementation of Bankers Algorithm as described in the slides
returns 1 if safe allocation 0 if not safe
*/
int bankers_algorithm(int process_id, int* request_vector){

	int i, j, inner_wait = 1, outer_wait = 1;
	while (outer_wait == 1) {
		while (inner_wait == 1) {
			// Step 1: Verify that the request matches the needs

			for (j = 0; j < num_resources; j++) {
				if (request_vector[j] > need[process_id][j]) return -1;
			}

			// Step 2: check if requested amount is available
			inner_wait = 0;
			for (j = 0; j < num_resources; j++) {
				if (request_vector[j] > avail[j]) inner_wait = 1; 
			}
			if (inner_wait == 1) sleep(1); // Go back to step 1, Pi must wait
		}

		// Step 3: Provisional allocation
		pthread_mutex_lock(&lock);
		

		for (j = 0; j < num_resources; j++) {
			avail[j] -= request_vector[j];
			hold[process_id][j] += request_vector[j];
			need[process_id][j] -= request_vector[j];
		}
		if (isSafe() == 1) {
			// Grant resources
			printf("Allocation is safe, resources granted\n\n");
			printAll();
			outer_wait = 0;
		} else {
			// Cancel allocation
			printf("Allocation is unsafe, process %i must wait\n", process_id);
			for (j = 1; j < num_resources; j++) {
				avail[j] += request_vector[j];
				hold[process_id][j] -= request_vector[j];
				need[process_id][j] += request_vector[j];
			} 
			outer_wait = 1; 
		}

		pthread_mutex_unlock(&lock);
		if (outer_wait == 1) sleep(1);
	}

    return 1;

}

/*
Simulates resource requests by processes
*/
int request_simulator(int process_id, int* request_vector){

	printf("Resource vector requested is: ");
	int i;

	for (i = 0; i < num_resources; i++) printf("%i ", request_vector[i]);
	printf("\n");

	printf("Checking if allocation is safe using banker's algorithm\n");
	int result = bankers_algorithm(process_id, request_vector);

	if (result == -1) {
		printf("Error, impossible request\n");
	}

	return 0;

}
/*
Simulates processes running on the system.

*/
void* process_simulator(void* pr_id){

	int process_id = *(int *)&pr_id;
	int i,j;
	printf("Process simulator %i thread created\n", process_id);

	int *request = malloc(num_resources * sizeof(int));

	while (1) {

		for (j = 0; j < num_resources; j++) {
			if (need[process_id][j] == 0) request[j] = 0;
			else request[j] = rand() % need[process_id][j] + 1;
			printf("Process %i requests %i of resource %i\n", process_id, request[j], j);
		}

		printf("Requesting resources for process %i\n", process_id);
		int result = request_simulator(process_id, request);

		if (result = 0) {
			printf("Request failed\n");
			break;
		} else {
			sleep(3);
		}

		int needs_satisfied = 1;
		for (j = 0; j < num_resources; j++) if (need[process_id][j] > 0) needs_satisfied = 0;
			if (needs_satisfied == 1) break;
	}	

	// Process is complete and may now relinquish resources
	pthread_mutex_lock(&lock);

	for (j = 0; j < num_resources; j++) {
		avail[j] += hold[process_id][j];
		hold[process_id][j] = 0;
	}

	pthread_mutex_unlock(&lock);

	printf("Process %i complete\n", process_id);
	printAll();

}


/*
Simulates a fault occuring on the system.

*/
void* fault_simulator(void* pr_id){

	printf("Fault simulator thread created\n");

	while (1) {

		sleep(10);

		int fault_roll = rand()%100; // random int from 0 to 99

		if (fault_roll < 50) { // Fault occurs at 50% chance

			int resource_roll = rand()%(num_resources + 1);
			pthread_mutex_lock(&lock);

			if (avail[resource_roll] > 0) {
				avail[resource_roll]--;
				 printf("Fault randomly occured, resource %i decreaed by 1\n", resource_roll);
			}
			
			pthread_mutex_unlock(&lock);

		}

	}


}

/*
Checks for deadlock
*/
void* deadlock_checker(){

	printf("Deadlock checker thread created\n");

	while (1) {

		sleep(10);
		printf("Checking for deadlock\n");

		int some_processes_ok = 0;

		for (int i = 0; i < num_processes; i ++) {
			int insufficient_resources = 0;
			for (int j = 0; j < num_resources; j ++) 
				if (need[i][j] > avail[j]) {
					insufficient_resources = 1;
					break;
				}
			if (insufficient_resources == 0) some_processes_ok = 1;

		}

		if (some_processes_ok == 0) {
			printf("Deadlock will occur as processes request more resources, exiting...\n\n");
			exit(1);
		}

	}

}

int main()
{
	int i,j;


    // Get inputs
    printf("\nEnter the number of processes: ");
    scanf("%d", &num_processes);
    pthread_t process_simulator_thread[num_processes], fault_simulator_thread, deadlock_checker_thread;

    printf("Enter the number of resources: ");
    scanf("%d", &num_resources);



    printf("\nEnter available resources\n");
    avail = (int *)  malloc(num_resources * sizeof(int));
    for (i = 0; i < num_resources; i++) {
    	printf("Amount of resource %i available: ", i);
    	scanf("%d", &avail[i]);
    }


    printf("\nEnter the maximum amount of resource each process can claim\n");
    //int max_zero_zero;
    need  = (int **) malloc(num_processes * sizeof(int *)); 
    max   = (int **) malloc(num_processes * sizeof(int *)); 
    for (i = 0; i < num_processes; i++) {
    	need[i] = (int *) malloc(num_resources * sizeof(int)); 
    	max[i] = (int *) malloc(num_resources * sizeof(int)); 
	    for (j = 0; j < num_resources; j++) {
	    	printf("Process %i, Resource %i: ",i, j);
	    	scanf("%d", &max[i][j]);
	    	need[i][j] = max[i][j];
	    	//if (i == 0 && j == 0) max_zero_zero = max[0][0];

    	}
    }

    // Initialize hold to 0
    hold  = (int **) malloc(num_processes * sizeof(int *));
    for (i = 0; i < num_processes; i++) {
    	hold[i] = (int *) malloc(num_resources * sizeof(int)); 
    	for (j = 0; j < num_resources; j++) {
    		hold[i][j] = 0; 
    	}
    } 
	
	printAll();

	printf("\nBEGIN SIMULATION\n");

	printAll();

	for (i = 0; i < num_processes; i++) { 
        int process_id = i;
		pthread_create(&process_simulator_thread[i], NULL, process_simulator, (void *)(intptr_t)process_id);
	}

	pthread_create(&fault_simulator_thread, NULL, fault_simulator, NULL);
	pthread_create(&deadlock_checker_thread, NULL, deadlock_checker, NULL);


    pthread_exit(NULL);
    printf("\nSIMULATION COMPLETE\n\n");
    return 0;


}











