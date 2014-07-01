

#include "query/Operator.h"
#include "array/Metadata.h"
#include "array/Array.h"
#include "percentileArray.h"

double defaultResult[5] = {0,0,0,1e10,-1e10};

namespace scidb 
{

using namespace std;
using namespace boost;




//  * PerWindowComputer

	void perWindowComputer::initialize(vector<WindowBoundaries> const& window, int percent, string const& method)	
	{
		_bufferSize = 1;
		for (size_t i = 0; i < window.size(); i++)	
			_bufferSize *= window[i]._boundaries.first + window[i]._boundaries.second + 1;
		_buffer = new double[_bufferSize];
		_percent = percent;
		_method = method;
	}

	void perWindowComputer::insertNew(double v)
	{
		_buffer[_size++] = v;
	}
	
	int compare(const void*a, const void*b)
	{
		return *(double*)a > *(double*)b ? 1:-1;
	}

	void swap(double* a, double* b)
	{
		double tmp = *a;
		*a = *b;
		*b = tmp;
	}
	int parition(double *a, int l, int r, int p)
	{
		double pv = a[p];
		swap(&a[p],&a[r]);
		int storeIndex = l;
		for (int i=l; i<=r-1; i++)
		if (a[i] < pv) {
			swap(&a[storeIndex],&a[i]);
			storeIndex++;
		}
		swap(&a[r],&a[storeIndex]);
		return storeIndex;
	}

	double qselect(double *a, int l, int r, int k)
	{
		if (l == r)
			return a[l];
		int p = (l+r)/2, pp;

//-----------------------------------------partition-----------------------------------------
		double pv = a[p];		// pv --> the pivot value 
		swap(&a[p],&a[r]);
		pp = l;
		for (int i=l; i<=r-1; i++)	// move all the a[i] smaller than pv to its left
		if (a[i] < pv) {
			swap(&a[pp],&a[i]);
			pp++;
		}
		swap(&a[r],&a[pp]);		// move pivot value to its final place
		p = pp;					// pp is the final place of pivot value
//----------------------------------------------------------------------------------------------


		if (k == p)
			return a[p];
		else if (k < p) 
			return qselect(a, l, p-1, k);
		else 
			return qselect(a, p+1, r, k);
	}


	double perWindowComputer::calculate()
	{
		int k = (int)((1.0*_percent/100.0)*_size+0.5)-1;
		if (k<0) k = 0;

		if (_method == percentileArray::QSELECT) {
			qselect(_buffer, 0, _size-1, k);
		}
		else {
			//if ( _method == percentileArray::QSORT) {
			qsort(_buffer, _size, sizeof(double), compare);
			return _buffer[k];
		}

	}


//	* IcWindowChunkIterator -------------------------------------------------------------

	percentileChunkIterator::percentileChunkIterator(percentileArrayIterator const& arrayIterator, percentileChunk const& chunk, int mode):
		_array(arrayIterator.array),
		_chunk(chunk),
		_firstPos(_chunk.getFirstPosition(false)),
		_lastPos(_chunk.getLastPosition(false)),
		_currPos(_firstPos.size()),
		_attrID(_chunk._attrID),
		_percentile(_array._percentiles[_attrID]),
		_defaultValue(_chunk.getAttributeDesc().getDefaultValue()),
		_iterationMode(mode),
		_method(_array.getMethod()),
		_nextValue(TypeLibrary::getType(_chunk.getAttributeDesc().getType()))
	{

		int iterMode = IGNORE_EMPTY_CELLS;
		_inputIterator = arrayIterator.iterator->getChunk().getConstIterator(iterMode);

		if (_array.getArrayDesc().getEmptyBitmapAttribute()) 		//empty ?
		{
			AttributeID eAttrId = _array._inputArray->getArrayDesc().getEmptyBitmapAttribute()->getId();
			_emptyTagArrayIterator = _array._inputArray->getConstIterator(eAttrId);

			if (!_emptyTagArrayIterator->setPosition(_firstPos))
				throw SYSTEM_EXCEPTION(SCIDB_SE_EXECUTION, SCIDB_LE_OPERATION_FAILED) << "setPosition";
			_emptyTagIterator = _emptyTagArrayIterator->getChunk().getConstIterator(IGNORE_EMPTY_CELLS);
		}

		//--------------------------------------------------------------------------
		_PwWorker.initialize(_chunk._array._window, _percentile._percent, _array.getMethod());
		_IcWorker.removeAll();	
		 
		reset();
	}

