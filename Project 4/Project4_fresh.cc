#include <queue>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define BANKHOURS 7
#define BANKHOURS_SIMULATION_TIME BANKHOURS * 60 * 0.1
#define SCALE_TO_NANOSECONDS 100000000

#define NUMBER_OF_TELLERS 3

//customer struct definition
typedef struct
{
	struct timespec qPushTime;		// The arrival time of the customer
	struct timespec qPopTime;		// The time it takes the customer to leave the queue'
	unsigned int serviceTime;
} customer;

// Queue to store customers
pthread_mutex_t queue_semaphore, previous_queue_semaphore;
std::queue<customer> customers, previousCustomers;

double teller_maximum_time_waiting = 0;
double teller_average_time_waiting = 0;

bool BankClosed = false;

//creates a customer
customer create_customer(struct timespec currentTime)
{
	customer temporaryCustomer;
	temporaryCustomer.qPushTime = currentTime;
	temporaryCustomer.serviceTime = (rand()%331) + 30;
	return temporaryCustomer;
}

//Converts simulated minutes to equivalent real timespec
timespec simulated_minutes_to_time(unsigned int minutes)
{
	struct timespec time;
	time.tv_sec = minutes/10;
	time.tv_nsec = minutes%10 * SCALE_TO_NANOSECONDS;
	return time;
}

//Converts simulated minutes to equivalent real timespec
timespec simulated_seconds_to_time(unsigned int seconds)
{
	struct timespec time;
	time.tv_sec = seconds/600;
	time.tv_nsec = ((seconds/60)%10) * SCALE_TO_NANOSECONDS;
	return time;
}

// calculates difference between two timespec in terms of seconds
long time_difference_to_simulated_seconds(struct timespec time1,struct timespec time2)
{
	long time;
	time = (time1.tv_sec - time2.tv_sec) * 600;
	if(time1.tv_nsec > time2.tv_nsec )
	{
		time += ((time1.tv_nsec - time2.tv_nsec) / SCALE_TO_NANOSECONDS) * 60;
	}
	else
	{
		time -= ((time2.tv_nsec - time1.tv_nsec) / SCALE_TO_NANOSECONDS) * 60;
	}
	return time;
}

