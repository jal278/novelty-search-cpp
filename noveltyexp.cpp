#include "experiments.h"
#include "noveltyset.h"

#include "datarec.h"
#include "maze.h"

#include "histogram.h"

//#define DEBUG_OUTPUT 1
#include <algorithm>
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;

static char output_dir[30]="";
static Environment* env;
static int param=-1;

//used for discretization studies
double discretize(double x,long bins,double low, double high)
{
	double norm = x-low;
	double binsize = (high-low)/bins;
	int bin = (int)(norm/binsize);
	if(bin==bins)
		bin--;
        double result = (double)binsize*bin+binsize/2.0+low;
	return result;
}

long powerof2(int num)
{
long x=1;
if(num==0) return 1;
for(int y=0;y<num;y++)
	x*=2;
return x;
}

//novelty metric for maze simulation
float maze_novelty_metric(noveltyitem* x,noveltyitem* y)
{
	float diff = 0.0;
	for(int k=0;k<(int)x->data.size();k++) 
	{
		diff+=hist_diff(x->data[k],y->data[k]);
	}
	return diff;
}

//fitness simulation of maze navigation
Population *maze_fitness_realtime(char* outputdir,const char *mazefile,int par) {
    Population *pop;
    Genome *start_genome;
    char curword[20];
    int id;
	
	//create new maze environment
	env=new Environment(mazefile);
	if(outputdir!=NULL) strcpy(output_dir,outputdir);
	param=par;
	
	//starter gene file
    ifstream iFile("mazestartgenes",ios::in);
	
    cout<<"START MAZE NAVIGATOR FITNESS REAL-TIME EVOLUTION VALIDATION"<<endl;

    cout<<"Reading in the start genome"<<endl;
    //Read in the start Genome
    iFile>>curword;
    iFile>>id;
    cout<<"Reading in Genome id "<<id<<endl;
    start_genome=new Genome(id,iFile);
    iFile.close();

    cout<<"Start Genome: "<<start_genome<<endl;

    //Spawn the Population from starter gene
    cout<<"Spawning Population off Genome"<<endl;
    pop=new Population(start_genome,NEAT::pop_size);
      
    cout<<"Verifying Spawned Pop"<<endl;
    pop->verify();
      
    //Start the evolution loop using rtNEAT method calls 
    maze_fitness_realtime_loop(pop);

	//clean up
	delete env;
    return pop;
}