	int percentileChunkIterator::getMode()
	{
		return _iterationMode;
	}

	







	void percentileChunkIterator::perWindowCalculate()
	{
		size_t nDims = _currPos.size();
		Coordinates firstGridPos(nDims);
		Coordinates lastGridPos(nDims);
		Coordinates currGridPos(nDims);
		// get the window grid scope
		for (size_t i = 0; i < nDims; i++) {
			firstGridPos[i] = std::max(_currPos[i] - _chunk._array._window[i]._boundaries.first,
					_chunk._array._dimensions[i].getStartMin());
			lastGridPos[i] = std::min(_currPos[i] + _chunk._array._window[i]._boundaries.second,
					_chunk._array._dimensions[i].getEndMax());
			currGridPos[i] = firstGridPos[i];
		}
		
		currGridPos[nDims-1] -= 1;
		
		_PwWorker._size = 0;
		while (true) {
			for (size_t i = nDims-1; ++currGridPos[i] > lastGridPos[i]; i--)
			{
				if (i == 0) {
					double res = _PwWorker.calculate();
					//double res = 1;
					_nextValue.setData(&res, sizeof(double));
					return;
				}
				currGridPos[i] = firstGridPos[i];
			}
			if (_inputIterator->setPosition(currGridPos))
			{
				double v = ValueToDouble(TID_DOUBLE, _inputIterator->getItem());
				_PwWorker.insertNew(v);
			}
		}
	}

	void percentileChunkIterator::insertWindowUnit(Coordinates const& first, Coordinates const& last, int lastDimValue)
	{
		size_t nDims = first.size();	
		Coordinates currPos(nDims);
		currPos = first;
		currPos[nDims-1] = lastDimValue;


		if (nDims == 1) {
			if (_inputIterator->setPosition(currPos))
			{
				double v = ValueToDouble(TID_DOUBLE, _inputIterator->getItem());
				_IcWorker.insert(v);
			}
			return;
		}


		currPos[nDims-2] -= 1;
		while (true)
		{
			for (size_t i =  nDims-2; ++currPos[i] > last[i]; i--)
			{
				if (i == 0)
					return;
				currPos[i] = first[i];
			}

			if (_inputIterator->setPosition(currPos))
			{
				double v = ValueToDouble(TID_DOUBLE, _inputIterator->getItem());
				_IcWorker.insert(v);			
			}
		}
	}

	void percentileChunkIterator::removeWindowUnit(Coordinates const& first, Coordinates const& last, int lastDimValue)
	{
		size_t nDims = first.size();
		Coordinates currPos(nDims);
		currPos = first;
		currPos[nDims-1] = lastDimValue;

		if (nDims == 1) {
			if (_inputIterator->setPosition(currPos))
			{
				double v = ValueToDouble(TID_DOUBLE, _inputIterator->getItem());
				_IcWorker.remove(v);
			}
			return;
		}

		currPos[nDims-2] -= 1;
		while (true)
		{
			for (size_t i =  nDims-2; ++currPos[i] > last[i]; i--)
			{
				if (i == 0)
					return;
				currPos[i] = first[i];
			}

			if (_inputIterator->setPosition(currPos))
			{
				double v = ValueToDouble(TID_DOUBLE, _inputIterator->getItem());
				_IcWorker.remove(v);			
			}
		}
	}

