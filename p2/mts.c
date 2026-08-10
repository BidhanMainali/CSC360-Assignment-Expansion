#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

typedef struct Train {
	int load_time;
	int cross_time;
	int priority; // Priority Denoted HIGH and LOW priortiy
	int trainId;  //specific Train ID
	int direction; // Deoted EAST or WEST (Ignoring uppercase as priority is handled above.
	int may_cross; // Condition vairable where it is One if safe to cross and Zero if it is currently not safe.
	pthread_cond_t cond;   // The trains private conditon variable
	pthread_t thread;  //Creates thread
	struct Train *next;





}Train;

enum Direction {EAST,WEST};
enum Priority{LOW,HIGH};


pthread_mutex_t threadmutex;

pthread_cond_t dispatcher_cond;

int last_direction;

int same_dir_count;

int crossed;

int track_busy;  // Zero or One if busy or not busy respectivly.

Train *queues[4];

struct timespec start_time;

FILE *out;










Train *parse_input(const char *filename, int *out_n){

	  FILE* fptr;
   	  int count = 0; //for First loop iteration
	  char c;
	  int load;
	  int cross;
	  int i = 0; //For second loop iteration
          fptr = fopen(filename,"r");

          if (fptr == NULL){
                  perror("file open");
                  exit(EXIT_FAILURE);
	  }

	  while (fscanf(fptr, " %c %d %d", &c, &load, &cross) == 3){  //Count how many trains are there so we can allocate correct memory.
		  count++;
	   
	  }

	  Train * trains = malloc(count * sizeof(Train));

	  if (trains == NULL){
	  	perror("malloc");
		exit(EXIT_FAILURE);
	  
	  
	  }

	  rewind(fptr);


	  while (fscanf(fptr, " %c %d %d", &c, &load, &cross) == 3){
	  	trains[i].load_time = load;
		trains[i].cross_time = cross;
		trains[i].trainId = i;  //Current slot intex
		trains[i].may_cross = 0; // For now is zero as not granted yet
		trains[i].next = NULL; // Currently not in queue
	  	

		if (isupper(c)){
			trains[i].priority = HIGH;

	

		
		
		}else {
			trains[i].priority = LOW;
		
		
		}


		if (tolower(c) ==  'e'){
		
			trains[i].direction = EAST;
		
		}else {
		
			trains[i].direction = WEST;
		
		
		}
	  
	  	i++;
	  }


	  fclose(fptr);
	  *out_n = count;
	  return trains;




}


void time_str( char *buf){


	
        struct timespec now;


        clock_gettime(CLOCK_MONOTONIC, &now);

        long elapsed_ns = (now.tv_sec - start_time.tv_sec) * 1000000000L + (now.tv_nsec - start_time.tv_nsec);
	int elapsed_time = elapsed_ns / 100000000L;


	int tenths = elapsed_time % 10;
 	int whole_seconds = elapsed_time / 10;
 	int seconds = whole_seconds % 60;
 	int minutes = (whole_seconds / 60) % 60;
 	int hours = whole_seconds / 3600;

        sprintf(buf, "%02d:%02d:%02d.%d", hours, minutes, seconds, tenths);





}



const char *dir_str(int dir){ // Helper function because we will reuse this logic again for formating and 
				// comparison.

	if (dir == EAST){
	
	return "East";
	
	}else {


		return "West";
	
	}



}







void enqueue(Train *t){

	int index = t->direction *2 + t->priority;  // train's slot index (N)
	
	t-> next = NULL;

	if (queues[index] == NULL){  //if has no trains
		queues[index] = t;	
	
	
	} else{
	
		Train *cur = queues[index];
	
		while( cur->next != NULL){
			cur = cur->next;
		
		
		
		}
		cur->next = t;
	}


	
	
	
	
	



}


