#include "noveltyset.h"
#include "population.h"
#include "organism.h"

//for sorting by novelty
bool cmp(const noveltyitem *a, const noveltyitem* b)
{
return a->novelty < b->novelty;
}

//for sorting by fitness
bool cmp_fit(const noveltyitem *a, const noveltyitem *b)
{
return a->fitness < b->fitness;
}

noveltyitem::noveltyitem(const noveltyitem& item)
{
	added=item.added;
	genotype=new Genome(*(item.genotype));
	phenotype=new Network(*(item.phenotype));
	age=item.age;
	fitness=item.fitness;
	novelty=item.novelty;
	generation=item.generation;
	indiv_number=item.indiv_number;
	for(int i=0;i<(int)item.data.size();i++)
	{
		vector<float> temp;
		for(int j=0;j<(int)item.data[i].size();j++)
			temp.push_back(item.data[i][j]);
		data.push_back(temp);		
	}
}

//evaluate the novelty of the whole population
void noveltyarchive::evaluate_population(Population* pop,bool fitness)
{
	Population *p = (Population*)pop;
	vector<Organism*>::iterator it;
	for(it=p->organisms.begin();it<p->organisms.end();it++)
		evaluate_individual((*it),pop,fitness);
}

//evaluate the novelty of a single individual
void noveltyarchive::evaluate_individual(Organism* ind,Population* pop,bool fitness)
{
	float result;
	if(fitness)  //assign fitness according to average novelty
	{
		result = novelty_avg_nn(ind->noveltypoint,-1,false,pop);
		ind->fitness = result;
	} 
	else  //consider adding a point to archive based on dist to nearest neighbor
	{
		result = novelty_avg_nn(ind->noveltypoint,1,false);
		ind->noveltypoint->novelty=result;
		if(add_to_novelty_archive(result))
				add_novel_item(ind->noveltypoint);
	}
}
