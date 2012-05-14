#include "link.h"

using namespace NEAT;

Link::Link(double w,NNode *inode,NNode *onode,bool recur) {
	weight=w;
	in_node=inode;
	out_node=onode;
	is_recurrent=recur;
	added_weight=0;
	linktrait=0;
	time_delay=false;
	trait_id=1;
}

Link::Link(Trait *lt,double w,NNode *inode,NNode *onode,bool recur) {
	weight=w;
	in_node=inode;
	out_node=onode;
	is_recurrent=recur;
	added_weight=0;
	linktrait=lt;
	time_delay=false;
	if (lt!=0)
		trait_id=lt->trait_id;
	else trait_id=1;
}	

Link::Link(double w) {
	weight=w;
	in_node=out_node=0;  
	is_recurrent=false;
	linktrait=0;
	time_delay=false;
	trait_id=1;
}

Link::Link(const Link& link)
{
	weight = link.weight;
	in_node = link.in_node;
	out_node = link.out_node;
	is_recurrent = link.is_recurrent;
	added_weight = link.added_weight;
	linktrait = link.linktrait;
	time_delay = link.time_delay;
	trait_id = link.trait_id;
}

void Link::derive_trait(Trait *curtrait) {

	if (curtrait!=0) {
		for (int count=0;count<NEAT::num_trait_params;count++)
			params[count]=(curtrait->params)[count];
	}
	else {
		for (int count=0;count<NEAT::num_trait_params;count++)
			params[count]=0;
	}

	if (curtrait!=0)
		trait_id=curtrait->trait_id;
	else trait_id=1;

}
