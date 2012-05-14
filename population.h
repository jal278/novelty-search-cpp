#ifndef _POPULATION_H_
#define _POPULATION_H_

#include <cmath>
#include <vector>
#include "innovation.h"
#include "genome.h"
#include "species.h"
#include "organism.h"

namespace NEAT {

	class Species;
	class Organism;

	// ---------------------------------------------  
	// POPULATION CLASS:
	//   A Population is a group of Organisms   
	//   including their species                        
	// ---------------------------------------------  
	class Population {

	protected: 

		// A Population can be spawned off of a single Genome 
		// There will be size Genomes added to the Population 
		// The Population does not have to be empty to add Genomes 
		bool spawn(Genome *g,int size);

	public:

        std::vector<Organism*> organisms; //The organisms in the Population

        std::vector<Species*> species;  // Species in the Population. Note that the species should comprise all the genomes 

		// ******* Member variables used during reproduction *******
        std::vector<Innovation*> innovations;  // For holding the genetic innovations of the newest generation
		int cur_node_id;  //Current label number available
		double cur_innov_num;

		int last_species;  //The highest species number

		// ******* Fitness Statistics *******
		double mean_fitness;
		double variance;
		double standard_deviation;

		int winnergen; //An integer that when above zero tells when the first winner appeared

		// ******* When do we need to delta code? *******
		double highest_fitness;  //Stagnation detector
		int highest_last_changed; //If too high, leads to delta coding

		// Separate the Organisms into species
		bool speciate();

		// Print Population to a file specified by a string 
		bool print_to_file(std::ostream& outFile);

		// Print Population to a file in speciated order with comments separating each species
		bool print_to_file_by_species(std::ostream& outFile);
		bool print_to_file_by_species(char *filename);

		// Prints the champions of each species to files starting with directory_prefix
		// The file name are as follows: [prefix]g[generation_num]cs[species_num]
		// Thus, they can be indexed by generation or species
		bool print_species_champs_tofiles(char *directory_prefix,int generation);

		// Run verify on all Genomes in this Population (Debugging)
		bool verify();

		// Turnover the population to a new generation using fitness 
		// The generation argument is the next generation
		bool epoch(int generation);

		// *** Real-time methods *** 

		// Places the organisms in species in order from best to worst fitness 
		bool rank_within_species();

		// Estimates average fitness for all existing species
		void estimate_all_averages();

		//Reproduce only out of the pop champ
		Organism* reproduce_champ(int generation);

		// Probabilistically choose a species to reproduce
		// Note that this method is effectively real-time fitness sharing in that the 
		// species will tend to produce offspring in an amount proportional
		// to their average fitness, which approximates the generational
		// method of producing the next generation of the species en masse
		// based on its average (shared) fitness.  
		Species *choose_parent_species();

		//Remove a species from the species list (sometimes called by remove_worst when a species becomes empty)
		bool remove_species(Species *spec);

		// Removes worst member of population that has been around for a minimum amount of time and returns
		// a pointer to the Organism that was removed (note that the pointer will not point to anything at all,
		// since the Organism it was pointing to has been deleted from memory)
		Organism* remove_worst();

		//Warning: rtNEAT does not behave like regular NEAT if you remove the worst probabilistically   
		//You really should just use "remove_worst," which removes the org with worst adjusted fitness. 
		Organism* remove_worst_probabilistic();

		//KEN: New 2/17/04
		//This method takes an Organism and reassigns what Species it belongs to
		//It is meant to be used so that we can reasses where Organisms should belong
		//as the speciation threshold changes.
        void reassign_species(Organism *org);

		//Move an Organism from one Species to another (called by reassign_species)
		void switch_species(Organism *org, Species *orig_species, Species *new_species);

		// Construct off of a single spawning Genome 
		Population(Genome *g,int size);

		// Construct off of a single spawning Genome without mutation
		Population(Genome *g,int size, float power);
		
		//MSC Addition
		// Construct off of a vector of genomes with a mutation rate of "power"
		Population(std::vector<Genome*> genomeList, float power);

		bool clone(Genome *g,int size, float power);

		//// Special constructor to create a population of random topologies     
		//// uses Genome(int i, int o, int n,int nmax, bool r, double linkprob) 
		//// See the Genome constructor for the argument specifications
		//Population(int size,int i,int o, int nmax, bool r, double linkprob);

		// Construct off of a file of Genomes 
		Population(const char *filename);

		// It can delete a Population in two ways:
		//    -delete by killing off the species
		//    -delete by killing off the organisms themselves (if not speciated)
		// It does the latter if it sees the species list is empty
		~Population();

		

	};

} // namespace NEAT

#endif
