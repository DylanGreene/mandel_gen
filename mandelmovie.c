// Dylan Greene, 23 Febrauary 2017
// mandelmovie - CSE 30341
// Keeps the same x and y value in the mandel set, but progressively
// zooms in and creates a series of images to allow creation of an animation

// Paramters for Mandlebrot movie
#define N_FRAMES 50
#define ZOOM_START 2
#define ZOOM_FINISH 0.00000000001
#define X "0.2929859127507"
#define Y "0.6117848324958"
#define MAX_ITER 10000
#define HEIGHT 1024
#define WIDTH 1024
#define THREADS 1

// Includes
#include <stdio.h> // printf
#include <stdlib.h> // exit and atoi
#include <math.h> // exp and log
#include <unistd.h> // fork and exec syscalls
#include <sys/types.h> // pid_t
#include <sys/wait.h> // wait syscall
#include <string.h> // strerror, strcmp, strsignal, and strtok
#include <errno.h> // errno

// Functions Prototypes
void usage(); // Prints when usage is incorrect and exits with code 1
int generate_mandelbrot_frames(int n_processes); // Loops to create the frames
void generate_args(char *command, char *args[]); // Splits a command into args

// Main Execution
int main(int argc, char *argv[]){
    // Check for proper usage and set the number of processes to be used
    int n_processes;
    if(argc == 1) n_processes = 1;
    else if(argc == 2){
        n_processes = atoi(argv[1]);
        if(n_processes == 0) usage();
    }else usage();

    generate_mandelbrot_frames(n_processes);
    return 0;
}

// Function Implementations

// Usage function to print usage and exit with code 1
void usage(){
    fprintf(stderr, "Usage: ./mandelmovie [Number of processes]\n");
    exit(1);
}

// The execution loop to actually generate the frames
int generate_mandelbrot_frames(int n_processes){
    // Get set up to start generating images and then start
    double zoom = ZOOM_START;
    int n_running_processes = 0, n_frame = 1;
    while(n_frame <= N_FRAMES){
        // Get the command to be used
        char command[512];
        char file_name[128];
        sprintf(file_name, "mandel%d.bmp", n_frame);
        sprintf(command, "./mandel -x %s -y %s -m %d -s %.15lf -H %d -W %d -n %d -o %s",
                X, Y, MAX_ITER, zoom, HEIGHT, WIDTH, THREADS, file_name);
        // Split the command into args for exec
        char *args[100];
        generate_args(command, args);

        // Smooth zoom
        zoom *= exp(log(ZOOM_FINISH / ZOOM_START) / 50.0);

        // Fork to create child that execs
        pid_t pid = fork();
        n_running_processes++;
        if(pid < 0){ // Fork failed
            perror("Fork failed");
            exit(1);
        }else if(pid == 0){ // Child
            if(execvp(args[0], args) < 0){
                perror("Unable to exec");
                exit(1);
            }
        }else{ // Parent
            if(n_running_processes >= n_processes){
                pid_t tid = wait(NULL);
                if(tid < 0){
                    perror("Wait failed");
                    exit(1);
                }
                n_running_processes--;
            }
        }
        n_frame++;
    }

    // Wait for any remaining processes
    while(n_running_processes > 0){
        pid_t tid = wait(NULL);
        if(tid < 0){
            perror("Wait failed");
            exit(1);
        }
        n_running_processes--;
    }
    return 0;
}

// Splits a command into args for use in exec
void generate_args(char *command, char *args[]){
    // Parse each 'word' in the command so exec can use it
    char *arg = strtok(command, " ");
    int i = 0;
    for(i = 0; arg != NULL; i++){
        args[i] = arg;
        arg = strtok(NULL, " \t\n");
    }
    args[i] = (char *)NULL;
}
