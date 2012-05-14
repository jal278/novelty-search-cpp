#include "population.h"
#include "organism.h"
#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;
using namespace NEAT;

extern int NEAT::time_alive_minimum;

Population::Population(Genome *g,int size) {
	winnergen=0;
	highest_fitness=0.0;
	highest_last_changed=0;
	spawn(g,size);
}

Population::Population(Genome *g,int size, float power) {
	winnergen=0;
	highest_fitness=0.0;
	highest_last_changed=0;
	clone(g, size, power);
}

//Population::Population(int size,int i,int o, int nmax, bool r, double linkprob) {    
//int count;
//Genome *new_genome; 

//cout<<"Making a random pop"<<endl;

//winnergen=0;
//highest_fitness=0.0;
//highest_last_changed=0;

//for(count=0;count<size;count++) {
//new_genome=new Genome(count,i,o,randint(0,nmax),nmax,r,linkprob);
//organisms.push_back(new Organism(0,new_genome,1));
//}

//cur_node_id=i+o+nmax+1;;
//cur_innov_num=(i+o+nmax)*(i+o+nmax)+1;

//cout<<"Calling speciate"<<endl;
//speciate(); 

//}


//MSC Addition
//Added the ability for a population to be spawned
//off of a vector of Genomes.  Useful when converging.
Population::Population(std::vector<Genome*> genomeList, float power) {
	
	winnergen=0;
	highest_fitness=0.0;
	highest_last_changed=0;
		
	int count;
	Genome *new_genome;
	Organism *new_organism;

	//Create size copies of the Genome
	//Start with perturbed linkweights
	for (std::vector<Genome*>::iterator iter = genomeList.begin(); iter != genomeList.end(); ++iter)
	{

		new_genome=(*iter); 
		if(power>0)
			new_genome->mutate_link_weights(power,1.0,GAUSSIAN);
		//new_genome->mutate_link_weights(1.0,1.0,COLDGAUSSIAN);
		new_genome->randomize_traits();
		new_organism=new Organism(0.0,new_genome,1);
		organisms.push_back(new_organism);
	}

	//Keep a record of the innovation and node number we are on
	cur_node_id=new_genome->get_last_node_id();
	cur_innov_num=new_genome->get_last_gene_innovnum();

	//Separate the new Population into species
	speciate();
}

Population::Population(const char *filename) {

	char curword[4096];  //max word size of 128 characters
	char curline[8000]; //max line size of 1024 characters
	char delimiters[] = " \n";

	Genome *new_genome;

	winnergen=0;

	highest_fitness=0.0;
	highest_last_changed=0;

	cur_node_id=0;
	cur_innov_num=0.0;

	int curwordnum = 0;

	std::ifstream iFile(filename);
	if (!iFile) {
		printf("Can't open genomes file for input");
		return;
	}

	else {
		bool md = false;
		char metadata[4024];
		//Loop until file is finished, parsing each line
		while (!iFile.eof()) 
		{
			iFile.getline(curline, sizeof(curline));
            std::stringstream ss(curline);
			//strcpy(curword, NEAT::getUnit(curline, 0, delimiters));
            ss >> curword;
			if(iFile.eof()) break;
            //std::cout << curline << std::endl;

			//Check for next
			if (strcmp(curword,"genomestart")==0) 
			{
				//strcpy(curword, NEAT::getUnit(curline, 1, delimiters));
				//int idcheck = atoi(curword);

                int idcheck;
                ss >> idcheck;
				// If there isn't metadata, set metadata to ""
				if(md == false)  {
					strcpy(metadata, "");
				}
				md = false;

				new_genome=new Genome(idcheck,iFile);
				organisms.push_back(new Organism(0,new_genome,1, metadata));
				if (cur_node_id<(new_genome->get_last_node_id()))
					cur_node_id=new_genome->get_last_node_id();

				if (cur_innov_num<(new_genome->get_last_gene_innovnum()))
					cur_innov_num=new_genome->get_last_gene_innovnum();
			}
			else if (strcmp(curword,"/*")==0) 
			{
				
				// New metadata possibly, so clear out the metadata
				strcpy(metadata, "");
				curwordnum=1;
				//strcpy(curword, NEAT::getUnit(curline, curwordnum++, delimiters));
                ss >> curword;

				while(strcmp(curword,"*/")!=0)
				{
					// If we've started to form the metadata, put a space in the front
					if(md) {
						strncat(metadata, " ", 4024 - strlen(metadata));
					}

					// Append the next word to the metadata, and say that there is metadata
					strncat(metadata, curword, 4024 - strlen(metadata));
					md = true;

					//strcpy(curword, NEAT::getUnit(curline, curwordnum++, delimiters));
                    ss >> curword;
				}
			}
			//Ignore comments - they get printed to screen
			//else if (strcmp(curword,"/*")==0) {
			//	iFile>>curword;
			//	while (strcmp(curword,"*/")!=0) {
			//cout<<curword<<" ";
			//		iFile>>curword;
			//	}
			//	cout<<endl;

			//}
			//Ignore comments surrounded by - they get printed to screen
		}

		iFile.close();

		speciate();

	}
}


Population::~Population() {

	std::vector<Species*>::iterator curspec;
	std::vector<Organism*>::iterator curorg;
	//std::vector<Generation_viz*>::iterator cursnap;

	if (species.begin()!=species.end()) {
		for(curspec=species.begin();curspec!=species.end();++curspec) {
			delete (*curspec);
		}
	}
	else {
		for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
			delete (*curorg);
		}
	}

	for (std::vector<Innovation*>::iterator iter = innovations.begin(); iter != innovations.end(); ++iter)
		delete *iter;

	//Delete the snapshots
	//		for(cursnap=generation_snapshots.begin();cursnap!=generation_snapshots.end();++cursnap) {
	//			delete (*cursnap);
	//		}
}

