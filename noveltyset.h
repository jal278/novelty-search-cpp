#ifndef NVSET_H
#define NVSET_H

#include <math.h>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>

#include "population.h"

#define ARCHIVE_SEED_AMOUNT 1

using namespace std;
using namespace NEAT;

//a novelty item is a "stake in the ground" i.e. a novel phenotype
class noveltyitem
{
public:
bool added;
int indiv_number;
//we can keep track of genotype & phenotype of novel item
NEAT::Genome* genotype;
NEAT::Network* phenotype;

//used to collect data
vector< vector<float> > data;

//future use
float age;

//used for analysis purposes
float novelty;
float fitness;
float generation;

//this will write a novelty item to file
bool Serialize(ofstream& ofile)
{
	genotype->print_to_file(ofile);
	SerializeNoveltyPoint(ofile);
	return true;
}

//serialize the novelty point itself to file
bool SerializeNoveltyPoint(ofstream& ofile)
{
	ofile << "/* Novelty: " << novelty << " Fitness: " << fitness << " Generation: " << generation << " Indiv: " << indiv_number << " */" << endl;
	ofile << "/* Point:";
	for(int i=0;i<(int)data.size();i++)
		for(int j=0;j<(int)data[i].size();j++)
			ofile << " " << data[i][j];
	ofile << " */" << endl;
	return true;
}

//copy constructor
noveltyitem(const noveltyitem& item);

//initialize...
noveltyitem()
{
	added=false;
	genotype=NULL;
	phenotype=NULL;
	age=0.0;
	generation=0.0;
	indiv_number=(-1);
}

~noveltyitem()
{
	if(genotype)
		delete genotype;
	if(phenotype)
		delete phenotype;
}


};

//different comparison functions used for sorting
bool cmp(const noveltyitem *a, const noveltyitem *b);
bool cmp_fit(const noveltyitem *a, const noveltyitem *b);
 

//the novelty archive contains all of the novel items we have encountered thus far
//Using a novelty metric we can determine how novel a new item is compared to everything
//currently in the novelty set 
class noveltyarchive
{

private:
	//are we collecting data?
	bool record;

	ofstream *datafile;
	ofstream *novelfile;
	typedef pair<float, noveltyitem*> sort_pair;
	//all the novel items we have found so far
	vector<noveltyitem*> novel_items;
	vector<noveltyitem*> fittest;

	//current generation novelty items
	vector<noveltyitem*> current_gen;

	//novel items waiting addition to the set pending the end of the generation 
	vector<noveltyitem*> add_queue;
	//the measure of novelty
	float (*novelty_metric)(noveltyitem*,noveltyitem*);
	//minimum threshold for a "novel item"
	float novelty_threshold;
	float novelty_floor;
	//counter to keep track of how many gens since we've added to the archive
	int time_out;
	//parameter for how many neighbors to look at for N-nearest neighbor distance novelty
	int neighbors;
	//radius for SOG-type (not currently used)
	float radius;
	int this_gen_index;
	
	//hall of fame mode, add an item each generation regardless of threshold
	bool hall_of_fame;
	//add new items according to threshold
	bool threshold_add;
	
	//current generation
	int generation;
public:

	//constructor
	noveltyarchive(float threshold,float (*nm)(noveltyitem*,noveltyitem*),bool rec=true)
	{
		//how many nearest neighbors to consider for calculating novelty score?
		neighbors=15;
		generation=0;
		time_out=0; //used for adaptive threshold
		novelty_threshold=threshold;
		novelty_metric=nm; //set the novelty metric via function pointer
		novelty_floor=0.25; //lowest threshold is allowed to get
		record=rec;
		this_gen_index=ARCHIVE_SEED_AMOUNT;
		hall_of_fame=false;
		threshold_add=true;
		
		if(record)
		{
			datafile = new ofstream("runresults.dat");
		}
	}

	~noveltyarchive()
	{
		if(record)
		{
			datafile->close();
		}
		//probably want to delete all the noveltyitems at this point
	}
	
public:
	float get_threshold() { return novelty_threshold; }
	int get_set_size()
	{
		return (int)novel_items.size();
	}
	
	//add novel item to archive
	void add_novel_item(noveltyitem* item,bool aq=true)
	{
		item->added=true;
		item->generation=generation;
		novel_items.push_back(item);
		if(aq)
			add_queue.push_back(item);
	}

