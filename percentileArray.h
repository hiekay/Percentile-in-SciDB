


#include <vector>
#include <string>

#include "query/Aggregate.h"
#include "array/DelegateArray.h"
#include "array/Metadata.h"
#include "array/MemArray.h"
#include "query/FunctionDescription.h"
#include "query/Expression.h"
#include "query/Aggregate.h"
#include "BST.h"


namespace scidb
{

using namespace std;
using namespace boost;

class incrementalComputer;
class perWindowComputer;
class percentileArray;
class percentileArrayIterator;


struct PercentileObject
{
	PercentileObject(AttributeID id, int p)
	{
		_inAttrID = id;
		_percent = p;
		
	}
	AttributeID _inAttrID;
	int _percent;
};

struct WindowBoundaries
{
		WindowBoundaries()
				
		{
			_boundaries.first = _boundaries.second = 0;
		}

		WindowBoundaries(Coordinate first, Coordinate second)
		{
			SCIDB_ASSERT(first >= 0);
			SCIDB_ASSERT(second >= 0);

			_boundaries.first = first;
			_boundaries.second = second;

		}
		std::pair<Coordinate, Coordinate> _boundaries;
};


struct PriorityQueueNode {
	friend bool operator < (PriorityQueueNode n1, PriorityQueueNode n2)
	{
		return n1._value < n2._value;
	}
public:
	PriorityQueueNode(int pos, double value)
	{
		_value = value;
		_pos = pos;
	}
	double _value;
	int _pos;
};


class incremntalComputer 	//compute window percentile with data sturecture of balanced binary tree
{
	friend class percentileChunkIterator;
public:
	void insertNew(double v);
private:
	string _method;
};

class perWindowComputer		//Nomal computer (qsort/ qselect) calcuted per window
{
	friend class percentileChunkIterator;
public:
	void insertNew(double v);
	void initialize(vector<WindowBoundaries> const& window, int percent, string const& method);
	double calculate();	
	~perWindowComputer()
	{
		delete []_buffer;
	}
private:
	int _size;
	int _percent;
	int _bufferSize;
	double* _buffer;
	string _method;
};


class percentileChunk : public ConstChunk
{
	friend class percentileChunkIterator;
public:
	percentileChunk(percentileArray const& array, AttributeID attrID);

	virtual const ArrayDesc& getArrayDesc() const;
	virtual const AttributeDesc& getAttributeDesc() const;
	virtual Coordinates const& getFirstPosition(bool withOverlap) const;
	virtual Coordinates const& getLastPosition(bool withOverlap) const;
	virtual shared_ptr<ConstChunkIterator> getConstIterator(int iteratorMode) const;
	virtual int getCompressionMethod() const;
	virtual Array const& getArray() const;

	void setPosition(percentileArrayIterator const* iterator, Coordinates const& pos); 

private:

	percentileArray const& _array;
	percentileArrayIterator const* _arrayIterator;
	size_t _nDims;
	Coordinates _arrSize;
	Coordinates _firstPos;
	Coordinates _lastPos;
	AttributeID	_attrID;
	AggregatePtr _aggregate;

	Value _nextValue;
};



/*
class IncrementalComputer
{
	friend class IcWindowChunkIterator;
public:
	void removeOld();
	void insertNew(int pos, double v);
	void baseWindowInitialize();
	double calculateCurrentValue(int pos);
	~IncrementalComputer()
	{
		delete []_interBuf;
	}
private:
	// sum | avg 
	int _aggrType;
	int _currPos;
	int _headPos;
	int _bufferSize;
	double _windowSize;
	double _sum;
	double* _interBuf;
	priority_queue<PriorityQueueNode> _heap;
	// max | min
};
*/

class percentileChunkIterator : public ConstChunkIterator
{
public:
	virtual int getMode();
	virtual bool isEmpty();
	virtual Value& getItem();
	virtual void operator ++();
	virtual bool end();
	virtual Coordinates const& getPosition();
	virtual bool setPosition(Coordinates const& pos);
	virtual void reset();
	ConstChunk const& getChunk();

	percentileChunkIterator(percentileArrayIterator const& arrayIterator, percentileChunk const& aChunk, int mode);
private:
	void calculateNextValue();
	void perWindowCalculate();
	void incrementalCalculate();
	void insertWindowUnit(Coordinates const& first, Coordinates const& last, int d);
	void removeWindowUnit(Coordinates const& first, Coordinates const& last, int d);
	void accumulate(Value const& v);
	

	perWindowComputer _PwWorker;
	BST _IcWorker;
	percentileArray const& _array;
	percentileChunk const& _chunk;
	Coordinates const& _firstPos;
	Coordinates const& _lastPos;
	Coordinates _currPos;
	bool _hasCurrent;
	AttributeID _attrID;
	PercentileObject const &_percentile;
	Value _defaultValue;
	int _iterationMode;
	shared_ptr<ConstChunkIterator> _inputIterator;
	shared_ptr<ConstArrayIterator> _emptyTagArrayIterator;
	shared_ptr<ConstChunkIterator> _emptyTagIterator;
	Value _nextValue;
	bool _noNullsCheck;
	double _result;	
	double number;
	string _method;
};



class percentileArrayIterator : public ConstArrayIterator
{
	friend class percentileChunkIterator;
	friend class percentileChunk;
public:
	virtual ConstChunk const& getChunk();
	virtual Coordinates const& getPosition();
	virtual bool setPosition(Coordinates const &pos);
	virtual bool end();
	virtual void operator ++();
	virtual void reset();


	percentileArrayIterator(percentileArray const& array, AttributeID id, PercentileObject input);

private:
	percentileArray const& array;
	shared_ptr<ConstArrayIterator> iterator;	// iterator of the input array;
	Coordinates currPos;
	bool hasCurrent;
	percentileChunk chunk;
	bool chunkInitialized;
};



class percentileArray : public Array
{
	friend class percentileArrayIterator;
	friend class percentileChunkIterator;
	friend class percentileChunk;
public:
	virtual ArrayDesc const& getArrayDesc() const;
	virtual shared_ptr<ConstArrayIterator> getConstIterator(AttributeID attr) const;
	
	string const& getMethod() const { return _method; };

	percentileArray(ArrayDesc const& desc,
				    shared_ptr<Array> const& inputArray,
				    vector<WindowBoundaries> const& window,
				    vector<PercentileObject> const& percentiles,
					string const& method);
	static const std::string QSORT;
	static const std::string QSELECT;
private:
	ArrayDesc _desc;
	ArrayDesc _inputDesc;
	vector<WindowBoundaries> _window;
	Dimensions _dimensions;
	shared_ptr<Array> _inputArray;
	vector<PercentileObject> _percentiles;
	string _method;
};



}