bool Population::verify() {
	std::vector<Organism*>::iterator curorg;

	bool verification;

	for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
		verification=((*curorg)->gnome)->verify();
	}

	return verification;
} 

bool Population::clone(Genome *g,int size, float power) {
	int count;
	Genome *new_genome;
	Organism *new_organism;

	new_genome = g->duplicate(1); 
	new_organism = new Organism(0.0,new_genome,1);
	organisms.push_back(new_organism);
	
	//Create size copies of the Genome
	//Start with perturbed linkweights
	for(count=2;count<=size;count++) {
		//cout<<"CREATING ORGANISM "<<count<<endl;
		new_genome=g->duplicate(count); 
		if(power>0)
			new_genome->mutate_link_weights(power,1.0,GAUSSIAN);
		
		new_genome->randomize_traits();
		new_organism=new Organism(0.0,new_genome,1);
		organisms.push_back(new_organism);
	}

	//Keep a record of the innovation and node number we are on
	cur_node_id=new_genome->get_last_node_id();
	cur_innov_num=new_genome->get_last_gene_innovnum();

	//Separate the new Population into species
	speciate();

	return true;
}

bool Population::spawn(Genome *g,int size) {
	int count;
	Genome *new_genome;
	Organism *new_organism;

	//Create size copies of the Genome
	//Start with perturbed linkweights
	for(count=1;count<=size;count++) {
		//cout<<"CREATING ORGANISM "<<count<<endl;

		new_genome=g->duplicate(count); 		
		//new_genome->mutate_link_weights(1.0,1.0,GAUSSIAN);
		new_genome->mutate_link_weights(1.0,1.0,COLDGAUSSIAN);
		new_genome->randomize_traits();
		new_organism=new Organism(0.0,new_genome,1);
		organisms.push_back(new_organism);
	}

	//Keep a record of the innovation and node number we are on
	cur_node_id=new_genome->get_last_node_id();
	cur_innov_num=new_genome->get_last_gene_innovnum();

	//Separate the new Population into species
	speciate();

	return true;
}

bool Population::speciate() {
	std::vector<Organism*>::iterator curorg;  //For stepping through Population
	std::vector<Species*>::iterator curspecies; //Steps through species
	Organism *comporg=0;  //Organism for comparison 
	Species *newspecies; //For adding a new species

	int counter=0; //Species counter

	//Step through all existing organisms
	for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {

		//For each organism, search for a species it is compatible to
		curspecies=species.begin();
		if (curspecies==species.end()){
			//Create the first species
			newspecies=new Species(++counter);
			species.push_back(newspecies);
			newspecies->add_Organism(*curorg);  //Add the current organism
			(*curorg)->species=newspecies;  //Point organism to its species
		} 
		else {
			comporg=(*curspecies)->first();
			while((comporg!=0)&&
				(curspecies!=species.end())) {

					if ((((*curorg)->gnome)->compatibility(comporg->gnome))<NEAT::compat_threshold) {

						//Found compatible species, so add this organism to it
						(*curspecies)->add_Organism(*curorg);
						(*curorg)->species=(*curspecies);  //Point organism to its species
						comporg=0;  //Note the search is over
					}
					else {

						//Keep searching for a matching species
						++curspecies;
						if (curspecies!=species.end()) 
							comporg=(*curspecies)->first();
					}
				}

				//If we didn't find a match, create a new species
				if (comporg!=0) {
					newspecies=new Species(++counter);
					species.push_back(newspecies);
					newspecies->add_Organism(*curorg);  //Add the current organism
					(*curorg)->species=newspecies;  //Point organism to its species
				}

		} //end else 

	} //end for

	last_species=counter;  //Keep track of highest species

	return true;
}

bool Population::print_to_file_by_species(char *filename) {

  std::vector<Species*>::iterator curspecies;

  std::ofstream outFile(filename,std::ios::out);

  //Make sure it worked
  if (!outFile) {
    std::cerr<<"Can't open "<<filename<<" for output"<<std::endl;
    return false;
  }


  //Step through the Species and print them to the file
  for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
    (*curspecies)->print_to_file(outFile);
  }

  outFile.close();

  return true;

}


bool Population::print_to_file_by_species(std::ostream& outFile) {

	std::vector<Species*>::iterator curspecies;

	//ofstream outFile(filename,ios::out);
	//std::ostream outFile;
	//ResourceManager->openFileForWrite(outFile, fileName, std::ostream::Write);

	//Make sure it worked
	//if (!outFile) {
	//	cerr<<"Can't open "<<filename<<" for output"<<endl;
	//	return false;
	//}


	//Step through the Species and print them to the file
	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		(*curspecies)->print_to_file(outFile);
	}

	return true;

}