//actual rtNEAT loop for fitness run of maze navigation
int maze_fitness_realtime_loop(Population *pop) {
  bool firstflag=false; //indicates whether maze has been solved yet
  int indiv_counter=0;
  vector<Organism*>::iterator curorg;
  vector<Species*>::iterator curspecies;
  vector<Species*>::iterator curspec; //used in printing out debug info                                                         
  vector<Species*> sorted_species;  //Species sorted by max fit org in Species                                                  

  data_rec Record; //holds run information
  int count=0;
  int pause;
  
  //Real-time evolution variables                                                                                             
  int offspring_count;
  Organism *new_org;

  //We try to keep the number of species constant at this number                                                    
  int num_species_target=NEAT::pop_size/15;
  
  //This is where we determine the frequency of compatibility threshold adjustment
  int compat_adjust_frequency = NEAT::pop_size/20;
  if (compat_adjust_frequency < 1)
    compat_adjust_frequency = 1;

  //Initially, we evaluate the whole population                                                                               
  //Evaluate each organism on a test                                                                                          
  for(curorg=(pop->organisms).begin();curorg!=(pop->organisms).end();++curorg) {

    //shouldn't happen                                                                                                        
    if (((*curorg)->gnome)==0) {
      cout<<"ERROR EMPTY GEMOME!"<<endl;
      cin>>pause;
    }

	//map the novelty point to each individual (this runs the maze simulation)
	(*curorg)->noveltypoint = maze_novelty_map((*curorg));
	(*curorg)->noveltypoint->indiv_number = indiv_counter;
	indiv_counter++;
	(*curorg)->fitness = (*curorg)->noveltypoint->fitness;
    
  }

  //Get ready for real-time loop
  //Rank all the organisms from best to worst in each species
  pop->rank_within_species();                                                                            

  //Assign each species an average fitness 
  //This average must be kept up-to-date by rtNEAT in order to select species probabailistically for reproduction
  pop->estimate_all_averages();

  cout <<"Entering real time loop..." << endl;

  //Now create offspring one at a time, testing each offspring,                                                               
  // and replacing the worst with the new offspring if its better

  //run for 2000 generations (250*2000 = 500,000 evaluations)
  for 
(offspring_count=0;offspring_count<NEAT::pop_size*2001;offspring_count++) 
{
    
    if(offspring_count % (NEAT::pop_size*NEAT::print_every) == 0 )
	{
			cout << offspring_count << endl;
			char fname[30];
			sprintf(fname,"%sfit_rtgen_%d",output_dir,offspring_count/250);
			pop->print_to_file_by_species(fname);
	}
	
    //Every pop_size reproductions, adjust the compat_thresh to better match the num_species_targer
    //and reassign the population to new species                                              
    if (offspring_count % compat_adjust_frequency == 0) {
		count++;
		#ifdef DEBUG_OUTPUT
		cout << "Adjusting..." << endl;
		#endif
		
			
      int num_species = pop->species.size();
      double compat_mod=0.1;  //Modify compat thresh to control speciation                                                     

      // This tinkers with the compatibility threshold 
      if (num_species < num_species_target) {
	NEAT::compat_threshold -= compat_mod;
      }
      else if (num_species > num_species_target)
	NEAT::compat_threshold += compat_mod;

      if (NEAT::compat_threshold < 0.3)
	NEAT::compat_threshold = 0.3;
		#ifdef DEBUG_OUTPUT
      cout<<"compat_thresh = "<<NEAT::compat_threshold<<endl;
		#endif
      //Go through entire population, reassigning organisms to new species                                                  
      for (curorg = (pop->organisms).begin(); curorg != pop->organisms.end(); ++curorg) {
	pop->reassign_species(*curorg);
      }
    }
    

    //For printing only
	#ifdef DEBUG_OUTPUT
    for(curspec=(pop->species).begin();curspec!=(pop->species).end();curspec++) {
      cout<<"Species "<<(*curspec)->id<<" size"<<(*curspec)->organisms.size()<<" average= "<<(*curspec)->average_est<<endl;
    }

    cout<<"Pop size: "<<pop->organisms.size()<<endl;
	#endif
    //Here we call two rtNEAT calls: 
    //1) choose_parent_species() decides which species should produce the next offspring
    //2) reproduce_one(...) creates a single offspring fromt the chosen species
    new_org=(pop->choose_parent_species())->reproduce_one(offspring_count,pop,pop->species);

    #ifdef DEBUG_OUTPUT
	cout<<"Evaluating new baby: "<<endl;
	#endif
	
	//create record for new individual
	data_record *newrec = new data_record();
	newrec->indiv_number=indiv_counter;
	//evaluate new individual
	new_org->noveltypoint = maze_novelty_map(new_org,newrec);
	new_org->noveltypoint->indiv_number = indiv_counter;
	//grab novelty
	newrec->ToRec[RECSIZE-2]=new_org->noveltypoint->novelty;
	//set organism's fitness 
	new_org->fitness = new_org->noveltypoint->fitness;
	indiv_counter++;
	
	#ifdef DEBUG_OUTPUT
	cout << "Fitness: " << new_org->fitness << endl;
	cout << "Novelty: " << new_org->noveltypoint->novelty << endl;
	cout << "RFit: " << new_org->noveltypoint->fitness << endl;
	#endif
	
	//add record of new individual to storage
	Record.add_new(newrec);
	
    //Now we reestimate the baby's species' fitness
    new_org->species->estimate_average();

    //Remove the worst organism                                                                                               
    pop->remove_worst();
	
	//store first solution organism
	if(!firstflag && newrec->ToRec[3]>0.0) {
		firstflag=true;
		char filename[30];
		sprintf(filename,"%sfit_rtgen_first",output_dir);
		pop->print_to_file_by_species(filename);
		cout << "Maze solved by indiv# " << indiv_counter << endl;	
	}
  	
  }
  
  //finish up, write out the record and the final generation
	cout << "COMPLETED..." << endl;
    char filename[30];
    sprintf(filename,"%srecord.dat",output_dir);
    Record.serialize(filename);
	sprintf(filename,"%sfit_rtgen_final",output_dir);
	pop->print_to_file_by_species(filename);
  
   return 0;
}