	#define MIN_ACCEPTABLE_NOVELTY 0.005
	//not currently used
	void add_randomly(Population* pop)
	{
		for(int i=0;i<(int)pop->organisms.size();i++)
		{
			if (((float)rand()/RAND_MAX)<(0.0005))
			{
				noveltyitem* newitem = new noveltyitem(*pop->organisms[i]->noveltypoint);
				if(newitem->novelty > MIN_ACCEPTABLE_NOVELTY)
					add_novel_item(newitem,false);
				else delete newitem;
			}
		}
	}
	
	noveltyitem *get_item(int i) { return novel_items[i]; }
	
	//re-evaluate entire population for novelty
	void evaluate_population(Population* pop,bool fitness=true);
	//evaluate single individual for novelty
	void evaluate_individual(Organism* individual,Population* pop,bool fitness=true);

	//maintain list of fittest organisms so far
	void update_fittest(Organism* org)
	{
		int allowed_size=5;
		if((int)fittest.size()<allowed_size)
		{
			if(org->noveltypoint!=NULL)
			{
			noveltyitem* x = new noveltyitem(*(org->noveltypoint));
			fittest.push_back(x);
			sort(fittest.begin(),fittest.end(),cmp_fit);
			reverse(fittest.begin(),fittest.end());
			}
			else
			{
				cout<<"WHY NULL?" << endl;
			}
		}			
		else
		{
			if(org->noveltypoint->fitness > fittest.back()->fitness)
			{
				noveltyitem* x = new noveltyitem(*(org->noveltypoint));
				fittest.push_back(x);
				
				sort(fittest.begin(),fittest.end(),cmp_fit);
				reverse(fittest.begin(),fittest.end());
				
				delete fittest.back();
				fittest.pop_back();
				
			}
		}
	}
	
	//resort fittest list
	void update_fittest(Population* pop)
	{

		sort(fittest.begin(),fittest.end(),cmp_fit);
		reverse(fittest.begin(),fittest.end());
		
	}
	
	//write out fittest list
	void serialize_fittest(char *fn)
	{
		ofstream outfile(fn);
		for(int i=0;i<(int)fittest.size();i++)
			fittest[i]->Serialize(outfile);
		outfile.close();
	}
	
	//adjust dynamic novelty threshold depending on how many have been added to
	//archive recently
	void add_pending()
	{
		if(record)
		{
			(*datafile) << novelty_threshold << " " << add_queue.size() << endl;
		}
		
		if(hall_of_fame) 
		{		
		if(add_queue.size()==1) time_out++;
		else time_out=0;
		}
		else 
		{
		if(add_queue.size()==0)	time_out++;
		else time_out=0;
		}
		
		//if no individuals have been added for 10 generations
		//lower threshold
		if(time_out==10) {
			novelty_threshold*=0.95;
			if(novelty_threshold<novelty_floor)
				novelty_threshold=novelty_floor;
			time_out=0;
		}
		
		//if more than four individuals added this generation
		//raise threshold
		if(add_queue.size()>4) novelty_threshold*=1.2;
		
		add_queue.clear();
		
		this_gen_index = novel_items.size();
	}
	
	//criteria for adding to the archive
	bool add_to_novelty_archive(float novelty)
	{
		if(novelty>novelty_threshold)
			return true;
		else
			return false;
	}
	
	//only used in generational model (obselete)
	void end_of_gen()
	{
		generation++;
		
			
			if(threshold_add)
			{
				find_novel_items(true);
			}
				
			if(hall_of_fame)
			{
				find_novel_items(false);
				
				sort(current_gen.begin(),current_gen.end(),cmp);
				reverse(current_gen.begin(),current_gen.end());
			
				add_novel_item(current_gen[0]);
			}
			
			clean_gen();
			
		
		add_pending();
	}
	
	//steady-state end of generation call (every so many indivudals)
	void end_of_gen_steady(Population* pop)
	{
		
		generation++;
		
		add_pending();
		
		vector<Organism*>::iterator cur_org;
	}
	
