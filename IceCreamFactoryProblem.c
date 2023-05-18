#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/kernel.h>
#include <time.h>
// prices of flavours and toppings

#define priceFlav_1 1.05
#define priceFlav_2 2.00
#define priceFlav_3 1.67
#define priceTopp_1 0.8
#define priceTopp_2 0.5

// resources
int ticket = 30, flavour[3] = {29, 34, 18}, topping[2] = {20, 34}; //total quantity
double revenue = 0.0;

struct customer{
	int id;
	int numOfF;
	int numOfT;
};

// semaphores declaration
sem_t ticketC, flavourC, f1, f2, f3, toppingC, t1, t2, paymentC;

void *IceCreamShop(void *arg)
{
	struct customer c  = *(struct customer *)arg;
	int RaceCond = 0;
	double bill = 0.0;

	// ticket counter - ENTER
	sem_wait(&ticketC);
	
	if(ticket <= 0)
	{
		printf("\nCustomer#%d Status: Leaving Shop, because there are no more tickets\n", c.id);
		syscall(335,"Leaving Shop, because there are no more tickets",c.id);
		sleep(1);
		pthread_exit(NULL);
	}

	ticket--;
	printf("\nCustomer#%d Status: Got Ticket\n", c.id);
	syscall(335,"Got Ticket",c.id);

	sem_post(&ticketC);
	// ticket counter - EXIT
	
	sleep(2);
	
	// flavours counter - ENTER
	sem_wait(&flavourC);

	// Race Condition will occur here but it will be handled through thread Local Variable
	// REASON: when this condition is checked by 4th thread
	//           and then flavour[i] is decremented by any of first 2 threads
	//           a thread will proceed even though flavors have been finished.
	// WHY THIS CONDITION IS USED WHEN ITS WORK IS DONE BY LOCAL THREAD VARIABLE?
	// REASON: If this is not used then a thread will check each condition below
	//           which will be time consuming.
	if(flavour[0] <= 0 && flavour[1] <= 0 && flavour[2] <= 0)
	{
		printf("\nCustomer#%d Status: Leaving Shop because all the flavours are finished\n", c.id);
		syscall(335,"Leaving Shop because all the flavours are finished", c.id);
		sleep(1);
		pthread_exit(NULL);
	}

	else
	{
		// flavour1
		sem_wait(&f1);
		
		if(flavour[0] > 0 && c.numOfF > 0)
		{
			flavour[0]--;
			RaceCond++;
			c.numOfF--;
			bill = bill + priceFlav_1;
			printf("\nCustomer#%d Status: Got Flavour 1\n", c.id);
			syscall(335,"Got Flavour 1",c.id);
			sleep(1);
		}
		
		sem_post(&f1);
		
		// flavour2
		sem_wait(&f2);
		
		if(flavour[1] > 0 && c.numOfF > 0)
		{
			flavour[1]--;
			RaceCond++;
			c.numOfF--;
			bill = bill + priceFlav_2;
			printf("Customer#%d Status: Got Flavour 2\n", c.id);
			syscall(335,"Got Flavour 2",c.id);
			sleep(1);
		}
		
		sem_post(&f2);
		
		// flavour3
		sem_wait(&f3);
		
		if(flavour[2] > 0 && c.numOfF > 0)
		{
			flavour[2]--;
			RaceCond++;
			c.numOfF--;
			bill = bill + priceFlav_3;
			printf("Customer#%d Status: Got Flavour 3\n", c.id);
			syscall(335,"Got Flavour 3",c.id);
			sleep(1);
		}
		
		sem_post(&f3);
		
		// if any of the threads did not get any flavour, 
		// 'RaceCond' will remain 0, 
		// threads will exit
		if(RaceCond == 0)
		{
			printf("\nCustomer#%d Status: Leaving Shop because all the flavours are finished", c.id);
			syscall(335,"Leaving Shop because all the flavours are finished", c.id);
			sleep(1);
			pthread_exit(NULL);
		}
	}

	printf("\nCustomer#%d Status: Got Flavour(s). Leaving Flavour Counter\n", c.id);
	syscall(335,"Got Flavour(s). Leaving Flavour Counter",c.id);
	sleep(2);
	
	sem_post(&flavourC);
	// flavours counter - EXIT

	if(c.numOfT > 0)
	{
		// topping counter - ENTER
		sem_wait(&toppingC);

		// topping1
		sem_wait(&t1);
	
		if(topping[0] > 0 && c.numOfT > 0)
		{
			topping[0]--;
			c.numOfT--;
			bill = bill + priceTopp_1;
		}
	
		sem_post(&t1);
	
		// topping2
		sem_wait(&t2);
	
		if(topping[1] > 0 && c.numOfT > 0)
		{
			topping[1]--;
			c.numOfT--;
			bill = bill + priceTopp_2;
		}
	
		sem_post(&t2);
	
		printf("\nCustomer#%d Status: Leaving Topping Counter\n", c.id);
		syscall(335,"Leaving Topping Counter",c.id);

		sem_post(&toppingC);
		// toppings counter - EXIT
	}

	else
	{
		printf("\nCustomer#%d Status: Did not want any toppings\n", c.id);
		syscall(335,"Did not want any toppings",c.id);
	}

	sleep(2);
	
	// payments counter - ENTER
	sem_wait(&paymentC);
	
	revenue = revenue + bill;
	
	printf("\nCustomer#%d Status: Billed: $ %.3f\n", c.id, bill);
	char a[100];
	sprintf(a,"Billed: $ %.3f",bill);
	syscall(335,a,c.id);	

	sem_post(&paymentC);
	// payments counter - EXIT
	
	sleep(2);
	
	printf("\nCustomer#%d Status: Leaving Ice-Cream Shop\n\n", c.id);
	syscall(335,"Leaving Ice-Cream Shop",c.id);
	
	return NULL;
}