bool Population::epoch(int generation) {

	std::vector<Species*>::iterator curspecies;
	std::vector<Species*>::iterator deadspecies;  //For removing empty Species

	std::vector<Organism*>::iterator curorg;
	std::vector<Organism*>::iterator deadorg;

	std::vector<Innovation*>::iterator curinnov;  
	std::vector<Innovation*>::iterator deadinnov;  //For removing old Innovs

	double total=0.0; //Used to compute average fitness over all Organisms

	double overall_average;  //The average modified fitness among ALL organisms

	int orgcount;

	static double max_fit=0.0;
	double high_fit=0.0;
	//The fractional parts of expected offspring that can be 
	//Used only when they accumulate above 1 for the purposes of counting
	//Offspring
	double skim; 
	int total_expected;  //precision checking
	int total_organisms=organisms.size();
	int max_expected;
	Species *best_species;
	int final_expected;

	int pause;

	//Rights to make babies can be stolen from inferior species
	//and given to their superiors, in order to concentrate exploration on
	//the best species
	int NUM_STOLEN=NEAT::babies_stolen; //Number of babies to steal
	int one_fifth_stolen;
	int one_tenth_stolen;

	std::vector<Species*> sorted_species;  //Species sorted by max fit org in Species
	int stolen_babies; //Babies taken from the bad species and given to the champs

	int half_pop;

	int best_species_num;  //Used in debugging to see why (if) best species dies
	bool best_ok;

	//We can try to keep the number of species constant at this number
	int num_species_target=10;
	int num_species=species.size();
	double compat_mod=0.3;  //Modify compat thresh to control speciation


	//Keeping species diverse
	//This commented out code forces the system to aim for 
	// num_species species at all times, enforcing diversity
	//This tinkers with the compatibility threshold, which
	// normally would be held constant
	
	if (generation>1) {
		if (num_species<num_species_target)
			NEAT::compat_threshold-=compat_mod;
		else if (num_species>num_species_target)
			NEAT::compat_threshold+=compat_mod;

		if (NEAT::compat_threshold<0.3) NEAT::compat_threshold=0.3;

	}
	


	//Stick the Species pointers into a new Species list for sorting
	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		sorted_species.push_back(*curspecies);
	}

	//Sort the Species by max fitness (Use an extra list to do this)
	//These need to use ORIGINAL fitness
	//sorted_species.qsort(order_species);
    std::sort(sorted_species.begin(), sorted_species.end(), order_species);

	//Flag the lowest performing species over age 20 every 30 generations 
	//NOTE: THIS IS FOR COMPETITIVE COEVOLUTION STAGNATION DETECTION

	/*
	curspecies=sorted_species.end();
	curspecies--;
	while((curspecies!=sorted_species.begin())&&
		((*curspecies)->age<20))
		--curspecies;
	if ((generation%30)==0)
		(*curspecies)->obliterate=true;
	*/

	std::cout<<"Number of Species: "<<num_species<<std::endl;
	std::cout<<"compat_thresh: "<<compat_threshold<<std::endl;

	//Use Species' ages to modify the objective fitness of organisms
	// in other words, make it more fair for younger species
	// so they have a chance to take hold
	//Also penalize stagnant species
	//Then adjust the fitness using the species size to "share" fitness
	//within a species.
	//Then, within each Species, mark for death 
	//those below survival_thresh*average
	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		(*curspecies)->adjust_fitness();
	}

	//Go through the organisms and add up their fitnesses to compute the
	//overall average
	for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
		total+=(*curorg)->fitness;
	}
	overall_average=total/total_organisms;
	std::cout<<"Generation "<<generation<<": "<<"overall_average = "<<overall_average<<std::endl;

	//Now compute expected number of offspring for each individual organism
	for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
		(*curorg)->expected_offspring=(((*curorg)->fitness)/overall_average);
	}

	//Now add those offspring up within each Species to get the number of
	//offspring per Species
	skim=0.0;
	total_expected=0;
	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		skim=(*curspecies)->count_offspring(skim);
		total_expected+=(*curspecies)->expected_offspring;
	}    

	//Need to make up for lost foating point precision in offspring assignment
	//If we lost precision, give an extra baby to the best Species
	if (total_expected<total_organisms) {
		//Find the Species expecting the most
		max_expected=0;
		final_expected=0;
		for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
			if ((*curspecies)->expected_offspring>=max_expected) {
				max_expected=(*curspecies)->expected_offspring;
				best_species=(*curspecies);
			}
			final_expected+=(*curspecies)->expected_offspring;
		}
		//Give the extra offspring to the best species
		++(best_species->expected_offspring);
		final_expected++;

		//If we still arent at total, there is a problem
		//Note that this can happen if a stagnant Species
		//dominates the population and then gets killed off by its age
		//Then the whole population plummets in fitness
		//If the average fitness is allowed to hit 0, then we no longer have 
		//an average we can use to assign offspring.
		if (final_expected<total_organisms) {
			//      cout<<"Population died!"<<endl;
			//cin>>pause;
			for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
				(*curspecies)->expected_offspring=0;
			}
			best_species->expected_offspring=total_organisms;
		}
	}

	//Sort the Species by max fitness (Use an extra list to do this)
	//These need to use ORIGINAL fitness
	//sorted_species.qsort(order_species);
    std::sort(sorted_species.begin(), sorted_species.end(), order_species);

	best_species_num=(*(sorted_species.begin()))->id;

	for(curspecies=sorted_species.begin();curspecies!=sorted_species.end();++curspecies) {

		//Print out for Debugging/viewing what's going on 
		std::cout<<"orig fitness of Species"<<(*curspecies)->id<<"(Size "<<(*curspecies)->organisms.size()<<"): "<<(*((*curspecies)->organisms).begin())->orig_fitness<<" last improved "<<((*curspecies)->age-(*curspecies)->age_of_last_improvement)<<std::endl;
	}

	//Check for Population-level stagnation
	curspecies=sorted_species.begin();
	(*(((*curspecies)->organisms).begin()))->pop_champ=true; //DEBUG marker of the best of pop
	if (((*(((*curspecies)->organisms).begin()))->orig_fitness)>
		highest_fitness) {
			highest_fitness=((*(((*curspecies)->organisms).begin()))->orig_fitness);
			highest_last_changed=0;
			std::cout<<"NEW POPULATION RECORD FITNESS: "<<highest_fitness<<std::endl;
		}
	else {
		++highest_last_changed;
		std::cout<<highest_last_changed<<" generations since last population fitness record: "<<highest_fitness<<std::endl;
	}

	if(highest_fitness>max_fit)
	{
		max_fit=highest_fitness;
	}


	//Check for stagnation- if there is stagnation, perform delta-coding
	if (highest_last_changed>=NEAT::dropoff_age+5) {

		//    cout<<"PERFORMING DELTA CODING"<<endl;

		highest_last_changed=0;

		half_pop=NEAT::pop_size/2;

		//    cout<<"half_pop"<<half_pop<<" pop_size-halfpop: "<<pop_size-half_pop<<endl;

		curspecies=sorted_species.begin();

		(*(((*curspecies)->organisms).begin()))->super_champ_offspring=half_pop;
		(*curspecies)->expected_offspring=half_pop;
		(*curspecies)->age_of_last_improvement=(*curspecies)->age;

		++curspecies;

		if (curspecies!=sorted_species.end()) {

			(*(((*curspecies)->organisms).begin()))->super_champ_offspring=NEAT::pop_size-half_pop;
			(*curspecies)->expected_offspring=NEAT::pop_size-half_pop;
			(*curspecies)->age_of_last_improvement=(*curspecies)->age;

			++curspecies;

			//Get rid of all species under the first 2
			while(curspecies!=sorted_species.end()) {
				(*curspecies)->expected_offspring=0;
				++curspecies;
			}
		}
		else {
			curspecies=sorted_species.begin();
			(*(((*curspecies)->organisms).begin()))->super_champ_offspring+=NEAT::pop_size-half_pop;
			(*curspecies)->expected_offspring=NEAT::pop_size-half_pop;
		}

	}
	//STOLEN BABIES:  The system can take expected offspring away from
	//  worse species and give them to superior species depending on
	//  the system parameter babies_stolen (when babies_stolen > 0)
	else if (NEAT::babies_stolen>0) {
		//Take away a constant number of expected offspring from the worst few species

		stolen_babies=0;
		curspecies=sorted_species.end();
		curspecies--;
		while ((stolen_babies<NUM_STOLEN)&&
			(curspecies!=sorted_species.begin())) {

				//cout<<"Considering Species "<<(*curspecies)->id<<": age "<<(((*curspecies)->age))<<" expected offspring "<<(((*curspecies)->expected_offspring))<<endl;

				if ((((*curspecies)->age)>5)&&
					(((*curspecies)->expected_offspring)>2)) {
						//cout<<"STEALING!"<<endl;

						//This species has enough to finish off the stolen pool
						if (((*curspecies)->expected_offspring-1)>=(NUM_STOLEN-stolen_babies)) {
							(*curspecies)->expected_offspring-=(NUM_STOLEN-stolen_babies);
							stolen_babies=NUM_STOLEN;
						}
						//Not enough here to complete the pool of stolen
						else {
							stolen_babies+=(*curspecies)->expected_offspring-1;
							(*curspecies)->expected_offspring=1;

						}
					}

					curspecies--;

					//if (stolen_babies>0)
					//cout<<"stolen babies so far: "<<stolen_babies<<endl;
			}

			//cout<<"STOLEN BABIES: "<<stolen_babies<<endl;

			//Mark the best champions of the top species to be the super champs
			//who will take on the extra offspring for cloning or mutant cloning
			curspecies=sorted_species.begin();

			//Determine the exact number that will be given to the top three
			//They get , in order, 1/5 1/5 and 1/10 of the stolen babies
			one_fifth_stolen=NEAT::babies_stolen/5;
			one_tenth_stolen=NEAT::babies_stolen/10;

			//Don't give to dying species even if they are champs
			while((curspecies!=sorted_species.end())&& ((*curspecies)->last_improved()>NEAT::dropoff_age))
				++curspecies;

			//Concentrate A LOT on the number one species
			if ((curspecies!=sorted_species.end()) && (stolen_babies>=one_fifth_stolen)) {
				(*(((*curspecies)->organisms).begin()))->super_champ_offspring=one_fifth_stolen;
				(*curspecies)->expected_offspring+=one_fifth_stolen;
				stolen_babies-=one_fifth_stolen;
				//cout<<"Gave "<<one_fifth_stolen<<" babies to Species "<<(*curspecies)->id<<endl;
				//      cout<<"The best superchamp is "<<(*(((*curspecies)->organisms).begin()))->gnome->genome_id<<endl;

				//Print this champ to file "champ" for observation if desired
				//IMPORTANT:  This causes generational file output 
				//print_Genome_tofile((*(((*curspecies)->organisms).begin()))->gnome,"champ");

				curspecies++;

			}

			//Don't give to dying species even if they are champs
			while((curspecies!=sorted_species.end())&&((*curspecies)->last_improved()>NEAT::dropoff_age))
				++curspecies;

			if ((curspecies!=sorted_species.end())) {
				if (stolen_babies>=one_fifth_stolen) {
					(*(((*curspecies)->organisms).begin()))->super_champ_offspring=one_fifth_stolen;
					(*curspecies)->expected_offspring+=one_fifth_stolen;
					stolen_babies-=one_fifth_stolen;
					//cout<<"Gave "<<one_fifth_stolen<<" babies to Species "<<(*curspecies)->id<<endl;
					curspecies++;

				}
			}

			//Don't give to dying species even if they are champs
			while(((*curspecies)->last_improved()>NEAT::dropoff_age)&&(curspecies!=sorted_species.end()))
				++curspecies;

			if (curspecies!=sorted_species.end())
				if (stolen_babies>=one_tenth_stolen) {
					(*(((*curspecies)->organisms).begin()))->super_champ_offspring=one_tenth_stolen;
					(*curspecies)->expected_offspring+=one_tenth_stolen;
					stolen_babies-=one_tenth_stolen;

					//cout<<"Gave "<<one_tenth_stolen<<" babies to Species "<<(*curspecies)->id<<endl;
					curspecies++;

				}

				//Don't give to dying species even if they are champs
				while(((*curspecies)->last_improved()>NEAT::dropoff_age)&&(curspecies!=sorted_species.end()))
					++curspecies;

				while((stolen_babies>0)&&
					(curspecies!=sorted_species.end())) {
						//Randomize a little which species get boosted by a super champ

						if (randfloat()>0.1)
							if (stolen_babies>3) {
								(*(((*curspecies)->organisms).begin()))->super_champ_offspring=3;
								(*curspecies)->expected_offspring+=3;
								stolen_babies-=3;
								//cout<<"Gave 3 babies to Species "<<(*curspecies)->id<<endl;
							}
							else {
								//cout<<"3 or less babies available"<<endl;
								(*(((*curspecies)->organisms).begin()))->super_champ_offspring=stolen_babies;
								(*curspecies)->expected_offspring+=stolen_babies;
								//cout<<"Gave "<<stolen_babies<<" babies to Species "<<(*curspecies)->id<<endl;
								stolen_babies=0;

							}

							curspecies++;

							//Don't give to dying species even if they are champs
							while((curspecies!=sorted_species.end())&&((*curspecies)->last_improved()>NEAT::dropoff_age))
								++curspecies;

					}

					//cout<<"Done giving back babies"<<endl;

					//If any stolen babies aren't taken, give them to species #1's champ
					if (stolen_babies>0) {

						//cout<<"Not all given back, giving to best Species"<<endl;

						curspecies=sorted_species.begin();
						(*(((*curspecies)->organisms).begin()))->super_champ_offspring+=stolen_babies;
						(*curspecies)->expected_offspring+=stolen_babies;
						stolen_babies=0;
					}

	}


	//Kill off all Organisms marked for death.  The remainder
	//will be allowed to reproduce.
	curorg=organisms.begin();
	while(curorg!=organisms.end()) {
		if (((*curorg)->eliminate)) {
			//Remove the organism from its Species
			((*curorg)->species)->remove_org(*curorg);

			//Delete the organism from memory
			delete (*curorg);

			//Remember where we are
			deadorg=curorg;
			++curorg;

			//iter2 =  v.erase(iter); 

			//Remove the organism from the master list
			curorg=organisms.erase(deadorg);

		}
		else {
			++curorg;
		}

	}

	//cout<<"Reproducing"<<endl;

	//Perform reproduction.  Reproduction is done on a per-Species
	//basis.  (So this could be paralellized potentially.)
	//	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {

	//KENHACK                                                                      
	//		for(std::vector<Species*>::iterator curspecies2=species.begin();curspecies2!=species.end();++curspecies2) {
	//		  std::cout<<"PRE in repro specloop SPEC EXISTING number "<<(*curspecies2)->id<<std::endl;
	//	}

	//	(*curspecies)->reproduce(generation,this,sorted_species);


	//}    


	curspecies=species.begin();
	int last_id=(*curspecies)->id;
	while(curspecies!=species.end()) {
	  (*curspecies)->reproduce(generation,this,sorted_species);

	  //Set the current species to the id of the last species checked
	  //(the iterator must be reset because there were possibly vector insertions during reproduce)
	  std::vector<Species*>::iterator curspecies2=species.begin();
	  while(curspecies2!=species.end()) {
	    if (((*curspecies2)->id)==last_id)
	      curspecies=curspecies2;
	    curspecies2++;
	  }

	  //Move to the next on the list
	  curspecies++;
	  
	  //Record where we are
	  if (curspecies!=species.end())
	    last_id=(*curspecies)->id;

	}

	//cout<<"Reproduction Complete"<<endl;


	//Destroy and remove the old generation from the organisms and species  
	curorg=organisms.begin();
	while(curorg!=organisms.end()) {

	  //Remove the organism from its Species
	  ((*curorg)->species)->remove_org(*curorg);

	  //std::cout<<"deleting org # "<<(*curorg)->gnome->genome_id<<std::endl;

	  //Delete the organism from memory
	  delete (*curorg);
	  
	  //Remember where we are
	  deadorg=curorg;
	  ++curorg;
	  
	  //std::cout<<"next org #  "<<(*curorg)->gnome->genome_id<<std::endl;

	  //Remove the organism from the master list
	  curorg=organisms.erase(deadorg);

	  //std::cout<<"nnext org # "<<(*curorg)->gnome->genome_id<<std::endl;

	}

	//Remove all empty Species and age ones that survive
	//As this happens, create master organism list for the new generation
	curspecies=species.begin();
	orgcount=0;
	while(curspecies!=species.end()) {
		if (((*curspecies)->organisms.size())==0) {
			delete (*curspecies);

			deadspecies=curspecies;
			++curspecies;

			curspecies=species.erase(deadspecies);
		}
		//Age surviving Species and 
		//Rebuild master Organism list: NUMBER THEM as they are added to the list
		else {
			//Age any Species that is not newly created in this generation
			if ((*curspecies)->novel) {
				(*curspecies)->novel=false;
			}
			else ++((*curspecies)->age);

			//Go through the organisms of the curspecies and add them to 
			//the master list
			for(curorg=((*curspecies)->organisms).begin();curorg!=((*curspecies)->organisms).end();++curorg) {
				((*curorg)->gnome)->genome_id=orgcount++;
				organisms.push_back(*curorg);
			}
			++curspecies;

		}
	}      

	//Remove the innovations of the current generation
	curinnov=innovations.begin();
	while(curinnov!=innovations.end()) {
		delete (*curinnov);

		deadinnov=curinnov;
		++curinnov;

		curinnov=innovations.erase(deadinnov);
	}

	//DEBUG: Check to see if the best species died somehow
	// We don't want this to happen
	curspecies=species.begin();
	best_ok=false;
	while(curspecies!=species.end()) {
		if (((*curspecies)->id)==best_species_num) best_ok=true;
		++curspecies;
	}
	if (!best_ok) {
		//cout<<"ERROR: THE BEST SPECIES DIED!"<<endl;
	}
	else {
		//cout<<"The best survived: "<<best_species_num<<endl;
	}

	//DEBUG: Checking the top organism's duplicate in the next gen
	//This prints the champ's child to the screen
	for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
		if ((*curorg)->pop_champ_child) {
			//cout<<"At end of reproduction cycle, the child of the pop champ is: "<<(*curorg)->gnome<<endl;
		}
	}

	//cout<<"babies_stolen at end: "<<babies_stolen<<endl;

	//cout<<"Epoch complete"<<endl; 

	return true;

}