	void clean_gen()
	{
		vector<noveltyitem*>::iterator cur_item;

		bool datarecord=true;	
		
		stringstream filename("");
		filename << "novrec/out" << generation << ".dat";
		ofstream outfile(filename.str().c_str());
		cout << filename.str() << endl;

		for(cur_item=current_gen.begin();cur_item!=current_gen.end();cur_item++)
		{
			if(datarecord)
			{
				(*cur_item)->SerializeNoveltyPoint(outfile);
			}
			if(!(*cur_item)->added)
				delete (*cur_item);
		}
		current_gen.clear();
	}
	
	//see if there are any individuals in current generation
	//that need to be added to the archive (obselete)
	void find_novel_items(bool add=true)
	{
		vector<noveltyitem*>::iterator cur_item;
		for(cur_item=current_gen.begin();cur_item!=current_gen.end();cur_item++)
		{
			float novelty = test_novelty((*cur_item));
			(*cur_item)->novelty = novelty;
			if(add && add_to_novelty_archive(novelty))
				add_novel_item(*cur_item);
		}
	}
	
	//add an item to current generation (obselete)
	void add_to_generation(noveltyitem* item)
	{
		current_gen.push_back(item);
	}
	
	//nearest neighbor novelty score calculation
	float novelty_avg_nn(noveltyitem* item,int neigh=-1,bool ageSmooth=false,Population* pop=NULL)
	{
		vector<sort_pair> novelties;
		if(pop)
			novelties = map_novelty_pop(novelty_metric,item,pop);
		else
			novelties = map_novelty(novelty_metric,item);
		sort(novelties.begin(),novelties.end());
			
		float density=0.0;
		int len=novelties.size();
		float sum=0.0;
		float weight=0.0;
		
		
		if(neigh==-1)
		{
			neigh=neighbors;
		}
		
		if(len<ARCHIVE_SEED_AMOUNT)
		{		
			item->age=1.0;
			add_novel_item(item);	
		}
		else
		{
			len=neigh;
			if((int)novelties.size()<len)
				len=novelties.size();
			int i=0;
			
			while(weight<neigh && i<(int)novelties.size())
			{
				float term = novelties[i].first;
				float w = 1.0;
				
				if(ageSmooth)
				{
					float age=(novelties[i].second)->age;
					w=1.0-pow((float)0.95,age);
				}
				
				sum+=term*w;
				weight+=w;		
				i++;				
			}

			if(weight!=0)
			{
					density = sum/weight; 
			}
		}	
		
		item->novelty=density;
		item->generation=generation;
		return density;
	}

	//fitness = avg distance to k-nn in novelty space
	float test_fitness(noveltyitem* item)
	{
		return novelty_avg_nn(item,-1,false);
	}
	
	float test_novelty(noveltyitem* item)
	{
		return novelty_avg_nn(item,1,false);
	}

	//map the novelty metric across the archive
	vector<sort_pair> map_novelty(float (*nov_func)(noveltyitem*,noveltyitem*),noveltyitem* newitem)
	{
		vector<sort_pair> novelties;
		for(int i=0;i<(int)novel_items.size();i++)
		{			
			novelties.push_back(make_pair((*novelty_metric)(novel_items[i],newitem),novel_items[i])); 
		}
		return novelties;
	}
	
	//map the novelty metric across the archive + current population
	vector<sort_pair> map_novelty_pop(float (*nov_func)(noveltyitem*,noveltyitem*),noveltyitem* newitem, Population* pop)
	{
		vector<sort_pair> novelties;
		for(int i=0;i<(int)novel_items.size();i++)
		{
			novelties.push_back(make_pair((*novelty_metric)(novel_items[i],newitem),novel_items[i]));
		}
		for(int i=0;i<(int)pop->organisms.size();i++)
		{
			novelties.push_back(make_pair((*novelty_metric)(pop->organisms[i]->noveltypoint,newitem),
				pop->organisms[i]->noveltypoint));
		}
		return novelties;
	}
	
	//write out archive
	bool Serialize(char* fname)
	{
		ofstream outfile;
		outfile.open(fname);
		bool res= Serialize(outfile);
		outfile.close();
		return res;
	}
	
	//write out archive
	bool Serialize(ofstream& ofile)
	{
		for(int i=0;i<(int)novel_items.size();i++)
		{
			novel_items[i]->Serialize(ofile);
		}
		return true;		
	}

};


#endif
