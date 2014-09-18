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


#include "ga.h"


 /**
  * User-defined methods
  */
 typedef struct {
     /* memory allocation */
     ga_alloc_genome_func_t alloc_genome;
     ga_free_genome_func_t free_genome;

     /* copying */
     ga_copy_genome_func_t copy_genome;

     /* fitness function */
     ga_fitness_func_t fitness;
 } arc_func_vect_t;


struct archive
{
    /* archive capacity */
    int capacity;

    /* number of stored items */
    int stored;

    /* stored items - ring buffer */
    ga_chr_t *chromosomes;

    /* pointer to beginning of ring buffer - where new item will be
       stored */
    int pointer;

    /* genome-specific functions */
    arc_func_vect_t methods;
};
typedef struct archive* archive_t;


/**
 * Allocate memory for and initialize new archive
 *
 * @param  size Archive size
 * @param  problem-specific genome function pointers
 * @return pointer to created archive
 */
archive_t arc_create(int capacity, arc_func_vect_t methods);


/**
 * Release given archive from memory
 */
void arc_destroy(archive_t arc);


/**
 * Insert chromosome into archive
 *
 * Chromosome is copied into place and pointer to it is returned.
 *
 * Chromosome is reevaluated using `arc->methods.fitness` (if set).
 *
 * @param  arc
 * @param  chr
 * @return pointer to stored chromosome in archive
 */
ga_chr_t arc_insert(archive_t arc, ga_chr_t chr);


/**
 * OpenMP: Enters archive write critical section
 *
 * Blocks until write lock is obtained.
 */
void arc_omp_write_enter(archive_t arc);


/**
 * OpenMP: Leaves archive writer critical section
 */
void arc_omp_write_leave(archive_t arc);


/**
 * OpenMP: Enters archive read critical section
 *
 * Blocks until read lock is obtained.
 */
void arc_omp_read_enter(archive_t arc);


/**
 * OpenMP: Leaves archive readr critical section
 */
void arc_omp_read_leave(archive_t arc);


/**
 * Returns real index of item in archive's ring buffer
 */
static inline int arc_real_index(archive_t arc, int index)
{
    if (arc->stored < arc->capacity) {
        return index;

    } else {
        int real = (arc->pointer + index) % arc->capacity;
        if (real < 0) real += arc->capacity;
        return real;
    }
}


/**
 * Returns item stored on given index
 */
static inline ga_chr_t arc_get(archive_t arc, int index)
{
    return arc->chromosomes[arc_real_index(arc, index)];
}


#ifdef _OPENMP

    static inline void arc_write_enter(archive_t arc) {
        arc_omp_write_enter(arc);
    }
    static inline void arc_write_leave(archive_t arc) {
        arc_omp_write_leave(arc);
    }
    static inline void arc_read_enter(archive_t arc) {
        arc_omp_read_enter(arc);
    }
    static inline void arc_read_leave(archive_t arc) {
        arc_omp_read_leave(arc);
    }

#else

    static inline void arc_write_enter(archive_t arc) {
        // nothing to do in single-thread environment
    }
    static inline void arc_write_leave(archive_t arc) {
        // nothing to do in single-thread environment
    }
    static inline void arc_read_enter(archive_t arc) {
        // nothing to do in single-thread environment
    }
    static inline void arc_read_leave(archive_t arc) {
        // nothing to do in single-thread environment
    }

#endif