bool Population::rank_within_species() {
	std::vector<Species*>::iterator curspecies;

	//Add each Species in this generation to the snapshot
	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		(*curspecies)->rank();
	}

	return true;
}

void Population::estimate_all_averages() {
	std::vector<Species*>::iterator curspecies;

	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		(*curspecies)->estimate_average();
	}

}

Species *Population::choose_parent_species() {  

	double total_fitness=0;
	std::vector<Species*>::iterator curspecies;  
	double marble; //The roulette marble
	double spin; //Spins until the marble reaches its chosen point

	//We can try to keep the number of species constant at this number
	int num_species_target=4;
	int num_species=species.size();
	double compat_mod=0.3;  //Modify compat thresh to control speciation


	//Keeping species diverse
	//This commented out code forces the system to aim for 
	// num_species species at all times, enforcing diversity
	//This tinkers with the compatibility threshold, which
	// normally would be held constant

	//if (num_species<num_species_target)
	//	NEAT::compat_threshold-=compat_mod;
	//else if (num_species>num_species_target)
	//	NEAT::compat_threshold+=compat_mod;

	//if (NEAT::compat_threshold<0.3) NEAT::compat_threshold=0.3;


	//Use the roulette method to choose the species 

	//Sum all the average fitness estimates of the different species
	//for the purposes of the roulette
	for(curspecies=species.begin();curspecies!=species.end();++curspecies) {
		total_fitness+=(*curspecies)->average_est;
	}

	marble=randfloat()*total_fitness;
	curspecies=species.begin();
	spin=(*curspecies)->average_est;
	while(spin<marble) {
		++curspecies;

		//Keep the wheel spinning
		spin+=(*curspecies)->average_est;
	}
	//Finished roulette

	//  cout<<"Chose random species "<<(*curspecies)->id<<endl;
	//printf("Chose random species %d.",(*curspecies)->id);

	//Return the chosen species
	return (*curspecies);
}

