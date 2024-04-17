#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

typedef struct
{
	int id;
	int time_to_wait;
	int number_of_tracks;
} thread_args;

//////////////////////
// HELPER FUNCTIONS
//////////////////////

void handle_error(char *error)
{
	perror(error);
	exit(EXIT_FAILURE);
}

int msleep(long msec)
{
	struct timespec ts;
	int res;

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	do
	{
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}

//////////////////////
// MAIN FUNCTIONS
//////////////////////

int
	*occupyingIDs,
	waiting,
	crossed;
sem_t
	semaphore,
	mutex_waiting,
	mutex_crossed,
	mutex_occupyingIDs;

void *train(void *args)
{
	sem_wait(&mutex_waiting);
	waiting++;
	sem_post(&mutex_waiting);

	// Wait to arrive at station:
	sem_wait(&semaphore);

	// Start of printing and various handling routine
	sem_wait(&mutex_waiting);
	waiting--;
	printf("Train arrived!\nNumber of waiting trains: %d\nTrains currently in station: [ ", waiting);
	sem_post(&mutex_waiting);

	sem_wait(&mutex_occupyingIDs);
	for (int i = 0; i < ((thread_args *)args)->number_of_tracks; i++)
	{
		if (occupyingIDs[i] == -1)
		{
			occupyingIDs[i] = ((thread_args *)args)->id;
			break;
		}
	}
	for (int i = 0; i < ((thread_args *)args)->number_of_tracks; i++)
	{
		if (occupyingIDs[i] != -1)
			printf("%d ", occupyingIDs[i]);
	}
	sem_post(&mutex_occupyingIDs);

	sem_wait(&mutex_crossed);
	printf("]\nNumber of trains that have crossed:%d\n", crossed);
	sem_post(&mutex_crossed);
	fflush(stdout);
	// End of printing and various handling routine

	msleep(((thread_args *)args)->time_to_wait);

	// Go out of station:
	sem_post(&semaphore);

	// Start of printing and various handling routine
	sem_wait(&mutex_waiting);
	printf("Train departed!\nNumber of waiting trains: %d\nTrains currently in station: [ ", waiting);
	sem_post(&mutex_waiting);

	sem_wait(&mutex_occupyingIDs);
	for (int i = 0; i < ((thread_args *)args)->number_of_tracks; i++)
	{
		if (occupyingIDs[i] == ((thread_args *)args)->id)
		{
			occupyingIDs[i] = -1;
			break;
		}
	}
	for (int i = 0; i < ((thread_args *)args)->number_of_tracks; i++)
	{
		if (occupyingIDs[i] != -1)
			printf("%d ", occupyingIDs[i]);
	}
	sem_post(&mutex_occupyingIDs);

	sem_wait(&mutex_crossed);
	crossed++;
	printf("]\nNumber of trains that have crossed:%d\n", crossed);
	sem_post(&mutex_crossed);
	// End of printing and various handling routine

	fflush(stdout);

	free(args);
}

int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		printf("Usage: %s [Number of trains] [Time at station] [Minimum time between trains] [Maximum time between trains]\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	int
		ret,
		current_id = 0,
		N = atoi(argv[1]),
		T = atoi(argv[2]),
		T_min = atoi(argv[3]),
		T_max = atoi(argv[4]);

	pthread_t thread;

	srand(time(NULL));

	occupyingIDs = malloc(N * sizeof(int));
	for (int i = 0; i < N; i++)
		occupyingIDs[i] = -1;

	ret = sem_init(&semaphore, 1, N);
	if (ret < 0)
		handle_error("sem_init");
	ret = sem_init(&mutex_waiting, 1, 1);
	if (ret < 0)
		handle_error("sem_init");
	ret = sem_init(&mutex_crossed, 1, 1);
	if (ret < 0)
		handle_error("sem_init");
	ret = sem_init(&mutex_occupyingIDs, 1, 1);
	if (ret < 0)
		handle_error("sem_init");

	thread_args *args;

	while (1)
	{
		msleep((int)(((double)(T_max - T_min + 1) / RAND_MAX) * rand() + T_min));

		args = malloc(sizeof(thread_args));
		args->id = current_id;
		args->time_to_wait = T;
		args->number_of_tracks = N;
		ret = pthread_create(&thread, NULL, train, args);
		if (ret)
			handle_error("pthread_create");

		current_id++;
	}
}
