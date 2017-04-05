#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include <semaphore.h>
#include <curses.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/resource.h>
#include <wait.h>
#include <unistd.h>
#include <errno.h>


struct msg{
	int sleepingTime;
	int threadID;
};

pthread_t carThreadp[200];
pthread_t truckThreadp[200];
pthread_t captainThreadp;
pthread_t makeMoreVehiclesThreadp;
pthread_t makeMoreTrucksThreadp;

pthread_mutex_t protectCarThreadCounter;
int carThreadCounter = 0;
pthread_mutex_t protectTruckThreadCounter;
int truckThreadCounter = 0;
pthread_mutex_t protectCarsInGroupCounter;
int carsInGroupCounter = 0;
pthread_mutex_t protectTrucksInGroupCounter;
int trucksInGroupCounter = 0;
pthread_mutex_t protectCarsWaitingCounter;
int carsWaitingCounter = 0;
pthread_mutex_t protectCarsWaitingCounter2;
int carsWaitingCounter2 = 0;
pthread_mutex_t protectTrucksWaitingCounter;
int trucksWaitingCounter = 0;
pthread_mutex_t protectTrucksWaitingCounter2;
int trucksWaitingCounter2 = 0;
pthread_mutex_t protectCarsLoadedCounter;
int carloaded = 0;
pthread_mutex_t protectCarsunLoadedCounter;
int carunloaded = 0;
pthread_mutex_t protectTrucksLoadedCounter;
int truckloaded = 0;
pthread_mutex_t protectTrucksunLoadedCounter;
int truckunloaded = 0;

int terminationFlag = 0;
double maxCarIntervalUsec = 0.0;
double maxTruckIntervalUsec = 0.0;
double truckProb = 0.0;

#define MAX_CARS = 6;
#define MAX_TRUCKS = 2;

int terminateFlag = 0;
void initialize();
void *captain();
void *car(void *inputStruct);
void *truck(void *inputStruct);
void *makeMoreVehicles();

int sem_waitChecked(sem_t *semaphoreID);
int sem_postChecked(sem_t *semaphoreID);
int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value);
int sem_destroyChecked(sem_t *semaphoreID);
int pthread_mutex_lockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_initChecked(pthread_mutex_t *mutexID,const pthread_mutexattr_t *attrib);
int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID);
void terminateSimulation();
int lineFlag = 0;
int carloaded;
int carunloaded;
int truckloaded;
int truckunloaded;
int unload;
int loading;

int main()
{
        int newSeed = 0;
	double maxCarInterval = 0.0;
	double maxTruckInterval = 0.0;
	double truckProb2 = 0.0;

	initialize();
	//Declare initial values
	printf("Please enter a random seed\n");
	scanf("%d",&newSeed);
	srand(newSeed);
	printf("Please enter the maximum interval length for vehicles(ms)\n");
	scanf("%lf",&maxCarInterval);
	maxCarIntervalUsec = (int)maxCarInterval * 1000;
	maxTruckIntervalUsec = maxCarIntervalUsec;
	printf("Please enter the probability of a truck(percentage)\n");
	scanf("%lf",&truckProb2);
	truckProb = truckProb2;
	printf("max car interval time %f \n", maxCarInterval);
	printf("truck probability %f \n", truckProb);
	//Create the threads for Vehicles and captain
	pthread_create(&makeMoreVehiclesThreadp,NULL,makeMoreVehicles, NULL);
	pthread_create(&captainThreadp,NULL,captain,NULL);
	//End the simulation
	pthread_join(captainThreadp,NULL);
	printf("captain joined \n");
	pthread_join(makeMoreVehiclesThreadp,NULL);
	printf("cars joined \n");
	terminateSimulation();
	printf("Terminated final%f \n", maxCarInterval);
	exit(0);
}