bool Population::remove_species(Species *spec) {
	std::vector<Species*>::iterator curspec;

	curspec=species.begin();
	while((curspec!=species.end())&&
		((*curspec)!=spec))
		++curspec;

	if (curspec==species.end()) {
		//   cout<<"ALERT: Attempt to remove nonexistent Species from Population"<<endl;
		return false;
	}
	else {
		species.erase(curspec);
		return true;
	}
}

Organism* Population::remove_worst() {

	double adjusted_fitness;
	double min_fitness=999999;
	std::vector<Organism*>::iterator curorg;
	Organism *org_to_kill = 0;
	std::vector<Organism*>::iterator deadorg;
	Species *orgs_species; //The species of the dead organism

	//Make sure the organism is deleted from its species and the population

	//First find the organism with minimum *adjusted* fitness
	for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
		adjusted_fitness=((*curorg)->fitness)/((*curorg)->species->organisms.size());
		if ((adjusted_fitness<min_fitness)&&
			(((*curorg)->time_alive) >= NEAT::time_alive_minimum))
		{
			min_fitness=adjusted_fitness;
			org_to_kill=(*curorg);
			deadorg=curorg;
			orgs_species=(*curorg)->species;
		}
	}

	if (org_to_kill) {

//		printf("Org to kill: species = %d",org_to_kill->species->id);

		//Remove the organism from its species and the population
		orgs_species->remove_org(org_to_kill);  //Remove from species
		delete org_to_kill;  //Delete the organism itself 
		organisms.erase(deadorg); //Remove from population list

		//Did the species become empty?
		if ((orgs_species->organisms.size())==0) {

			remove_species(orgs_species);
			delete orgs_species;
		}
		//If not, re-estimate the species average after removing the organism
		else {
			orgs_species->estimate_average();
		}
	}

	return org_to_kill;
} 

