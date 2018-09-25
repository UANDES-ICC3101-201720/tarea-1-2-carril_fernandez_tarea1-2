#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "const.h"
#include "util.h"

void swap(UINT *A, int a, int b) {
    UINT temp;
    temp = A[a]; 
    A[a] = A[b]; 
    A[b] = temp;
}

int partition (UINT* A, int lo, int hi){
    
    int pivot = A[hi];
    int i = (lo - 1);
    
    for (int j = lo; j <= hi-1; j++){
        
        if (A[j] <= pivot){
            i++;
            swap(A, i, j);
        }
    }
    swap(A, i + 1, hi);
    return (i + 1);
}

int quicksort(UINT* A, int lo, int hi) {

	if(lo<hi){

        int split = partition(A, lo, hi);
	quicksort(A,lo,split-1);
	quicksort(A,split+1,hi);

	}
    return 0;
    
}

// TODO: implement
int parallel_quicksort(UINT* A, int lo, int hi) {
    return 0;
}

int main(int argc, char** argv) {
    printf("[quicksort] Starting up...\n");
	int num_exp, opt, num_pot;
	char *c_num_pot;

    /* Get the number of CPU cores available */
    printf("[quicksort] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));

    /* TODO: parse arguments with getopt */
	while ((opt = getopt (argc, argv, "E:T:")) != -1)
	{
		switch (opt)
		{
			case 'E':
				num_exp = atoi(optarg);
				if(num_exp < 1) {
					printf("-E value out of range, exiting program\n");
					exit(-1);
				}
				break;
			case 'T':
				c_num_pot = optarg;
				num_pot = atoi(optarg);
				if(num_pot < 3 || num_pot > 9){
					printf("-T value out of range, exiting program\n");
					exit(-1);
				}
				break;
			case '?':
				printf("please use -E <number of experiments> -T <exponent of size of array> -P <position to find in array>");
				exit(-1);
		}
	}
	printf("c_num_pot = %s\n", c_num_pot);
    /* TODO: start datagen here as a child process. */
	int pid = fork();
	if(pid == 0){
		execvp("./datagen", argv);
	}
	else if (pid > 0){
		printf("datagen in action\n");
	}
	else if (pid < 0){
		fprintf(stderr, "Cannot make datagen child\n"); 
		exit(-1);
	}

    /* Create the domain socket to talk to datagen. */
    struct sockaddr_un addr;
    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("[quicksort] Socket error.\n");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("[quicksort] connect error.\n");
        close(fd);
        exit(-1);
    }

    /* DEMO: request two sets of unsorted random numbers to datagen */
    for (int i = 0; i < num_exp; i++) {
        char *begin = "BEGIN U ";
        size_t len = strlen(begin);
        char *begin_w_num = malloc(len + 1+ 1);
        strcpy(begin_w_num, begin);
        begin_w_num[len] = c_num_pot[0];
        begin_w_num[len + 1] = '\0';
        printf("sent command to datagen: %s\n", begin_w_num);
        int rc = strlen(begin_w_num);

        /* Request the random number stream to datagen */
        if (write(fd, begin_w_num, strlen(begin_w_num)) != rc) {
            if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
            else {
                perror("[quicksort] write error.\n");
                exit(-1);
            }
        }

        /* validate the response */
        char respbuf[10];
        read(fd, respbuf, strlen(DATAGEN_OK_RESPONSE));
        respbuf[strlen(DATAGEN_OK_RESPONSE)] = '\0';

        if (strcmp(respbuf, DATAGEN_OK_RESPONSE)) {
            perror("[quicksort] Response from datagen failed.\n");
            close(fd);
            exit(-1);
        }

        UINT readvalues = 0;
        size_t numvalues = pow(10, num_pot);
        size_t readbytes = 0;

        UINT *readbuf = malloc(sizeof(UINT) * numvalues);

        while (readvalues < numvalues) {
            /* read the bytestream */
            readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
            readvalues += readbytes / 4;
        }

        /* Print out the values obtained from datagen */
        printf("E%i: ", i + 1);
        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++) {
            printf("%u, ", *pv);
        }
        printf("\n");
        quicksort(readbuf, 0, (pow(10, num_pot) - 1) );
        printf("S%i: ", i + 1);
        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++) {
            printf("%u, ", *pv);
        }
        printf("\n");
        free(readbuf);
    }

    /* Issue the END command to datagen */
    int rc = strlen(DATAGEN_END_CMD);
    if (write(fd, DATAGEN_END_CMD, strlen(DATAGEN_END_CMD)) != rc) {
        if (rc > 0) fprintf(stderr, "[quicksort] partial write.\n");
        else {
            perror("[quicksort] write error.\n");
            close(fd);
            exit(-1);
        }
    }

    close(fd);
    exit(0);
}