void *makeMoreVehicles()
{
	int carsCreated = 0;
	int trucksCreated = 0;
	int nextInterval = 0.0;
	int threadID;
	struct msg *inCarp;
	struct msg inCar;

	struct msg *inTruckp;
	struct msg inTruck;

	inCarp = &inCar;
	inCarp->threadID = 3;

	inTruckp = &inTruck;
	inTruckp->threadID = 3;
	//While we need to make more vehicles
	while(inCarp->threadID < 200 && terminationFlag == 0 && inTruckp->threadID < 200)
        {
		usleep(500);
		if(rand()%100 >= truckProb)
                {
			nextInterval = (int)(((double)rand() / RAND_MAX)*maxCarIntervalUsec);
			inCarp->sleepingTime = (int)(((double)rand()/RAND_MAX)*maxCarIntervalUsec);
			threadID = inCarp->threadID;
			inCarp->threadID++;
			if(pthread_create((&carThreadp[carsCreated]),NULL,car,(void*)inCarp)==0)
                        {
				carsCreated++;
				printf("CCC %d CCC NEW CAR CREATED %d\n",threadID,carsCreated);
			}
			else
                        {
				printf("car process not created \n");
				continue;
			}
			usleep(nextInterval);
		}
                else
                {
			nextInterval = (int)(((double)rand() / RAND_MAX)*maxTruckIntervalUsec);
			inTruckp->sleepingTime = (int)(((double)rand()/RAND_MAX)*maxTruckIntervalUsec);
			threadID = inTruckp->threadID;
			inTruckp->threadID++;
			if(pthread_create((&truckThreadp[trucksCreated]),NULL,truck,(void*)inTruckp)==0)
                        {
				trucksCreated++;
				printf("TTT %d TTT NEW TRUCK CREATED %d\n",threadID,trucksCreated);
			}
			else
                        {
				printf("truck process not created \n");
				continue;
			}
			usleep(nextInterval);
		}
	}
	terminateSimulation();
	printf("GOODBYE from thread makeMoreVehicles\n");
	fflush(stdout);
	exit(0);

}