	void percentileChunkIterator::incrementalCalculate()
	{	
		size_t nDims = _currPos.size();
		Coordinates firstGridPos(nDims);
		Coordinates lastGridPos(nDims);

		for (size_t i = 0; i< nDims ; i++) {
			firstGridPos[i] = std::max(_currPos[i] - _chunk._array._window[i]._boundaries.first,
					_chunk._array._dimensions[i].getStartMin());
			lastGridPos[i] = std::min(_currPos[i] + _chunk._array._window[i]._boundaries.second,
					_chunk._array._dimensions[i].getEndMax());
		}

		if (_currPos[nDims-1] == _firstPos[nDims-1])			//new BaseWindow
		{
			_IcWorker.removeAll();
	
			for (size_t i = firstGridPos[nDims-1]; i <= lastGridPos[nDims-1]; i++)
			{
				insertWindowUnit(firstGridPos, lastGridPos, i);
			}
		}
		else {
			if (_firstPos[nDims-1] != firstGridPos[nDims-1])
				removeWindowUnit(firstGridPos, lastGridPos, firstGridPos[nDims-1]-1);

			int realLastPos = _currPos[nDims-1] + _chunk._array._window[nDims-1]._boundaries.second;
			if (realLastPos == lastGridPos[nDims-1])
			{
				insertWindowUnit(firstGridPos, lastGridPos, lastGridPos[nDims-1]);
			}
		}

		int k = (int)((1.0*_percentile._percent/100.0)*_IcWorker.getSize()+0.5);
		if (k == 0) k=1;
		double v = _IcWorker.select(k);
		_nextValue.setData(&v, sizeof(double));
	}


	//	calculating the whole window
	void percentileChunkIterator::calculateNextValue()
	{

		if (_method  == percentileArray::QSORT || _method == percentileArray::QSELECT)
		{
			perWindowCalculate();
		}
		else {
			incrementalCalculate();
		}   	
	}

	Value& percentileChunkIterator::getItem()
	{
		if (!_hasCurrent)
			throw USER_EXCEPTION(SCIDB_SE_EXECUTION, SCIDB_LE_NO_CURRENT_ELEMENT);
		return _nextValue;
	}

	Coordinates const& percentileChunkIterator::getPosition()
	{
		return _currPos;
	}

	bool percentileChunkIterator::setPosition(Coordinates const&pos)
	{
		for (size_t i = 0; i < _currPos.size(); i++)
		{
			if (pos[i] < _firstPos[i] || pos[i] > _lastPos[i])
				return false;
		}
		_currPos = pos;
		number += 1000;
		calculateNextValue();
		if (_iterationMode & IGNORE_NULL_VALUES && _nextValue.isNull())
			return false;
		if (_iterationMode & IGNORE_DEFAULT_VALUES && _nextValue == _defaultValue)
			return false;

		return true;
	}


	bool percentileChunkIterator::isEmpty()
	{
		return false;
	}

	void percentileChunkIterator::reset()
	{
		if (setPosition(_firstPos))
		{
			_hasCurrent = true;
			return;
		}
		++(*this);
	}

	void percentileChunkIterator::operator ++()
	{
		bool done = false;
		while (!done)
		{
			size_t nDims = _firstPos.size();
			for (size_t i = nDims-1; ++_currPos[i] > _lastPos[i]; i--)
			{
				if (i == 0)
				{
					_hasCurrent = false;
					return;
				}
				_currPos[i] = _firstPos[i];
			}
			number += 1;
			calculateNextValue();
			
			if (_iterationMode & IGNORE_NULL_VALUES && _nextValue.isNull())
				continue;
			if (_iterationMode & IGNORE_DEFAULT_VALUES && _nextValue == _defaultValue)
				continue;
			done = true;
			_hasCurrent = true;
		}

	}

	bool percentileChunkIterator::end()
	{
		return !_hasCurrent;
	}

	ConstChunk const& percentileChunkIterator::getChunk()
	{
		return _chunk;
	}



//	* IcWindowChunk ------------------------------------------------------------------------
	percentileChunk::percentileChunk(percentileArray const& arr, AttributeID attr):
		_array(arr),
		_arrayIterator(NULL),
		_nDims(arr._desc.getDimensions().size()),
		_firstPos(_nDims),
		_lastPos(_nDims),
		_attrID(attr)
	{}

	Array const& percentileChunk::getArray() const
	{
		return _array;
	}


	const ArrayDesc& percentileChunk::getArrayDesc() const
	{
		return _array._desc;
	}

	const AttributeDesc& percentileChunk::getAttributeDesc() const
	{
		return _array._desc.getAttributes()[_attrID];
	}

	Coordinates const& percentileChunk::getFirstPosition(bool withOverlap) const
	{
		return _firstPos;
	}

	Coordinates const& percentileChunk::getLastPosition(bool withOVerlap) const
	{
		return _lastPos;
	}

