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


#pragma once


#include "cgp.h"
#include "image.h"


/**
 * Initializes fitness module - prepares test image
 * @param original
 * @param noisy
 */
void fitness_init(img_image_t original, img_image_t noisy);


/**
 * Deinitialize fitness module internals
 */
void fitness_deinit();


/**
 * Filters image using given filter. Caller is responsible for freeing
 * the filtered image
 *
 * Works in single thread
 *
 * @param  chr
 * @return fitness value
 */
img_image_t fitness_filter_image(ga_chr_t chr);


/**
 * Evaluates CGP circuit fitness
 *
 * @param  chr
 * @return fitness value
 */
ga_fitness_t fitness_eval_cgp(ga_chr_t chr);


/**
 * Predictes CGP circuit fitness
 *
 * @param  chr
 * @param  predictor
 * @return fitness value
 */
//ga_fitness_t fitness_predict_cgp(ga_chr_t chr, pred_genome predictor);


/**
 * Calculates fitness using the PSNR (peak signal-to-noise ratio) function.
 * The higher the value, the better the filter.
 *
 * @param  original image
 * @param  filtered image
 * @return fitness value (PSNR)
 */
ga_fitness_t fitness_psnr(img_image_t original, img_image_t filtered);
