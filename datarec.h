#ifndef _DATAREC_H_
#define _DATAREC_H_

#include <vector>
#include <fstream>
#include <iostream>

#define RECSIZE 120
using namespace std;

class data_record
{
	public:
	data_record()
	{
		for(int i=0;i<RECSIZE;i++)
			ToRec[i]=-10.0;
		indiv_number=-1;
	}
	float ToRec[RECSIZE];
	int indiv_number;
};

class data_rec 
{
	public:
	vector<data_record*> data;
	
	void serialize(char*fn)
	{
		ofstream output(fn);
		for(int i=0;i<(int)data.size();i++)
		{
			for(int j=0;j<RECSIZE;j++)
			{
				output << data[i]->ToRec[j] << " ";
			}
			output << data[i]->indiv_number << endl;
		}
		output.close();
	}
	
	void add_new(data_record* k)
	{
		data.push_back(k);
	}
	
	~data_rec()
	{

	}
};

#endif