// main
int main()
{
	srand(time(NULL));
	int numOfC;

	printf("\nEnter Number Of Customers [1-%d]: ", ticket);
	scanf("%d", &numOfC);
	printf("\n\n");

	if(numOfC <= 0)
	{
		printf("\n\nInvalid Input!\n\n");
		return 0;
	}

	else if(numOfC > ticket)
	{
		printf("Max customer limit is 30!\n");
		printf("Setting number of customers to 30\n");
		numOfC = ticket;
	}

	struct customer c[numOfC];

	for(int i=0; i<numOfC; i++)
	{
		c[i].id = i+100;
		c[i].numOfF = rand()%3 + 1;
		c[i].numOfT = rand()%3;
	}

	// semaphore initialization - START

	sem_init(&ticketC, 0, 1);
	
	sem_init(&flavourC, 0, 3);
	sem_init(&f1, 0, 1);
	sem_init(&f2, 0, 1);
	sem_init(&f3, 0, 1);
	
	sem_init(&toppingC, 0, 2);
	sem_init(&t1, 0, 1);
	sem_init(&t2, 0, 1);
	
	sem_init(&paymentC, 0, 1);
	
	// semaphore initialization - END
	
	
	// multithreading region - START
	pthread_t customer[numOfC];
	
	for(int i=0; i<numOfC; i++)
	{
		if(pthread_create(&customer[i], 0, &IceCreamShop, (void*) &c[i]) != 0)
		{
			printf("Thread not created!");
			exit(1);
		}

	}
	
	for(int i=0; i<numOfC; i++)
	{
		if(pthread_join(customer[i], NULL) != 0)
		{
			printf("Thread not join!");
			exit(1);
		}
	}
	// multithreading region - END
	
	
	printf("\nBusiness Journal - At Closing:\n\n");
	printf("Number Of Customers: %d", numOfC);
	printf("\nRevenue Generated: $ %.3f", revenue);
	printf("\nTickets Remaining: %d\n\n", ticket);
	
	
	// semaphore destroying - START
	
	sem_destroy(&ticketC);
	
	sem_destroy(&flavourC);
	sem_destroy(&f1);
	sem_destroy(&f2);
	sem_destroy(&f3);
	
	sem_destroy(&toppingC);
	sem_destroy(&t1);
	sem_destroy(&t2);
	
	sem_destroy(&paymentC);
	
	// semaphore destroying - END
	
	return 0;
}
