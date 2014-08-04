/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master Thesis
 * 2014/2015
 *
 * Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>
 *
 * Faculty of Information Technologies
 * Brno University of Technology
 * http://www.fit.vutbr.cz/
 *
 * Started on 28/07/2014.
 *      _       _
 *   __(.)=   =(.)__
 *   \___)     (___/
 */


#include <stdlib.h>
#include <assert.h>

#ifdef FITNESS_USE_PTHREAD
    #include <pthread.h>
#endif

#include "fitness.h"


img_image original_image;
img_window_array noisy_image_windows;


/**
 * Initializes fitness module - prepares test image
 * @param original
 * @param noisy
 */
void fitness_init(img_image original, img_image noisy)
{
    assert(original->width == noisy->width);
    assert(original->height == noisy->height);
    assert(original->comp == noisy->comp);

    original_image = original;
    noisy_image_windows = img_split_windows(noisy);
}


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit()
{
    img_windows_destroy(noisy_image_windows);
}


/**
 * Filters image using given filter. Caller is responsible for freeing
 * the filtered image
 *
 * Works in single thread
 *
 * @param  chr
 * @return fitness value
 */
img_image _fitness_filter_image_simple(cgp_chr chr)
{
    img_image filtered = img_create(original_image->width, original_image->height,
        original_image->comp);

    for (int i = 0; i < noisy_image_windows->size; i++) {
        img_window *w = &noisy_image_windows->windows[i];

        cgp_value_t *inputs = w->pixels;
        cgp_value_t output_pixel[1];
        cgp_get_output(chr, inputs, output_pixel);

        img_set_pixel(filtered, w->pos_x, w->pos_y, output_pixel[0]);
    }

    return filtered;
}


#ifdef FITNESS_USE_PTHREAD


typedef struct {
    int start;
    int stop;
    cgp_chr chr;
    img_image filtered;
} fitness_filter_image_pthread_work;


/**
 * _fitness_filter_image_pthread helper - thread source code
 * @param chromosome to evaluate
 */
void* _fitness_filter_image_pthread_worker(void *_work)
{
    fitness_filter_image_pthread_work *work =(fitness_filter_image_pthread_work*) _work;

    for (int i = work->start; i < work->stop; i++) {
        img_window *w = &noisy_image_windows->windows[i];

        cgp_value_t *inputs = w->pixels;
        cgp_value_t output_pixel[1];
        cgp_get_output(work->chr, inputs, output_pixel);

        // thread-safe: each thread writes to different pixels
        img_set_pixel(work->filtered, w->pos_x, w->pos_y, output_pixel[0]);
    }

    return NULL;
}

/**
 * Filters image using given filter. Caller is responsible for freeing
 * the filtered image
 *
 * Uses FITNESS_NUMTHREADS threads for paralelization
 *
 * @param  chr
 * @return fitness value
 */
img_image _fitness_filter_image_pthread(cgp_chr chr)
{
    img_image filtered = img_create(original_image->width, original_image->height,
        original_image->comp);


    /* prepare work for threads */

    // rounds up the number of tasks per thread, useful if windows count
    // does not divide evenly by NUMTHREADS
    int per_thread = (noisy_image_windows->size + FITNESS_NUMTHREADS - 1) / FITNESS_NUMTHREADS;
    fitness_filter_image_pthread_work works[FITNESS_NUMTHREADS];
    pthread_t threads[FITNESS_NUMTHREADS];

    for (int i = 0; i < FITNESS_NUMTHREADS; i++) {
        works[i].chr = chr;
        works[i].filtered = filtered;
        works[i].start = i * per_thread;
        works[i].stop = (i == FITNESS_NUMTHREADS - 1)
            ? noisy_image_windows->size
            : (i + 1) * per_thread;
    }

    for (int i = 0; i < FITNESS_NUMTHREADS; i++) {
        pthread_create(&threads[i], NULL, _fitness_filter_image_pthread_worker, &works[i]);
    }

    for (int i = 0; i < FITNESS_NUMTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return filtered;
}


#endif /* FITNESS_USE_PTHREAD */


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
cgp_fitness_t fitness_eval_cgp(cgp_chr chr)
{
    img_image filtered = fitness_filter_image(chr);
    cgp_fitness_t fitness = fitness_psnr(original_image, filtered);
    img_destroy(filtered);

    return fitness;
}


/**
 * Calculates fitness using the PSNR (peak signal-to-noise ratio) function.
 * The higher the value, the better the filter.
 *
 * @param  original image
 * @param  filtered image
 * @return fitness value (PSNR)
 */
cgp_fitness_t fitness_psnr(img_image original, img_image filtered)
{
    assert(original->width == filtered->width);
    assert(original->height == filtered->height);
    assert(original->comp == filtered->comp);

    double coef = 255 * 255 * (double)original->width * original->height;
    double sum = 0;

    for (int x = 0; x < original->width; x++) {
        for (int y = 0; y < original->height; y++) {
            double diff = img_get_pixel(filtered, x, y) - img_get_pixel(original, x, y);
            sum += diff * diff;
        }
    }

    return coef / sum;
}
