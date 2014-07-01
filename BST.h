#include <time.h>
#include <stdlib.h>

class BST
{
public:
	BST():root(NULL)  
	{
		srand(time(NULL));
	}
	~BST() 
	{
		removeAll(root);
	}

	void insert(const double key);	//insert new key
	void remove(const double key);	//remove key (just remove 1 if multiple exist)
	void removeAll();
	int getSize();
	double select(int k);	// get the kth smallest key	
private:
	struct node {			// treap node difinition
		double value;		// key value
		int weight;			// heap value for maintain balanced
		int number;			// number of this value
		int size;			// the total size of this subtree
		node *L,*R;			// left child, right child
	};
	typedef  node* pNode;

	pNode root;				// the root node
	int getSize(pNode &rt);
	void leftRotate(pNode &rt);
	void rightRotate(pNode &rt);
	void removeAll(pNode &rt);
	void insert(const double key, pNode &rt);		
	void remove(const double key, pNode &rt);
	double select(int k, pNode &rt);
};