//Warning: rtNEAT does not behave like regular NEAT if you remove the worst probabilistically
//You really should just use "remove_worst," which removes the org with worst adjusted fitness.
Organism* Population::remove_worst_probabilistic() {

	double adjusted_fitness;
	double min_fitness=999999;
	std::vector<Organism*>::iterator curorg;
	Organism *org_to_kill = 0;
	std::vector<Organism*>::iterator deadorg;
	Species *orgs_species; //The species of the dead organism

	//Make sure the organism is deleted from its species and the population

	std::vector<Organism*> sorted_adjusted_orgs;

	for(curorg = organisms.begin(); curorg != organisms.end(); ++curorg) {
		if ((*curorg)->time_alive >= NEAT::time_alive_minimum) 
			sorted_adjusted_orgs.push_back(*curorg);
	}

	if (sorted_adjusted_orgs.size() == 0)
		return 0;

	//sorted_adjusted_orgs.qsort(order_orgs_by_adjusted_fit);
    std::sort(sorted_adjusted_orgs.begin(), sorted_adjusted_orgs.end(), order_orgs_by_adjusted_fit);

	int size_bottom_10_percent = ceil((float)sorted_adjusted_orgs.size() * 0.10);
	int randorgnum = NEAT::randint(sorted_adjusted_orgs.size() - size_bottom_10_percent, sorted_adjusted_orgs.size() - 1);

	curorg = sorted_adjusted_orgs.begin();
	curorg += randorgnum;
	org_to_kill = *curorg;
	orgs_species=(org_to_kill)->species;

	curorg = organisms.begin();
	while (*curorg != org_to_kill) {
		++curorg;
	}
	deadorg = curorg;

	//First find the organism with minimum *adjusted* fitness
	//for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
	//	adjusted_fitness=((*curorg)->fitness)/((*curorg)->species->organisms.size());
	//	if ((adjusted_fitness<min_fitness)&&
	//		(((*curorg)->time_alive) >= NEAT::time_alive_minimum))
	//	{
	//		min_fitness=adjusted_fitness;
	//		org_to_kill=(*curorg);
	//		deadorg=curorg;
	//		orgs_species=(*curorg)->species;
	//	}
	//}

	//printf("Org to kill time_alive = %d and fitness = %f",org_to_kill->time_alive,org_to_kill->fitness);

	if (org_to_kill) {
		//Remove the organism from its species and the population
		orgs_species->remove_org(org_to_kill);  //Remove from species
		delete org_to_kill;  //Delete the organism itself 
		organisms.erase(deadorg); //Remove from population list

		//Did the species become empty?
		if ((orgs_species->organisms.size())==0) {

			remove_species(orgs_species);
			delete orgs_species;
		}
		//If not, re-estimate the species average after removing the organism
		else {
			orgs_species->estimate_average();
		}
	}

	return org_to_kill;
} 

