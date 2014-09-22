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


#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "random.h"
#include "fitness.h"
#include "predictors.h"


static pred_gene_t _max_gene_value;
static pred_index_t _max_genome_length;
static pred_index_t _initial_genome_length;
static float _mutation_rate;
static float _offspring_elite;
static float _offspring_combine;


enum _offspring_op {
    random_mutant,
    crossover_product,
    keep_intact,
};


/**
 * Initialize predictor internals
 */
void pred_init(pred_gene_t max_gene_value, pred_index_t max_genome_length,
    pred_index_t initial_genome_length, float mutation_rate,
    float offspring_elite, float offspring_combine)
{
    _max_gene_value = max_gene_value;
    _max_genome_length = max_genome_length;
    _initial_genome_length = initial_genome_length;
    _mutation_rate = mutation_rate;
    _offspring_elite = offspring_elite;
    _offspring_combine = offspring_combine;

    assert(_initial_genome_length <= _max_genome_length);
}


/**
 * Create a new predictors population with given size
 * @param  size
 * @param  problem-specific methods
 * @return
 */
ga_pop_t pred_init_pop(int pop_size)
{
    /* prepare methods vector */
    ga_func_vect_t methods = {
        .alloc_genome = pred_alloc_genome,
        .free_genome = pred_free_genome,
        .init_genome = pred_randomize_genome,

        .fitness = fitness_eval_predictor,
        .offspring = pred_offspring,
    };

    /* initialize GA */
    ga_pop_t pop = ga_create_pop(pop_size, minimize, methods);
    ga_init_pop(pop);
    return pop;
}


/**
 * Allocates memory for new predictor genome
 * @return pointer to newly allocated genome
 */
void* pred_alloc_genome()
{
    pred_genome_t genome = (pred_genome_t) malloc(sizeof(struct pred_genome));
    if (genome == NULL) {
        return NULL;
    }

    genome->genes = (pred_gene_t*) malloc(sizeof(pred_gene_t) * _max_genome_length);
    if (genome->genes == NULL) {
        free(genome);
        return NULL;
    }

    return genome;
}


/**
 * Deinitialize predictor genome
 * @param  genome
 * @return
 */
void pred_free_genome(void *_genome)
{
    pred_genome_t genome = (pred_genome_t) _genome;
    free(genome->genes);
    free(genome);
}


/**
 * Initializes predictor genome to random values
 * @param chromosome
 */
int pred_randomize_genome(ga_chr_t chromosome)
{
    pred_genome_t genome = (pred_genome_t) chromosome->genome;

    genome->used_genes = _initial_genome_length;
    for (int i = 0; i < _max_genome_length; i++) {
        genome->genes[i] = rand_urange(0, _max_gene_value);
    }

    return 0;
}


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void pred_copy_genome(void *_dst, void *_src)
{
    pred_genome_t dst = (pred_genome_t) _dst;
    pred_genome_t src = (pred_genome_t) _src;

    memcpy(dst->genes, src->genes, sizeof(pred_gene_t) * _max_genome_length);
    dst->used_genes = src->used_genes;
}



/**
 * Genome mutation function
 * @param  chromosome
 * @return
 */
void pred_mutate(pred_gene_array_t genes)
{
    int max_changed_genes = _mutation_rate * _max_genome_length;
    int genes_to_change = rand_range(0, max_changed_genes);

    for (int i = 0; i < genes_to_change; i++) {
        int gene = rand_range(0, _max_genome_length - 1);
        genes[gene] = rand_urange(0, _max_gene_value);
    }
}


void _find_elites(ga_pop_t pop, int count, enum _offspring_op ops[])
{
    for (; count > 0; count--) {
        ga_fitness_t best_fitness = ga_worst_fitness(pop->problem_type);
        int best_index = -1;

        // find best non-selected individual
        for (int i = 0; i < pop->size; i++) {
            if (ops[i] != keep_intact) {
                ga_chr_t chr = pop->chromosomes[i];
                if (ga_is_better(pop->problem_type, chr->fitness, best_fitness)) {
                    best_fitness = chr->fitness;
                    best_index = i;
                }
            }
        }

        // mark best found as elite
        ops[best_index] = keep_intact;
    }
}


void _tournament(ga_pop_t pop, ga_chr_t *winner, ga_chr_t red, ga_chr_t blue)
{
    if (ga_is_better_or_same(pop->problem_type, red->fitness, blue->fitness)) {
        *winner = red;
    } else {
        *winner = blue;
    }
}


