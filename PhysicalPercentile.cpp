
#include <query/Operator.h>
#include "array/Metadata.h"
#include "array/Array.h"
#include "percentileArray.h"


namespace scidb
{

using namespace std;
using namespace boost;

class PhysicalPercentile : public PhysicalOperator
{
private:
	vector<WindowBoundaries> _window;
public:

    PhysicalPercentile(string const& logicalName,string const& physicalName, Parameters const& parameters, ArrayDesc const& schema):
        PhysicalOperator(logicalName, physicalName, parameters, schema)
    {
		size_t nDims = _schema.getDimensions().size();
		_window = vector<WindowBoundaries>(nDims);
		for (size_t i = 0, size = nDims*2, boundaryNo = 0; i < size; i+=2, ++boundaryNo)
		{
			_window[boundaryNo] = WindowBoundaries(
					((shared_ptr<OperatorParamPhysicalExpression>&)_parameters[i])->getExpression()->evaluate().getInt64(),
					((shared_ptr<OperatorParamPhysicalExpression>&)_parameters[i+1])->getExpression()->evaluate().getInt64());
		}
	}


	// check if the srcArray need to be repart
	virtual bool requiresRepart(ArrayDesc const& inputSchema) const
	{
		Dimensions const& dims = inputSchema.getDimensions();
		for (size_t i = 0; i < dims.size(); i++)
		{
			DimensionDesc const& srcDim = dims[i];
			if (static_cast<uint64_t>(srcDim.getChunkInterval()) != srcDim.getLength() &&
					srcDim.getChunkOverlap() < std::max(_window[i]._boundaries.first, _window[i]._boundaries.second))
				return true;
		}
		return false;
	}

	// get the repart schema for the input array to fit in the window boundaries 
	// set the chunk overlaps correctly
	virtual ArrayDesc getRepartSchema(ArrayDesc const& inputSchema) const
	{
		Attributes attrs = inputSchema.getAttributes();
		Dimensions dims;

		for (size_t i = 0; i < inputSchema.getDimensions().size(); i++)
		{
			DimensionDesc srcDim = inputSchema.getDimensions()[i];
			int64_t overlap = srcDim.getChunkOverlap();
			int64_t const neededOverlap = std::max(_window[i]._boundaries.first, _window[i]._boundaries.second);
			if ( neededOverlap > overlap)
				overlap = neededOverlap;
			dims.push_back( DimensionDesc(srcDim.getBaseName(),
										  srcDim.getNamesAndAliases(),
										  srcDim.getStartMin(),
										  srcDim.getCurrStart(),
										  srcDim.getCurrEnd(),
										  srcDim.getEndMax(),
										  srcDim.getChunkInterval(),
										  overlap));
		}
		return ArrayDesc(inputSchema.getName(), attrs, dims);
	}


	// check if input array chunk overlap fit in window requirement
	void verifyInputSchema(ArrayDesc const&input) const
	{
		Dimensions const& dims = input.getDimensions();
		for (size_t i = 0; i < dims.size(); i++)
		{
			DimensionDesc const& srcDim = dims[i];
			if (static_cast<uint64_t>(srcDim.getChunkInterval()) != srcDim.getLength() &&
					srcDim.getChunkOverlap() < std::max(_window[i]._boundaries.first, _window[i]._boundaries.second))
				throw USER_EXCEPTION(SCIDB_SE_EXECUTION, SCIDB_LE_OP_WINDOW_ERROR2);
		}
	}


    // input array with actuall data, Non-array arguments --> in the _parameters, result of LogicalIcWindow()::inferSchema in _schema
    // Execute is called once on each instance.
	boost::shared_ptr< Array> execute(vector< boost::shared_ptr<Array> >& inputArrays, boost::shared_ptr<Query> query)
    {
		SCIDB_ASSERT(inputArrays.size() == 1);

		//get the least restrictive access mode the array support, check if ...
		if ( inputArrays[0]->getSupportedAccess() == Array::SINGLE_PASS)		
		{
			throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_UNSUPPORTED_INPUT_ARRAY) << getLogicalName(); 
		}
		
		ArrayDesc const& srcArr = inputArrays[0]->getArrayDesc();
		verifyInputSchema(srcArr);

		//get the aggregate parameters
		vector<PercentileObject> percentiles;
		for (size_t i = srcArr.getDimensions().size() *2; i < _parameters.size()-1; i+=2)
		{
			boost::shared_ptr<OperatorParam>& param = _parameters[i];

			AttributeID inAttrID = ((shared_ptr<OperatorParamReference>&)_parameters[i])->getObjectNo();
			int64_t percent = ((shared_ptr<OperatorParamPhysicalExpression>&)_parameters[i+1])->getExpression()->evaluate().getInt64();
			percentiles.push_back(PercentileObject(inAttrID, percent));

		}

		string method;
		shared_ptr<OperatorParam>& param = _parameters[_parameters.size()-1];
		if (param->getParamType() == PARAM_PHYSICAL_EXPRESSION)		 
		{
			method  = ((shared_ptr<OperatorParamPhysicalExpression>&)param)->getExpression()->evaluate().getString();
		}
		else method = "BST";

		return shared_ptr<Array>(new percentileArray(_schema, inputArrays[0], _window, percentiles, method));
       
    }
};

REGISTER_PHYSICAL_OPERATOR_FACTORY(PhysicalPercentile, "percentile", "PhysicalPercentile");

} //namespace scidb