//novelty maze navigation run
Population *maze_novelty_realtime(char* outputdir,const char* mazefile,int par) {
	
    Population *pop;
    Genome *start_genome;
    char curword[20];
    int id;

	//create new maze environment
	env=new Environment(mazefile);
	param=par;
	if(outputdir!=NULL) strcpy(output_dir,outputdir);
		
	//starter genes file
    ifstream iFile("mazestartgenes",ios::in);
	
    cout<<"START MAZE NAVIGATOR NOVELTY REAL-TIME EVOLUTION VALIDATION"<<endl;

    cout<<"Reading in the start genome"<<endl;
    //Read in the start Genome
    iFile>>curword;
    iFile>>id;
    cout<<"Reading in Genome id "<<id<<endl;
    start_genome=new Genome(id,iFile);
    iFile.close();

    cout<<"Start Genome: "<<start_genome<<endl;

    //Spawn the Population from starter gene
    cout<<"Spawning Population off Genome"<<endl;
    pop=new Population(start_genome,NEAT::pop_size);
      
    cout<<"Verifying Spawned Pop"<<endl;
    pop->verify();
      
    //Start the evolution loop using rtNEAT method calls 
    maze_novelty_realtime_loop(pop);

	//clean up
	delete env;
    return pop;
}

//actual rtNEAT loop for novelty maze navigation runs
int maze_novelty_realtime_loop(Population *pop) {
	bool firstflag=false; //indicates whether the maze has been solved yet
	
  vector<Organism*>::iterator curorg;
  vector<Species*>::iterator curspecies;
  vector<Species*>::iterator curspec; //used in printing out debug info                                                         

  vector<Species*> sorted_species;  //Species sorted by max fit org in Species 

   float archive_thresh=6.0; //initial novelty threshold

  //archive of novel behaviors
  noveltyarchive archive(archive_thresh,*maze_novelty_metric);
	
  data_rec Record; //stores run information
	
  int count=0;
  int pause;

  //Real-time evolution variables                                                                                             
  int offspring_count;
  Organism *new_org;

  //We try to keep the number of species constant at this number                                                    
  int num_species_target=NEAT::pop_size/15;
  
  //This is where we determine the frequency of compatibility threshold adjustment
  int compat_adjust_frequency = NEAT::pop_size/20;
  if (compat_adjust_frequency < 1)
    compat_adjust_frequency = 1;

  //Initially, we evaluate the whole population                                                                               
  //Evaluate each organism on a test                   
  int indiv_counter=0;  
  for(curorg=(pop->organisms).begin();curorg!=(pop->organisms).end();++curorg) {

    //shouldn't happen                                                                                                        
    if (((*curorg)->gnome)==0) {
      cout<<"ERROR EMPTY GEMOME!"<<endl;
      cin>>pause;
    }

	//evaluate each individual
	(*curorg)->noveltypoint = maze_novelty_map((*curorg));
	(*curorg)->noveltypoint->indiv_number=indiv_counter;
	indiv_counter++;
  }

  //assign fitness scores based on novelty
  archive.evaluate_population(pop,true);
  //add to archive
  archive.evaluate_population(pop,false);
  
  //Get ready for real-time loop
  //Rank all the organisms from best to worst in each species
  pop->rank_within_species();                                                                            

  //Assign each species an average fitness 
  //This average must be kept up-to-date by rtNEAT in order to select species probabailistically for reproduction
  pop->estimate_all_averages();

  cout <<"Entering real time loop..." << endl;

  //Now create offspring one at a time, testing each offspring,                                                               
  // and replacing the worst with the new offspring if its better
  for 
(offspring_count=0;offspring_count<NEAT::pop_size*2000;offspring_count++) 
{
	//only continue past generation 1000 if not yet solved
	if(offspring_count>=pop_size*1000 && firstflag)
		break;
	
	
	//end of generation
    if(offspring_count % (NEAT::pop_size*1) == 0)
	{
			archive.end_of_gen_steady(pop);
			//archive.add_randomly(pop);
			archive.evaluate_population(pop,false);
			cout << "ARCHIVE SIZE:" << 
			archive.get_set_size() << endl;
	}

	//write out current generation and fittest individuals
    if( offspring_count % (NEAT::pop_size*NEAT::print_every) == 0 )
	{
        	cout << offspring_count << endl;
			char fname[30];
			
			sprintf(fname,"%sfittest_%d",output_dir,offspring_count/250);
			archive.serialize_fittest(fname);

			sprintf(fname,"%srtgen_%d",output_dir,offspring_count/250);
			pop->print_to_file_by_species(fname);
	}
	
    //Every pop_size reproductions, adjust the compat_thresh to better match the num_species_targer
    //and reassign the population to new species                                              
    if (offspring_count % compat_adjust_frequency == 0) {
		count++;
		#ifdef DEBUG_OUTPUT
		cout << "Adjusting..." << endl;
		#endif
	
	   //update fittest individual list		
	   archive.update_fittest(pop);
	   //refresh generation's novelty scores
	   archive.evaluate_population(pop,true);
			
      int num_species = pop->species.size();
      double compat_mod=0.1;  //Modify compat thresh to control speciation                                                     

      // This tinkers with the compatibility threshold 
      if (num_species < num_species_target) {
	NEAT::compat_threshold -= compat_mod;
      }
      else if (num_species > num_species_target)
	NEAT::compat_threshold += compat_mod;

      if (NEAT::compat_threshold < 0.3)
	NEAT::compat_threshold = 0.3;
		#ifdef DEBUG_OUTPUT
      cout<<"compat_thresh = "<<NEAT::compat_threshold<<endl;
		#endif
	  
      //Go through entire population, reassigning organisms to new species                                                  
      for (curorg = (pop->organisms).begin(); curorg != pop->organisms.end(); ++curorg) {
			pop->reassign_species(*curorg);
      }
    }
    

    //For printing only
	#ifdef DEBUG_OUTPUT
    for(curspec=(pop->species).begin();curspec!=(pop->species).end();curspec++) {
      cout<<"Species "<<(*curspec)->id<<" size"<<(*curspec)->organisms.size()<<" average= "<<(*curspec)->average_est<<endl;
    }

    cout<<"Pop size: "<<pop->organisms.size()<<endl;
	#endif
	
    //Here we call two rtNEAT calls: 
    //1) choose_parent_species() decides which species should produce the next offspring
    //2) reproduce_one(...) creates a single offspring fromt the chosen species
    new_org=(pop->choose_parent_species())->reproduce_one(offspring_count,pop,pop->species);

    //Now we evaluate the new individual
    //Note that in a true real-time simulation, evaluation would be happening to all individuals at all times.
    //That is, this call would not appear here in a true online simulation.
	#ifdef DEBUG_OUTPUT
    cout<<"Evaluating new baby: "<<endl;
	#endif
	
	data_record* newrec=new data_record();
	newrec->indiv_number=indiv_counter;
	//evaluate individual, get novelty point
	new_org->noveltypoint = maze_novelty_map(new_org,newrec);
	new_org->noveltypoint->indiv_number = indiv_counter;
	//calculate novelty of new individual
	archive.evaluate_individual(new_org,pop);
	newrec->ToRec[4] = archive.get_threshold();
	newrec->ToRec[5] = archive.get_set_size();
	newrec->ToRec[RECSIZE-2] = new_org->noveltypoint->novelty;
	
	//add record of new indivdual to storage
	Record.add_new(newrec);
	indiv_counter++;
	
	//update fittest list
	archive.update_fittest(new_org);
	#ifdef DEBUG_OUTPUT
	cout << "Fitness: " << new_org->fitness << endl;
	cout << "Novelty: " << new_org->noveltypoint->novelty << endl;
	cout << "RFit: " << new_org->noveltypoint->fitness << endl;
    #endif
	
    //Now we reestimate the baby's species' fitness
    new_org->species->estimate_average();

	//write out the first individual to solve maze
	if(!firstflag && newrec->ToRec[3]>0.0) {
		firstflag=true;
		char filename[30];
		sprintf(filename,"%srtgen_first",output_dir);
		pop->print_to_file_by_species(filename);
		cout << "Maze solved by indiv# " << indiv_counter << endl;	
		break;
	}

    //Remove the worst organism                                                                                               
    pop->remove_worst();
	

  }
  
  //write out run information, archive, and final generation
  cout << "COMPLETED...";
  char filename[30];
  sprintf(filename,"%srecord.dat",output_dir);
  char fname[30];
  sprintf(fname,"%srtarchive.dat",output_dir);
  archive.Serialize(fname);
  Record.serialize(filename);
  
  sprintf(fname,"%sfittest_final",output_dir);
  archive.serialize_fittest(fname);

  sprintf(fname,"%srtgen_final",output_dir);
  pop->print_to_file_by_species(fname);

  return 0;
}
  