//compares two timespec
bool isgreater(struct timespec time1,struct timespec time2)
{
	if(time1.tv_sec > time2.tv_sec)
	{
		return true;
	}
	else if(time1.tv_sec < time2.tv_sec)
	{
		return false;
	}
	else{
		if(time1.tv_nsec > time2.tv_nsec)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

// custom C function that call nanosleep and accepts simulated time
void my_sleep_seconds(int seconds)
{
	struct timespec timesleep,rem;
	timesleep = simulated_seconds_to_time(seconds);
	nanosleep(&timesleep,&rem);
}

//function for teller thread
void *eachTeller(void*)
{
	bool iswaiting = false;
	double total_time_waiting = 0;
	double max_time_waiting = 0;
	unsigned int customers_serviced = 0;
	timespec waiting_since;
	timespec lastbreak;

	//decide first break
	long breakAfter = (rand() % 31) + 30;
	clock_gettime(CLOCK_REALTIME,&lastbreak);

	while(1)
	{
		struct timespec current_time;
		clock_gettime(CLOCK_REALTIME,&current_time);

		//check if time for break
		if(time_difference_to_simulated_seconds(current_time,lastbreak) > breakAfter * 60)
		{
			//std::cout << "It's been " << time_difference_to_simulated_seconds(current_time,lastbreak) << "since last break, taking break" << std::endl;
			breakAfter = (rand() % 31) + 30;
			lastbreak = current_time;
			my_sleep_seconds(rand() % 4 + 1);
		}

		customer current_customer;
		bool hasCustomer;

		//get next customer if there is any
		pthread_mutex_lock(&queue_semaphore);
		if(customers.empty())
		{
			hasCustomer = false;
		}
		else
		{
			current_customer = customers.front();
			customers.pop();
			hasCustomer = true;
		}
		pthread_mutex_unlock(&queue_semaphore);

		//if there is customer, service customer
		if(true == hasCustomer)
		{
			//check if teller was waiting, if yes, count and save wait time
			if(true == iswaiting)
			{
				long wait_time = time_difference_to_simulated_seconds(current_time,waiting_since);
				total_time_waiting += wait_time;
				if(wait_time > max_time_waiting)
				{
					max_time_waiting = wait_time;
				}
				//std::cout << "teller was waiting from " << waiting_since.tv_sec <<"  " <<waiting_since.tv_nsec << "  " << current_time.tv_sec  <<"  " << current_time.tv_nsec << "  "<< wait_time << std::endl;
				iswaiting = false;
			}
			current_customer.qPopTime = current_time;
			//
			//clock_gettime(CLOCK_REALTIME,&current_customer.qPopTime);

			my_sleep_seconds(current_customer.serviceTime);

			// put serviced customer info in another queue
			pthread_mutex_lock(&previous_queue_semaphore);
			previousCustomers.push(current_customer);
			pthread_mutex_unlock(&previous_queue_semaphore);

			customers_serviced++;
		}
		else
		{
			//no customer,
			if(true == BankClosed)
			{
				//prepare exit
				if(max_time_waiting > teller_maximum_time_waiting)
				{
					teller_maximum_time_waiting = max_time_waiting;
				}
				teller_average_time_waiting += (total_time_waiting/customers_serviced)/NUMBER_OF_TELLERS;
				void* retValue = 0;
				pthread_exit(retValue);
			}
			else
			{
				//wait if bank is not closed
				if(false == iswaiting)
				{
					waiting_since = current_time;
					iswaiting = true;
				}
			}
		}
	}
}

// simulation of bank and generating customers
int main(int argc, char *argv[]) {

	struct timespec current_time;
	struct timespec closing_time;
	struct timespec rem;
	struct timespec timesleep;
	unsigned int wait_minute;
	unsigned int total_customers = 0;
	unsigned int max_queue_size = 0;

	pthread_t threads[NUMBER_OF_TELLERS];
	void* results[NUMBER_OF_TELLERS];

	timesleep.tv_sec = 1;

	clock_gettime(CLOCK_REALTIME,&current_time);

	// decide closing time
	closing_time.tv_sec = current_time.tv_sec + BANKHOURS_SIMULATION_TIME;

	//std::cout << closing_time.tv_sec << std::endl;
	//std::cout << current_time.tv_sec << std::endl;
	//std::cout << "Started" << std::endl;

	pthread_mutex_init(&queue_semaphore, NULL);
	pthread_mutex_init(&previous_queue_semaphore, NULL);

	// create the three threads corresponding to each teller
	for(int i=0;i<NUMBER_OF_TELLERS;i++)
	{
		pthread_create(&threads[i], NULL, &eachTeller, NULL);
	}

	// create new customers till bank is open
	while(closing_time.tv_sec > current_time.tv_sec)
	{
		//get current time to check if its time to close bank
		clock_gettime(CLOCK_REALTIME,&current_time);

		//create customer add in queue
		customer new_customer = create_customer(current_time);

		//put customer in queue
		pthread_mutex_lock(&queue_semaphore);
		customers.push(new_customer);
		if(max_queue_size < customers.size() )
		{
			max_queue_size = customers.size();
		}
		pthread_mutex_unlock(&queue_semaphore);

		//sleep till it's time to create next customer
		wait_minute = (rand()%4) + 1;
		timesleep = simulated_minutes_to_time(wait_minute);

		//std::cout << "sleeping for " << timesleep.tv_sec  << std::endl;
		nanosleep(&timesleep,&rem);

		total_customers++;
		//std::cout << current_time.tv_sec << std::endl;
	}
	//std::cout << "Ended" << total_customers << std::endl;

	//bank is closed
	BankClosed = true;

	//wait till all tellers service customers inside bank
	while(customers.size() > 0)
	{
		//wait for 10 simulated minutes (1 second in real)
		timesleep.tv_sec = 1;
		timesleep.tv_nsec = 0;
		nanosleep(&timesleep,&rem);
	}

	//wait till/check if all threads have finished execution
	for(int i=0;i<NUMBER_OF_TELLERS;i++)
	{
		pthread_join(threads[i],&results[i]);
	}

	// destroy
	pthread_mutex_destroy(&queue_semaphore);
	pthread_mutex_destroy(&previous_queue_semaphore);

	double total_wait_queue = 0;
	double max_wait_queue = 0;
	unsigned int total_transaction_time = 0;
	unsigned int max_transaction_time = 0;

	//std::cout << "Previous customer size " << previousCustomers.size()<< std::endl;

	// get information saved in another queue
	while(previousCustomers.size() > 0)
	{
		customer temp_customer = previousCustomers.front();
		previousCustomers.pop();

		long queue_wait = time_difference_to_simulated_seconds(temp_customer.qPopTime,temp_customer.qPushTime);
		total_wait_queue += queue_wait;

		if(queue_wait > max_wait_queue)
		{
			max_wait_queue = queue_wait;
		}

		total_transaction_time += temp_customer.serviceTime;

		if(max_transaction_time < temp_customer.serviceTime )
		{
			max_transaction_time = temp_customer.serviceTime;
		}
		//std::cout << "wait " << queue_wait << " service time " << temp_customer.serviceTime << std::endl;

	}

	// print information
	std::cout << "The total number of customers serviced are " << total_customers << std::endl;
	std::cout << "The average customer wait is " << total_wait_queue/total_customers << " Seconds" << std::endl;
	std::cout << "The average time spent with teller is " << total_transaction_time/total_customers << " Seconds" << std::endl;
	std::cout << "The average wait that tellers do is " << teller_average_time_waiting << " Seconds" << std::endl;
	std::cout << "The maximum customer waiting time in queue is " << max_wait_queue << " Seconds" << std::endl;
	std::cout << "The maximum teller waiting time is " << teller_maximum_time_waiting << " Seconds" << std::endl;
	std::cout << "The maximum transaction time for tellers is " << max_transaction_time << " Seconds" << std::endl;
	std::cout << "The maximum depth of the queue is " << max_queue_size << std::endl;

	return EXIT_SUCCESS;
}