void _crossover1p(pred_gene_array_t baby, pred_gene_array_t mom, pred_gene_array_t dad)
{
    const int split_point = rand_range(0, _max_genome_length - 1);

    memcpy(baby, mom, sizeof(pred_gene_t) * split_point);

    memcpy(baby + split_point, dad + split_point,
        sizeof(pred_gene_t) * (_max_genome_length - split_point));
}


void _create_combined(ga_pop_t pop, pred_gene_array_t children)
{
    ga_chr_t mom;
    ga_chr_t dad;
    int red, blue;

    red = rand_range(0, pop->size - 1);
    blue = rand_range(0, pop->size - 1);
    _tournament(pop, &mom, pop->chromosomes[red], pop->chromosomes[blue]);

    red = rand_range(0, pop->size - 1);
    blue = rand_range(0, pop->size - 1);
    _tournament(pop, &dad, pop->chromosomes[red], pop->chromosomes[blue]);

    pred_genome_t mom_genome = (pred_genome_t)mom->genome;
    pred_genome_t dad_genome = (pred_genome_t)dad->genome;
    _crossover1p(children, mom_genome->genes, dad_genome->genes);

    /*
    for (int i = 0; i < _max_genome_length; i++) {
        printf("%4x ", mom_genome->genes[i]);
    }
    printf("\n");
    for (int i = 0; i < _max_genome_length; i++) {
        printf("%4x ", dad_genome->genes[i]);
    }
    printf("\n");
    for (int i = 0; i < _max_genome_length; i++) {
        printf("%4x ", children[i]);
    }
    printf("\n");
    */

    pred_mutate(children);

    /*
    for (int i = 0; i < _max_genome_length; i++) {
        printf("%4x ", children[i]);
    }
    printf("\n\n");
    */
}


/**
 * Create new generation
 * @param pop
 * @param mutation_rate
 */
void pred_offspring(ga_pop_t pop)
{
    // number of children copied and crossovered from parents
    const int elite_count = ceil(pop->size * _offspring_elite);
    const int crossover_count = ceil(pop->size * _offspring_combine);
    assert(elite_count + crossover_count <= pop->size);

    // this array describes how to create each individual
    enum _offspring_op child_type[pop->size];
    memset(child_type, random_mutant, sizeof(enum _offspring_op) * pop->size);

    // find which individuals will be kept intact
    _find_elites(pop, elite_count, child_type);

    // find which individuals will be replaced from parents
    // `i < pop->size` is already guarded by assert above
    int crossover_set = 0;
    for (int i = 0; crossover_set < crossover_count; i++) {
        if (child_type[i] != keep_intact) {
            child_type[i] = crossover_product;
            crossover_set++;
        }
    }

    // create new population
    #pragma omp parallel for
    for (int i = 0; i < pop->size; i++) {

        // copy elites
        if (child_type[i] == keep_intact) {
            ga_copy_chr(pop->children[i], pop->chromosomes[i], pred_copy_genome);

        // if there are any combined children to make, do it
        } else if (child_type[i] == crossover_product) {

            pred_genome_t target_genome = (pred_genome_t) pop->children[i]->genome;
            _create_combined(pop, target_genome->genes);
            target_genome->used_genes = _initial_genome_length;
            pop->children[i]->has_fitness = false;

        // otherwise create random mutant
        } else {

            pred_randomize_genome(pop->children[i]);
            pop->children[i]->has_fitness = false;
        }
    }

    // switch new and old population
    ga_chr_t *tmp = pop->chromosomes;
    pop->chromosomes = pop->children;
    pop->children = tmp;
}



/**
 * Dump predictor chromosome to file
 */
void pred_dump_chr(ga_chr_t chr, FILE *fp)
{
    pred_genome_t genome = (pred_genome_t) chr->genome;

    fprintf(fp, "%d used genes\n", genome->used_genes);
    return;

    for (int i = 0; i < _max_genome_length; i++) {
        if (i == genome->used_genes) fputs(" | ", fp);
        else if (i > 0) fputs(", ", fp);

        fprintf(fp, "%d", genome->genes[i]);
    }

    fprintf(fp, "\n");
}



/**
 * Dump predictor chromosome to file
 */
void pred_dump_pop(ga_pop_t pop, FILE *fp)
{
    fprintf(fp, "Generation: %d\n", pop->generation);
    fprintf(fp, "Best chromosome: %d\n", pop->best_chr_index);
    fprintf(fp, "Chromosomes: %d\n", pop->size);

    for (int i = 0; i < pop->size; i++) {
        pred_dump_chr(pop->chromosomes[i], fp);
    }
}