	shared_ptr<ConstChunkIterator> percentileChunk::getConstIterator(int iterationMode) const
	{
		SCIDB_ASSERT( (_arrayIterator != NULL) );
		ConstChunk const& inputChunk = _arrayIterator->iterator->getChunk();	//chunk in SrcArr
		
		if (_array.getArrayDesc().getEmptyBitmapAttribute() && _attrID == _array.getArrayDesc().getEmptyBitmapAttribute()->getId())
		{
			return inputChunk.getConstIterator((iterationMode & ~ChunkIterator::INTENDED_TILE_MODE) | ChunkIterator::IGNORE_OVERLAPS);
		}	
		// materiliezed?

		return shared_ptr<ConstChunkIterator>(new percentileChunkIterator(*_arrayIterator, *this, iterationMode));
	}

	int percentileChunk:: getCompressionMethod() const
	{
		return _array._desc.getAttributes()[_attrID].getDefaultCompressionMethod();
	}


	void percentileChunk::setPosition(percentileArrayIterator const* iterator, Coordinates const& pos)
	{
		_arrayIterator = iterator;
		_firstPos = pos;
		Dimensions const& dims = _array._desc.getDimensions();

		for (size_t i = 0; i < dims.size(); i++)
		{
			_lastPos[i] = _firstPos[i] + dims[i].getChunkInterval() - 1;
			if (_lastPos[i] > dims[i].getEndMax())
				_lastPos[i] = dims[i].getEndMax();

		}
	}




//	* IcWindowArrayIterator ---------------------------------------------------------------------


	percentileArrayIterator::percentileArrayIterator(percentileArray const& arr, AttributeID attrID, PercentileObject input):
		array(arr),
		iterator(arr._inputArray->getConstIterator(input._inAttrID)),
		currPos(arr._dimensions.size()),
		chunk(arr, attrID)
	{
		reset();
	}
		
	ConstChunk const& percentileArrayIterator::getChunk()
	{
		if (!chunkInitialized)
		{
			chunk.setPosition(this, currPos);
			chunkInitialized = true;
		}
		return chunk;
	}

	Coordinates const& percentileArrayIterator::getPosition()
	{
		if (!hasCurrent)
			throw USER_EXCEPTION(SCIDB_SE_EXECUTION, SCIDB_LE_NO_CURRENT_ELEMENT);
		return currPos;
	}


	bool percentileArrayIterator::setPosition(Coordinates const& pos)
	{
		chunkInitialized = false;					//?
		if (!iterator->setPosition(pos))
		{
			return hasCurrent = false;
		}
		currPos = pos;
		return hasCurrent = true;
	}


	bool percentileArrayIterator::end()
	{
		return !hasCurrent;
	}

	void percentileArrayIterator::operator ++()
	{
		if (!hasCurrent)
			throw USER_EXCEPTION(SCIDB_SE_EXECUTION, SCIDB_LE_NO_CURRENT_ELEMENT);
		chunkInitialized = false;					//?
		++(*iterator);
		hasCurrent = !iterator->end();
		if (hasCurrent)
		{
			currPos = iterator->getPosition();
		}
	}

	void percentileArrayIterator::reset()
	{
		chunkInitialized = false;
		iterator->reset();
		hasCurrent = !iterator->end();
		if (hasCurrent)
		{
			currPos = iterator->getPosition();
		}
	}



	const std::string percentileArray::QSORT="qsort";
	const std::string percentileArray::QSELECT="qselect";




//	*  prcnetileArray     	---------------------------------------------------------------------

	percentileArray::percentileArray(ArrayDesc const& desc, shared_ptr<Array> const& inputArray,
			vector<WindowBoundaries> const& window, vector<PercentileObject> const& percentiles, string const& method):
		_desc(desc),
		_inputDesc(inputArray->getArrayDesc()),
		_window(window),
		_dimensions(_desc.getDimensions()),
		_inputArray(inputArray),
		_percentiles(percentiles),
		_method(method)
	{}

	ArrayDesc const& percentileArray::getArrayDesc() const
	{
		return _desc;
	}

	shared_ptr<ConstArrayIterator> percentileArray::getConstIterator(AttributeID attr) const
	{
		return shared_ptr<ConstArrayIterator>(new percentileArrayIterator(*this, attr, _percentiles[attr] ));
	}


} // namespace