void *train_func(void *arg){

	Train *train_ptr;

	train_ptr = (Train *) arg;	//cast it to train struct
	


	usleep(train_ptr->load_time *100000);	//Load the train
	

	pthread_mutex_lock(&threadmutex);
	char ts[16];
	// Train is read to go on the track
	time_str(ts);
	fprintf(out, "%s Train %2d is ready to go %4s\n", ts, train_ptr->trainId, dir_str(train_ptr->direction));
	enqueue(train_ptr);
	pthread_cond_signal(&dispatcher_cond);	
	//Wait for signal to be awake when dispathcer thread awakes the thread
	while( train_ptr -> may_cross ==0){
	pthread_cond_wait(&train_ptr -> cond, &threadmutex);
	
	}

	// Train is currently on the track
	char ts2[16]; time_str(ts2);
	fprintf(out, "%s Train %2d is ON the main track going %4s\n", ts2, train_ptr->trainId, dir_str(train_ptr->direction));

	// Train crosses
	pthread_mutex_unlock(&threadmutex);
	usleep(train_ptr ->cross_time * 100000);

	pthread_mutex_lock(&threadmutex);
	time_str(ts);
	// Outputs and prepares for next train
	fprintf(out, "%s Train %2d is OFF the main track after going %4s\n", ts, train_ptr->trainId, dir_str(train_ptr->direction));
	track_busy = 0;
	crossed++;
	pthread_cond_signal(&dispatcher_cond);
	pthread_mutex_unlock(&threadmutex);

	return NULL;





}




Train *best_in_direction(int dir){ //helper Funciton

	int high = dir *2 + HIGH;
	int low = dir *2 + LOW;	

	if(queues[high] != NULL){
		return queues[high];
	
	
	
	}

	if(queues[low] != NULL){
	
		return queues[low];
	
	
	}


	return NULL;




}



Train *select_next(void){

	Train *east_best = best_in_direction(EAST);
	Train *west_best = best_in_direction(WEST);

	// Trivial Cases
	if(east_best == NULL && west_best == NULL){ //Nobody is waiting
	
		return NULL;
	
	
	}

	if(east_best == NULL){  // Only west waiting
		return west_best;
	
	
	}

	if(west_best == NULL){  // Only east waiting
	
		return east_best;
	
	}


	
	// Anti-starvation
	if(same_dir_count >= 2){

	
		if(last_direction == EAST) return west_best;
		if(last_direction == WEST) return east_best;
	
	
	}


	// Priority
	
	if(east_best->priority != west_best->priority){
		if(east_best->priority > west_best->priority) return east_best;
		return west_best;
	
	
	}

	if(last_direction == EAST) return west_best;
	if(last_direction == WEST) return east_best;
	return west_best;



}



void dequeue(Train *t){

	int index = t->direction * 2 + t->priority;
	queues[index] = t->next;
	t->next = NULL;




}


















int main(int argc, char* argv[]){
	int n;
	int i;
	Train* train;
	if (argc != 2){  // If user inputs a argument that is not valid
		fprintf(stderr,"%s: no proper input was given:\n" ,argv[0]);
		exit(EXIT_FAILURE);
	}


	// Initiilze all variables

	train = parse_input(argv[1], &n);

	
	pthread_mutex_init(&threadmutex,NULL);

	pthread_cond_init(&dispatcher_cond,NULL);

	
	for(i = 0; i<4; i++){
		queues[i] = NULL;
	
	
	}

	last_direction = -1;

	same_dir_count = 0;
	
	crossed = 0;

	track_busy = 0;

        out = fopen("output.txt", "w");

        if(out == NULL){
                perror("outout error");
                exit(EXIT_FAILURE);

        }


	clock_gettime(CLOCK_MONOTONIC, &start_time);
	

	for (i = 0; i < n; i++) {

    		pthread_cond_init(&train[i].cond, NULL);
    		pthread_create(&train[i].thread, NULL, train_func, &train[i]);
	}


	pthread_mutex_lock(&threadmutex);

	while (crossed <n){
		// Decided who goes

		Train *next = NULL;
		if(!track_busy){
			next = select_next();
		
		
		}


		//Grant permission to go
		
		if(next != NULL){

			dequeue(next);
			track_busy = 1;

			if(next->direction == last_direction){
				same_dir_count++;
			
			} else {

				same_dir_count = 1;
				last_direction = next->direction;
			
			
			
			
			}

			next->may_cross= 1;
			pthread_cond_signal(&next->cond);



		
		
		}else{
			// Otherwise Wait
			pthread_cond_wait(&dispatcher_cond,&threadmutex);
		
		
		
		
		}

	
	
	}

	pthread_mutex_unlock(&threadmutex);
	
	for(i = 0; i< n; i++){
		pthread_join(train[i].thread, NULL); // cleanup, wait until all train threads are complete before procceding
		
		
		
	
	
	
	}
	//free memory
	fclose(out);
	free(train);
	return 0;





}