Organism* Population::reproduce_champ(int generation) {
	std::vector<Organism*>::iterator curorg;
	double max_fitness=0;
	Organism *champ;
	Organism *baby;
	Genome *new_genome;
	std::vector<Species*>::iterator curspecies;
	Species *newspecies;
	Organism *comporg;
	bool found;

	//The weight mutation power is species specific depending on its age
	double mut_power=NEAT::weight_mut_power;
	
		


	champ=*(organisms.begin()); //Make sure at least something is chosen
	//Find the population champ
	for(curorg = organisms.begin(); curorg != organisms.end(); ++curorg) {
		if (((*curorg)->fitness>max_fitness)&&
			((*curorg)->time_alive >= NEAT::time_alive_minimum)){
			champ=(*curorg);
			max_fitness=champ->fitness;
		}
	}

	//Now reproduce the pop champ as a new org	
	new_genome=(champ->gnome)->duplicate(generation);
	
	//Maybe mutate its link weights
	if (randfloat()<NEAT::mutate_link_weights_prob) {  
	//if (randfloat()<0.5) {
			//cout<<"mutate_link_weights"<<endl;
			new_genome->mutate_link_weights(mut_power,1.0,GAUSSIAN);
	}

	baby=new Organism(0.0,new_genome,generation);

		curspecies=(species).begin();
	if (curspecies==(species).end()){
		//Create the first species
		newspecies=new Species(++(last_species),true);
		(species).push_back(newspecies);
		newspecies->add_Organism(baby);  //Add the baby
		baby->species=newspecies;  //Point the baby to its species
	} 
	else {
		comporg=(*curspecies)->first();
		found=false;

		// Testing out what happens when speciation is disabled
		//found = true;
		//(*curspecies)->add_Organism(baby);
		//baby->species = (*curspecies);


		while((curspecies!=(species).end()) && (!found)) 
		{	
			if (comporg==0) {
				//Keep searching for a matching species
				++curspecies;
				if (curspecies!=(species).end())
					comporg=(*curspecies)->first();
			}
			else if (((baby->gnome)->compatibility(comporg->gnome))<NEAT::compat_threshold) {
				//Found compatible species, so add this organism to it
				(*curspecies)->add_Organism(baby);
				baby->species=(*curspecies);  //Point organism to its species
				found=true;  //Note the search is over
			}
			else {
				//Keep searching for a matching species
				++curspecies;
				if (curspecies!=(species).end()) 
					comporg=(*curspecies)->first();
			}
		}

		//If we didn't find a match, create a new species
		if (found==false) {
			newspecies=new Species(++(last_species),true);
			(species).push_back(newspecies);
			newspecies->add_Organism(baby);  //Add the baby
			baby->species=newspecies;  //Point baby to its species
		}

	} //end else     

	//Put the baby also in the master organism list
	(organisms).push_back(baby);


	//printf("CHAMPBABY --- species: %i   fitness: %f   ", champ->species->id, champ->fitness );

	//printf("----------------------------");

	return baby; //Return a pointer to the baby

}

//KEN: New 2/17/04
//This method takes an Organism and reassigns what Species it belongs to
//It is meant to be used so that we can reasses where Organisms should belong
//as the speciation threshold changes.
void Population::reassign_species(Organism *org) {
	
		Organism *comporg;
		bool found=false;  //Note we don't really need this flag but it
		                    //might be useful if we change how this function works
		std::vector<Species*>::iterator curspecies;
		Species *newspecies;
		Species *orig_species;

		curspecies=(species).begin();

		comporg=(*curspecies)->first();
		while((curspecies!=(species).end()) && (!found)) 
		{	
			if (comporg==0) {
				//Keep searching for a matching species
				++curspecies;
				if (curspecies!=(species).end())
					comporg=(*curspecies)->first();
			}
			else if (((org->gnome)->compatibility(comporg->gnome))<NEAT::compat_threshold) {
				//If we found the same species it's already in, return 0
				if ((*curspecies)==org->species) {
					found=true;				
					break;
				}
				//Found compatible species
				else {
					switch_species(org,org->species,(*curspecies));
				    found=true;  //Note the search is over
				}
			}
			else {
				//Keep searching for a matching species
				++curspecies;
				if (curspecies!=(species).end()) 
					comporg=(*curspecies)->first();
			}
		}

		//If we didn't find a match, create a new species, move the org to
		// that species, check if the old species is empty, 
		//re-estimate averages, and return 0
		if (found==false) {

			//Create a new species for the org
			newspecies=new Species(++(last_species),true);
			(species).push_back(newspecies);
		
			switch_species(org,org->species,newspecies);
		}

}

