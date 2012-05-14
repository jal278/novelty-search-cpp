/* Created by Anjuta version 1.2.4a */
/*	This file will not be overwritten */
#ifndef HISTOGRAM
#define HISTOGRAM

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <math.h>

#define realtype float

using namespace std;

//Function for creating a normalized histogram of an input vector given a number of bins
//not currently used
void pdf(vector<realtype> &input, vector<realtype> &bins, vector<realtype> &hist,bool clear=true)
{
	//if clear flag set
	if(clear)
		hist.clear();
	
	int end_index = hist.size();
	int bin_size = bins.size();
	int input_size = input.size();
	int counted = 0;
	//for(int i=0;i<=bin_size;i++)
	//	hist.push_back(0.0);
	
	//roll back modifications to make zero values count
	hist.insert(hist.end(),bin_size+1,0.0);
	//hist.insert(hist.end(),bin_size,0.0);
	
	for(int j=0;j<input_size;j++)
	{
		realtype val=input[j];
		if (val==-1) continue;	
		counted++;			
		int k;
		for(k=end_index;k<end_index+bin_size;k++)
		{
			if(val<bins[k-end_index])
				break;
		}
		//roll back modifications to make zero values count
		//if(k>end_index)
		//	hist[k-1]+=1.0;
		hist[k]+=1.0;
	}
	
	if(input.size()!=0)
	//roll back modifications to make zero values count
	for(int i=end_index;i<=end_index+bin_size;i++)
		hist[i]/=(realtype)input.size();
	//for(int i=end_index;i<end_index+bin_size;i++)	
	
}

//optimize this later
//when you shift the window, instead of recalculating accum completely
//simply subtract the number leaving and add the number arriving
//then divide to get avg
void window_filter(vector<realtype> &input, int winsize, vector<realtype> &res)
{
	res.clear();
	
	int inpSize=input.size();
	for(int i=0;i<=inpSize-winsize;i++)
	{
		realtype accum=0.0;
		//int badcount=0;
		for(int j=i;j<winsize+i;j++)
		{
			if(input[j]==-1) continue;
			accum+=input[j];
		}
		accum/=(realtype)winsize;
		res.push_back(accum);
	}
}

//multiresolution histogram, not used currently
void multires_hist(vector<realtype> &input, vector<realtype> &bins, vector<int> &granularities, vector<realtype> &res)
{
	vector<realtype> accum;
	res.clear();
	for(int i=0;i<(int)granularities.size();i++)
	{
		window_filter(input,granularities[i],accum);
		pdf(accum,bins,res,false);
	}	
}

//store a vector in plain text in file
void write_vect(char *fn,vector<realtype> res)
{
	ofstream output(fn);
	vector<realtype>::iterator it1 = res.begin();
	while(it1!=res.end())
	{
		output << (*it1) << endl;
		it1++;
	}		
	output.close();
}

//calculates item-wise difference between two vectors
realtype hist_diff(vector<realtype> &in1, vector<realtype> &in2)
{
	int size=in1.size();
	
	realtype diff_accum = 0.0;
	for(int x=0;x<size;x++)
	{
		realtype diff = in1[x]-in2[x];
		diff_accum+=abs(diff);
	}
	return diff_accum/size;
}

//test routines
void hist_test()
{
	vector<realtype> inputs;
	vector<realtype> inputs2;
	vector<realtype> inputs3;
	vector<realtype> inputs4;
	vector<realtype> bins;
	vector<realtype> res;
	
	srand(20);
	
	for(int i=1;i<10;i++)
		bins.push_back(0.1*i);
	
	for(int i=0;i<1000;i++)
	{
		inputs.push_back((realtype)rand()/(realtype)RAND_MAX);
		inputs3.push_back((realtype)rand()/(realtype)RAND_MAX);
		inputs2.push_back((cos((realtype)i/100.0)+1.0)/2.0);
		inputs4.push_back((sin((realtype)i/500.0)+1.0)/2.0);
	}
	pdf(inputs,bins,res);
	
	cout << "PDF Test " << endl;

	for(int i=0;i<10;i++)
		cout << res[i] << endl;
	
	cout << "Window Test " << endl;
	cout << "Little Win " << endl;
	
	window_filter(inputs,1,res);
	for(int i=0;i<10;i++)
		cout << res[i] << endl;
	cout << "Big Win " << endl;
	
	window_filter(inputs,50,res);
	for(int i=0;i<10;i++)
		cout << res[i] << endl;
	
	cout << "MultiRes Histogram Test" << endl;
	
	vector<int> grans;
	for(int i=0;i<20;i++)
		grans.push_back(i*5+1);
	
	
	multires_hist(inputs,bins,grans,res);
	
	vector<realtype> res2;
	multires_hist(inputs2,bins,grans,res2);
	
	vector<realtype> res3;
	multires_hist(inputs3,bins,grans,res3);
	
	vector<realtype> res4;
	multires_hist(inputs4,bins,grans,res4);
	
	for(int i=0;i<(int)res.size();i++)
	{
		if(i%10==0)
			cout << i/10 << ":" << endl;
		cout << res[i] << endl;
	}
	
	cout << "Difference Test " << endl;
	cout << hist_diff(res,res2) << endl;
	cout << hist_diff(res,res3) << endl;
	cout << hist_diff(res2,res4) << endl;
	
	std::cout << "End Test" << std::endl;
	
}
#endif
