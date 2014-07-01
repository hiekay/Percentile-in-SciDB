#include "BST.h"
#include <math.h>

#define delta 1e-6


//public functions
	

	void BST::insert(const double key)
	{
		insert(key, root);
	}

	void BST::remove(const double key) 
	{
		remove(key, root);
	}

	double BST::select(int k)
	{
		return select(k, root);
	}

	void BST::removeAll()
	{
		removeAll(root);
	}

	int BST::getSize()			//return the size of the whole tree
	{
		return root->size;
	}


//private functions
	
	int BST::getSize(pNode &rt)
	{
		if (!rt)
			return 0;
		else 
			return rt->size;
	}



	void BST::insert(const double key, pNode &rt)
	{
		if (!rt) {
			rt = new node();
			rt->value = key;
			rt->number = rt->size = 1;
			rt->L = rt->R = NULL;
			return;
		}
		if (fabs(rt->value - key) < delta) {			//rt->value == key
			rt->number++;
			rt->size++;
			return;
		}
		if (key < rt->value) {
			insert(key, rt->L);
			if (rt->L->weight < rt->weight)			//maintain heap feature
				leftRotate(rt);
		}
		else {
			insert(key, rt->R);
			if (rt->R->weight < rt->weight)			//maintain heap feature
				rightRotate(rt);
		}
		rt->size = getSize(rt->L) + getSize(rt->R) + rt->number; 	//update the subtree size.
	}

	void BST::remove(const double key, pNode &rt) 
	{
		if (!rt)
			return;
		if (fabs(rt->value - key) < delta) {		//rt->value == key
			if (rt->number > 0) {
				rt->number--;
				rt->size--;
			}
			return;
		}
		if (key < rt->value) 
			remove(key, rt->L);
		else 
			remove(key, rt->R);
		rt->size = getSize(rt->L) + getSize(rt->R) + rt->number;
	}

	double BST::select(int k, pNode &rt)
	{
		int lSize = getSize(rt->L);
		if (k > lSize && k <= lSize + rt->number)
			return rt->value;
		else if (k <= lSize)
			return select(k, rt->L);
		else 
			return select(k - lSize - rt->number, rt->R);
	}

	void BST::removeAll(pNode &rt)
	{
		if (!rt) 
			return;
		removeAll(rt->L);
		removeAll(rt->R);
		delete rt;
		rt = NULL;
	}


	void BST::leftRotate(pNode &rt) 
	{
		pNode p = rt->L;
		rt->L = p->R;
		p->R = rt;
		rt->size = rt->L->size + rt->R->size + rt->number;
		p->size = rt->size + p->L->size + p->number;
		rt = p;
	}

	void BST::rightRotate(pNode &rt)
	{
		pNode p = rt->R;
		rt->R = p->L;
		p->L = rt;
		rt->size = rt->L->size + rt->R->size + rt->number;
		p->size = rt->size + p->R->size + p->number;
		rt = p;
	}