void *captain()
{
	int cars = 0;
	int trucks = 0;
	int prevCars = 9999;
	int prevTrucks = 9999;
	int i;
	int counter = 0;
	unload = 0;
	loading = 1;
	int counter1 = 0;
	int repeated = 0;
	while(terminationFlag == 0)
        {
	//Check which line
		if(lineFlag == 0)
                {
			pthread_mutex_lockChecked(&protectCarsWaitingCounter);
			if(carsWaitingCounter >= 1)
                        {
				cars++;
				printf("CAPTAIN - number of cars in the waiting line: %d\n",cars);
				printf("CAPTAIN - number of trucks in the waiting line: %d\n",trucks);
				carsWaitingCounter--;
			}
			pthread_mutex_unlockChecked(&protectCarsWaitingCounter);
			pthread_mutex_lockChecked(&protectTrucksWaitingCounter);
			if(trucksWaitingCounter >= 1)
                        {
				trucks++;
				printf("CAPTAIN - number of cars in the waiting line: %d\n",cars);
				printf("CAPTAIN - number of trucks in the waiting line: %d\n",trucks);
				trucksWaitingCounter--;
			}
			pthread_mutex_unlockChecked(&protectTrucksWaitingCounter);
		}
		else
                {
			pthread_mutex_lockChecked(&protectCarsWaitingCounter2);
			if(carsWaitingCounter2 >= 1)
                        {
				cars++;
				printf("CAPTAIN - number of cars in the waiting line: %d\n",cars);
				printf("CAPTAIN - number of trucks in the waiting line: %d\n",trucks);
				carsWaitingCounter2--;
			}
			pthread_mutex_unlockChecked(&protectCarsWaitingCounter2);
			pthread_mutex_lockChecked(&protectTrucksWaitingCounter2);
			if(trucksWaitingCounter2 >= 1)
                        {
				trucks++;
				printf("CAPTAIN - number of cars in the waiting line: %d\n",cars);
				printf("CAPTAIN - number of trucks in the waiting line: %d\n",trucks);
				trucksWaitingCounter2--;
			}
			pthread_mutex_unlockChecked(&protectTrucksWaitingCounter2);
		}
			//Time it takes for the ferry arrive
			if(counter1 == 0 && loading == 0)
                        {
				printf("CAPTAIN - FERRY ARRIVES AT THE DOCK \n");
				counter1++;
				loading = 1;

			}
                        else
                        {
				counter1++;
				if(counter1 >=10)
                                {
					counter1 = counter1 - 10;
				}
			}
			//if ready to load
			if(loading == 1)
                        {
				if((trucks >= 2 && cars >=2) || (trucks == 1 && cars >= 4) || (trucks ==0 && cars >=6))
                                {
					printf("CAPTAIN - READY TO LOAD \n");
					unload = 0;
					if(lineFlag == 0)
                                        {
						printf("CAPTAIN - switch to lane 1 \n");
						lineFlag = 1;
					}
                                        else if(lineFlag == 1)
                                        {
						printf("CAPTAIN - switch to lane 0 \n");
						lineFlag = 0;
					}

					if(trucks >= 2 && cars >=2)
                                        {
						//2 cars
						carloaded = 0;

						while(carloaded <= 1)
                                                {
						}

						carloaded = 0;
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						//2 trucks
						truckloaded = 0;

						while(truckloaded <=1)
                                                {
						}

						truckloaded = 0;
						printf("CAPTAIN - TRUCK loaded \n");
						printf("CAPTAIN - TRUCK loaded \n");

						cars = cars - 2;
						trucks = trucks - 2;
						counter++;
						printf("CAPTAIN - DONE A LOAD %d\n",counter);
						loading = 0;
						printf("CAPTAIN - SAILING \n");
						printf("CAPTAIN - FERRY IS READY TO UNLOAD \n");
						unload = 1;
						carunloaded = 0;

						while(carunloaded <=1)
                                                {
						}
						carunloaded = 0;
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");

						while(truckunloaded <=1)
                                                {
						}

						truckunloaded -=2;
						printf("CAPTAIN - TRUCK unloaded \n");
						printf("CAPTAIN - TRUCK unloaded \n");

						unload = 0;
						printf("CAPTAIN - FERRY RETURNS TO LOADING DOCK \n");
					}
                                        else if(trucks == 1 && cars >=4)
                                        {
						carloaded = 0;

						while(carloaded <= 3)
                                                {
						}

						carloaded = 0;
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						truckloaded = 0;

						while(truckloaded <=0)
                                                {
						}

						truckloaded = 0;
						printf("CAPTAIN - TRUCK loaded \n");
						cars = cars - 4;
						trucks = trucks - 1;
						counter++;
						printf("CAPTAIN - DONE A LOAD %d\n",counter);
						loading = 0;
						printf("CAPTAIN - SAILING \n");
						printf("CAPTAIN - FERRY IS READY TO UNLOAD b \n");
						unload = 1;
						carunloaded = 0;

						while(carunloaded <=3)
                                                {
						}

						carunloaded = 0;
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");

						while(truckunloaded <=0)
                                                {
						}

						truckunloaded--;
						printf("CAPTAIN - TRUCK unloaded \n");

						unload = 0;
						printf("CAPTAIN - FERRY RETURNS TO LOADING DOCK \n");
					}
                                        else if(trucks == 0 && cars >=6)
                                        {
						carloaded = 0;

						while(carloaded <= 5)
                                                {
						}

						carloaded = 0;
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");
						printf("CAPTAIN - CAR loaded \n");

						cars = cars - 6;
						counter++;
						printf("CAPTAIN - DONE A LOAD %d\n",counter);
						loading = 0;
						printf("CAPTAIN - SAILING \n");
						printf("CAPTAIN - FERRY IS READY TO UNLOAD \n");
						unload = 1;
						carunloaded = 0;

						while(carunloaded <=5)
                                                {
						}

						carunloaded = 0;
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");
						printf("CAPTAIN - CAR unloaded \n");

						unload = 0;
						printf("CAPTAIN - FERRY RETURNS TO LOADING DOCK \n");
					}
                                        else
                                        {
						if(lineFlag == 0)
                                                {
							printf("CAPTAIN - switch to lane 1\n");
							lineFlag = 1;
						}
                                                else if(lineFlag == 1)
                                                {
							printf("CAPTAIN - switch to lane 0\n");
							lineFlag = 0;
						}
					}
				}
				if (counter == 11)
                                {
					printf("The ferry has finished 11 loads\n");
					printf("End!\n");
					terminationFlag = 1;
				}
			}
	}
	return 0;
}
//Initialize mutexes
void initialize()
{
	pthread_mutex_initChecked(&protectCarThreadCounter, NULL);
	pthread_mutex_initChecked(&protectTruckThreadCounter, NULL);
	pthread_mutex_initChecked(&protectCarsInGroupCounter, NULL);
	pthread_mutex_initChecked(&protectTrucksInGroupCounter, NULL);
	pthread_mutex_initChecked(&protectCarsWaitingCounter, NULL);
	pthread_mutex_initChecked(&protectTrucksWaitingCounter, NULL);
	pthread_mutex_initChecked(&protectCarsWaitingCounter2, NULL);
	pthread_mutex_initChecked(&protectTrucksWaitingCounter2, NULL);
	pthread_mutex_initChecked(&protectCarsLoadedCounter, NULL);
	pthread_mutex_initChecked(&protectTrucksLoadedCounter, NULL);
	pthread_mutex_initChecked(&protectCarsunLoadedCounter, NULL);
	pthread_mutex_initChecked(&protectTrucksunLoadedCounter, NULL);
	printf("mutexes initiialized\n");
}