//initialize the maze simulation
Environment* mazesimIni(Environment* tocopy,Network *net, vector< vector<float> > &dc)
  {
    double inputs[20];
    Environment *newenv= new Environment(*tocopy);
	 
	//flush the neural net
	net->flush();
	//update the maze
	newenv->Update();
	//create neural net inputs
	newenv->generate_neural_inputs(inputs);
	//load into neural net
	net->load_sensors(inputs);
	
	//propogate input through net
    for(int i=0;i<10;i++)
		net->activate();
	
	return newenv;
  }
  
  //execute a timestep of the maze simulation evaluation
  double mazesimStep(Environment* newenv,Network *net,vector< vector<float> > &dc)
  {
	  double inputs[20];
	  
	  
		newenv->generate_neural_inputs(inputs);
		net->load_sensors(inputs);
		net->activate();
	
	  	//use the net's outputs to change heading and velocity of navigator
		newenv->interpret_outputs(net->outputs[0]->activation,net->outputs[1]->activation);
	  	//update the environment
		newenv->Update();
	  
	  double dist = newenv->distance_to_target();
	  if(dist<=1) dist=1;
	  double fitness = 5.0/dist; //used for accumulated fitness (obselete)
	  
	  return fitness;
  }
double mazesim(Network* net, vector< vector<float> > &dc, data_record *record)
{
	vector<float> data;
	
	int timesteps=400;
	int stepsize=1000;
	
	double fitness=0.0;
	Environment *newenv;
	newenv=mazesimIni(env,net,dc);
	
	//data collection vector initialization
	dc.clear();
	dc.push_back(data);
	dc[0].reserve(param*2+20);
	
	/*ENABLE FOR ADDT'L INFO STUDIES*/
	/*
	if(param>0)
	{
		stepsize=timesteps/param;
	}
	/* */
	
	for(int i=0;i<timesteps;i++)
	{
		fitness+=mazesimStep(newenv,net,dc);
		//if taking additional samples, collect during run
		if((timesteps-i-1)%stepsize==0)
		{
				dc[0].push_back(newenv->hero.location.x);
				dc[0].push_back(newenv->hero.location.y);		
		}
	}

	//calculate fitness of individual as closeness to target
	fitness=300.0 - newenv->distance_to_target();
	if(fitness<0.1) fitness=0.1;
	
	//fitness as novelty studies
	//data.push_back(fitness);
	
	float x=newenv->hero.location.x;
	float y=newenv->hero.location.y;
	
	/* ENABLE FOR DISCRETIZATION STUDIES
	if(param>0)
	{
	 long bins=powerof2(param);
	 x=discretize(x,bins,0.0,200.0);
	 y=discretize(y,bins,0.0,200.0);
	}
	*/
	
	if(param<=0)
	{
	//novelty point is the ending location of the navigator
	dc[0].push_back(x);
	dc[0].push_back(y);
	}
		
	if(record!=NULL)
	{
		record->ToRec[0]=fitness;
		record->ToRec[1]=newenv->hero.location.x;
		record->ToRec[2]=newenv->hero.location.y;
		record->ToRec[3]=newenv->reachgoal;
		
		/*ADDTL INFO RECORDING */
		if(param>=0)
		{
			for(int x=0;x<(int)dc[0].size();x++)
			{
				if((10+x)<RECSIZE)
					record->ToRec[10+x]=dc[0][x];
			}
		}
		/*ADDTL INFO RECORDING */
	}

	delete newenv;
	return fitness;
}

//evaluates an individual and stores the novelty point
noveltyitem* maze_novelty_map(Organism *org,data_record* record)
{

  noveltyitem *new_item = new noveltyitem;
  new_item->genotype=new Genome(*org->gnome);
  new_item->phenotype=new Network(*org->net);
  vector< vector<float> > gather;

  double fitness;
  static float highest_fitness=0.0;

	fitness=mazesim(org->net,gather,record);
  	if(fitness>highest_fitness)
		highest_fitness=fitness;
	
	//keep track of highest fitness so hard in record
	if(record!=NULL)
	{
		/*
	record->ToRec[19]=org->gnome->parent1;
	record->ToRec[18]=org->gnome->parent2;
	record->ToRec[17]=org->gnome->struct_change;
		*/
	record->ToRec[RECSIZE-1]=highest_fitness;
	}

 	 //push back novelty characterization
	 new_item->data.push_back(gather[0]);
 	 //set fitness (this is 'real' objective-based fitness, not novelty)
 	 new_item->fitness=fitness;
  return new_item;
}
