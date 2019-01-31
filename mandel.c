// Dylan Greene, 23 Febrauary 2017
// mandel - CSE 30341
// Note: Adapted from sourete to include color and multithreading capabilities
// Generates an image of a mandelbrot region

// Includes
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

// Thread arguments struct
struct thread_args{
    int thread_id, n_threads;
    struct bitmap *bm;
    double xmin, xmax, ymin, ymax;
    int itermax;
};

// Function Prototypes
int iteration_to_color(int i, int max);
int iterations_at_point(double x, double y, int max);
void *compute_image(void *thread_arg);

void show_help(){
    printf("Use: mandel [options]\n");
    printf("Where options are:\n");
    printf("-n <threads>    Set the number of threads to be used. (default=1)\n");
    printf("-m <max>        The maximum number of iterations per point. (default=1000)\n");
    printf("-x <coord>      X coordinate of image center point. (default=0)\n");
    printf("-y <coord>      Y coordinate of image center point. (default=0)\n");
    printf("-s <scale>      Scale of the image in Mandlebrot coordinates. (default=4)\n");
    printf("-W <pixels>     Width of the image in pixels. (default=500)\n");
    printf("-H <pixels>     Height of the image in pixels. (default=500)\n");
    printf("-o <file>       Set output file. (default=mandel.bmp)\n");
    printf("-h              Show this help text.\n");
    printf("\nSome examples are:\n");
    printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
    printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
    printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main(int argc, char *argv[]){
    char c;

    // These are the default configuration values used
    // if no command line arguments are given.
    const char *outfile = "mandel.bmp";
    double xcenter = 0;
    double ycenter = 0;
    double scale = 4;
    int    image_width = 500;
    int    image_height = 500;
    int    max = 1000;
    int    n_threads = 1;

    // For each command line argument given,
    // override the appropriate configuration value.
    while((c = getopt(argc,argv,"x:y:s:W:H:m:n:o:h"))!=-1){
        switch(c){
            case 'x':
                xcenter = atof(optarg);
                break;
            case 'y':
                ycenter = atof(optarg);
                break;
            case 's':
                scale = atof(optarg);
                break;
            case 'W':
                image_width = atoi(optarg);
                break;
            case 'H':
                image_height = atoi(optarg);
                break;
            case 'm':
                max = atoi(optarg);
                break;
            case 'n':
                n_threads = atoi(optarg);
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'h':
                show_help();
                exit(1);
                break;
        }
    }

    // Display the configuration of the image.
    printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s\n",xcenter,ycenter,scale,max,outfile);

    // Create a bitmap of the appropriate size.
    struct bitmap *bm = bitmap_create(image_width, image_height);

    // Fill it with a dark blue, for debugging
    bitmap_reset(bm, MAKE_RGBA(0, 0, 255, 0));

    // Set up pthreads and related argument structs
    struct thread_args *thread_arg_array = malloc(sizeof(struct thread_args) * n_threads);
    pthread_t *threads = malloc(sizeof(pthread_t) * n_threads);

    int t, ret;
    for(t = 0; t < n_threads; t++){
        thread_arg_array[t].thread_id = t;
        thread_arg_array[t].n_threads = n_threads;
        thread_arg_array[t].bm = bm;
        thread_arg_array[t].xmin = xcenter-scale;
        thread_arg_array[t].xmax = xcenter+scale;
        thread_arg_array[t].ymin = ycenter-scale;
        thread_arg_array[t].ymax = ycenter+scale;
        thread_arg_array[t].itermax = max;

        // Compute the Mandelbrot image portions
        ret = pthread_create(&threads[t], NULL, (void *) compute_image, (void *) &thread_arg_array[t]);
        if(ret){
            fprintf(stderr, "Unable to create thread; return code: %d\n", ret);
            exit(1);
        }
    }
    // Wait for the threads and join
    for(t = 0; t < n_threads; t++){
       ret = pthread_join(threads[t], NULL);
       if(ret){
          fprintf(stderr, "Return code from pthread_join() is %d\n", ret);
          exit(1);
        }
    }

    // Save the image in the stated file.
    if(!bitmap_save(bm, outfile)){
        fprintf(stderr, "mandel: couldn't write to %s: %s\n", outfile, strerror(errno));
        return 1;
    }

    //pthread_exit(NULL);
    free(threads);
    free(thread_arg_array);
    bitmap_delete(bm);
    return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/
void *compute_image(void *thread_arg){
    struct thread_args *args;
    args = (struct thread_args *) thread_arg;

    int i, j, start, end;

    int width = bitmap_width(args->bm);
    int height = bitmap_height(args->bm);

    // Threaded portion only has to do part of the image
    start = (height / args->n_threads) * args->thread_id;
    if(args->thread_id + 1 == args->n_threads){
        end = height;
    }else{
        end = start + (height / args->n_threads);
    }

    // For every pixel that the tread is assigned...
    for(j = start; j < end; j++) {
        for(i = 0; i < width; i++) {

            // Determine the point in x,y space for that pixel.
            double x = args->xmin + i*(args->xmax-args->xmin)/width;
            double y = args->ymin + j*(args->ymax-args->ymin)/height;

            // Compute the iterations at that point.
            int iters = iterations_at_point(x,y,args->itermax);

            // Set the pixel in the bitmap.
            bitmap_set(args->bm,i,j,iters);
        }
    }
    return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/
int iterations_at_point(double x, double y, int max){
    double x0 = x;
    double y0 = y;

    int iter = 0;

    while((x*x + y*y <= 4) && iter < max){
        double xt = x*x - y*y + x0;
        double yt = 2*x*y + y0;

        x = xt;
        y = yt;

        iter++;
    }

    return iteration_to_color(iter, max);
}

/*
Convert a iteration number to an RGBA color.
Modified to have a cool blue green color scheme
*/
int iteration_to_color(int i, int max){
    int r = 55 * i / max;
    int g = 155 * i / max;
    int b = 255 * i / max;
    return MAKE_RGBA(r, g, b, 0) / 5;
}