void *car(void *inputStruct)
{
	int threadId;
	int startTimeN;
	int k;
	int prevCars = 999999;

	threadId = ((struct msg *)inputStruct)->threadID;
	startTimeN = ((struct msg *)inputStruct)->sleepingTime;
	printf("CCC %d CCC the car arrives \n",threadId);

	if(startTimeN > 0)
        {
		usleep(startTimeN);
	}
	printf("CCC %d CCC car drives for %f ms\n",threadId,startTimeN/1000.0);
	//Count the cars in each lane
	if(lineFlag == 0)
        {
		pthread_mutex_lockChecked(&protectCarsWaitingCounter);
		carsWaitingCounter++;
		pthread_mutex_unlockChecked(&protectCarsWaitingCounter);
	}
        else
        {
		pthread_mutex_lockChecked(&protectCarsWaitingCounter2);
		carsWaitingCounter2++;
		pthread_mutex_unlockChecked(&protectCarsWaitingCounter2);
	}

	printf("CCC %d CCC car enters line %d\n",threadId,lineFlag);

	while(terminationFlag == 0)
        {
		//ready to load
		while(loading != 1)
                {
		}
		//If the car is loaded
		pthread_mutex_lockChecked(&protectCarsLoadedCounter);
		carloaded++;
		pthread_mutex_unlockChecked(&protectCarsLoadedCounter);

		//ready to unload
		while(unload != 1)
                {
		}
		//i have unloaded

		pthread_mutex_lockChecked(&protectCarsunLoadedCounter);
		carunloaded++;
		pthread_mutex_unlockChecked(&protectCarsunLoadedCounter);
		pthread_exit(NULL);
	}
}