//Move an Organism from one Species to another
void Population::switch_species(Organism *org, Species *orig_species, Species *new_species) {

		//Remove organism from the species we want to remove it from
		orig_species->remove_org(org);

		//Add the organism to the new species it is being moved to
		new_species->add_Organism(org);
		org->species=new_species;

		//KEN: Delete orig_species if empty, and remove it from pop
		if ((orig_species->organisms.size())==0) {

			remove_species(orig_species);
			delete orig_species;

			//Re-estimate the average of the species that now has a new member
			new_species->estimate_average();
		}
		//If not, re-estimate the species average after removing the organism
		// AND the new species with the new member
		else {
			orig_species->estimate_average();
			new_species->estimate_average();
		}
}

//Organism* Population::remove_worst_probabilistic() {
//
//	double adjusted_fitness;
//	double min_fitness=999999;
//	std::vector<Organism*>::iterator curorg;
//	Organism *org_to_kill = 0;
//	std::vector<Organism*>::iterator deadorg;
//	Species *orgs_species; //The species of the dead organism
//
//	// Select a random species from which to remove an organism
//	//std::vector<Species*>::iterator curspecies = species.begin();
//	//int randspecies = NEAT::randint(0, species.size() - 1);
//	//curspecies += randspecies;
//
//	float sum_of_averages = 0;
//
//	for (std::vector<NEAT::Species*>::iterator curspec = species.begin(); curspec != species.end(); ++curspec) {
//		(*curspec)->estimate_average();
//		sum_of_averages += (*curspec)->average_est;
//	}
//
//	float marble = randfloat() * sum_of_averages;
//	std::vector<Species*>::iterator curspecies=species.begin();
//	float spin = (*curspecies)->average_est * (*curspecies)->organisms.size();
//	while(spin<marble) {
//		++curspecies;
//
//		//Keep the wheel spinning
//		spin += (*curspecies)->average_est * (*curspecies)->organisms.size();
//	}
//
//
//	int num_below_min=0; //# orgs below minimum age
//
//	//Make sure the organism is deleted from its species and the population
//
//	//Find how many organisms in this species are below minimum age
//	for(curorg=(*curspecies)->organisms.begin();curorg!=(*curspecies)->organisms.end();++curorg) {
//		if ((*curorg)->time_alive < NEAT::time_alive_minimum) 
//			num_below_min++;
//	}
//	if (num_below_min == (*curspecies)->organisms.size())
//		return 0;
//
//	int speciessize = (*curspecies)->organisms.size() - num_below_min;
//
//	//First find the organism with minimum *adjusted* fitness within the selected species
//	//for(curorg=(*curspecies)->organisms.begin();curorg!=(*curspecies)->organisms.end();++curorg) {
//		//adjusted_fitness=((*curorg)->fitness)/((*curorg)->species->organisms.size());
//	(*curspecies)->organisms.qsort(order_orgs);
//	int size_bottom_10_percent = mCeil((float)speciessize * 0.10);
//	int randorgnum = NEAT::randint(speciessize - size_bottom_10_percent - 1, speciessize - 1);
//	
//	curorg = (*curspecies)->organisms.begin();
//	while (randorgnum > 0) {
//		if ((*curorg)->time_alive >= NEAT::time_alive_minimum)
//			--randorgnum;
//		++curorg;
//	}
//	org_to_kill = *curorg;
//	orgs_species=(org_to_kill)->species;
//		
//		//if ((adjusted_fitness < min_fitness)&&
//		//	(((*curorg)->time_alive) >= NEAT::time_alive_minimum))
//		//{
//		//	min_fitness=adjusted_fitness;
//		//	org_to_kill=(*curorg);
//		//	deadorg=curorg;
//		//	orgs_species=(*curorg)->species;
//		//}
//	//}
//
//	curorg = organisms.begin();
//	while (*curorg != org_to_kill) {
//		++curorg;
//	}
//	deadorg = curorg;
//
//	//Con::printf("Org to kill time_alive = %d and fitness = %f",org_to_kill->time_alive,org_to_kill->fitness);
//
//	if (org_to_kill) {
//		//Remove the organism from its species and the population
//		orgs_species->remove_org(org_to_kill);  //Remove from species
//		delete org_to_kill;  //Delete the organism itself 
//		organisms.erase(deadorg); //Remove from population list
//
//		//Did the species become empty?
//		if ((orgs_species->organisms.size())==0) {
//
//			remove_species(orgs_species);
//			delete orgs_species;
//		}
//		//If not, re-estimate the species average after removing the organism
//		else {
//			orgs_species->estimate_average();
//		}
//	}
//
//	return org_to_kill;
//} 


////Print Population to a file specified by a string   
//bool Population::print_to_file(char *filename){
//std::vector<Organism*>::iterator curorg;
//
//ofstream outFile(filename,ios::out);
//
////Make sure it worked
//if (!outFile) {
//cerr<<"Can't open "<<filename<<" for output"<<endl;
//return false;
//}
//
////Print all the Organisms' Genomes to the outFile
//for(curorg=organisms.begin();curorg!=organisms.end();++curorg) {
//((*curorg)->gnome)->print_to_file(outFile);
////We can confirm by writing the genome #'s to the screen
////cout<<((*curorg)->gnome)->genome_id<<endl;
//}
//
//outFile.close();
//
//return true;
//
//}