void *truck(void *inputStruct)
{
	int threadId;
	int startTimeN;
	int k;
	int prevTrucks = 999999;

	threadId = ((struct msg *)inputStruct)->threadID;
	startTimeN = ((struct msg *)inputStruct)->sleepingTime;
	printf("TTT %d TTT the truck arrives \n",threadId);
	if(startTimeN > 0)
        {
		usleep(startTimeN);
	}
	printf("TTT %d TTT truck drives for %f ms\n",threadId,startTimeN/1000.0);
	//Check each lane
	if(lineFlag == 0)
        {
		pthread_mutex_lockChecked(&protectTrucksWaitingCounter);
		trucksWaitingCounter++;
		pthread_mutex_unlockChecked(&protectTrucksWaitingCounter);
	}
        else
        {
		pthread_mutex_lockChecked(&protectTrucksWaitingCounter2);
		trucksWaitingCounter2++;
		pthread_mutex_unlockChecked(&protectTrucksWaitingCounter2);
	}

	printf("TTT %d TTT truck enters line %d\n",threadId,lineFlag);

	while(terminationFlag == 0)
        {
		while(loading != 1)
                {
		}
		//Ready to load
		pthread_mutex_lockChecked(&protectTrucksLoadedCounter);
		truckloaded++;

		pthread_mutex_unlockChecked(&protectTrucksLoadedCounter);

		//ready to unload
		while(unload != 1)
                {
		}
		//i have unloaded

		pthread_mutex_lockChecked(&protectTrucksunLoadedCounter);
		truckunloaded++;
		pthread_mutex_unlockChecked(&protectTrucksunLoadedCounter);
		pthread_exit(NULL);
	}

}

void terminateSimulation()
{
	//Terminate
	pthread_mutex_destroyChecked(&protectCarThreadCounter);
	pthread_mutex_destroyChecked(&protectTruckThreadCounter);
	pthread_mutex_destroyChecked(&protectCarsInGroupCounter);
	pthread_mutex_destroyChecked(&protectTrucksInGroupCounter);
	pthread_mutex_destroyChecked(&protectCarsWaitingCounter);
	pthread_mutex_destroyChecked(&protectTrucksWaitingCounter);
	pthread_mutex_destroyChecked(&protectCarsWaitingCounter2);
	pthread_mutex_destroyChecked(&protectTrucksWaitingCounter2);

	pthread_mutex_destroyChecked(&protectCarsLoadedCounter);
	pthread_mutex_destroyChecked(&protectTrucksLoadedCounter);
	pthread_mutex_destroyChecked(&protectCarsunLoadedCounter);
	pthread_mutex_destroyChecked(&protectTrucksunLoadedCounter);

	printf("XXXXTERMINATETERMINATE   mutexes destroyed\n");

	printf("GOODBYE from terminate \n");
}

int sem_waitChecked(sem_t *semaphoreID)
{
	int returnValue;
        returnValue = sem_wait(semaphoreID);
        if (returnValue == -1 )
        {
            printf("semaphore wait failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
 	}
        return returnValue;
}

int sem_postChecked(sem_t *semaphoreID)
{
	int returnValue;
        returnValue = sem_post(semaphoreID);
        if (returnValue < 0 )
        {
            printf("semaphore post operation failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value)
{
        int returnValue;
        returnValue = sem_init(semaphoreID, pshared, value);
        if (returnValue < 0 )
        {
            printf("semaphore init operation failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int sem_destroyChecked(sem_t *semaphoreID)
{
        int returnValue;
        returnValue = sem_destroy(semaphoreID);
        if (returnValue < 0 )
        {
            printf("semaphore destroy operation failed: terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_lockChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
        returnValue = pthread_mutex_lock(mutexID);
        if (returnValue < 0 )
        {
            printf("pthread mutex lock operation failed: terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
        returnValue = pthread_mutex_unlock(mutexID);
        if (returnValue < 0 )
        {
            printf("pthread mutex unlock operation failed: terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
	                              const pthread_mutexattr_t *attrib)
{
	int returnValue;
        returnValue = pthread_mutex_init(mutexID, attrib);
        if (returnValue < 0 )
        {
            printf("pthread init operation failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
        returnValue = pthread_mutex_destroy(mutexID);
        if (returnValue < 0 )
        {
            printf("pthread destroy failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}
