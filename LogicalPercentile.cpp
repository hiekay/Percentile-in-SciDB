/*
compute the window aggregate incrementally.

*/

#include <boost/shared_ptr.hpp>
#include <log4cxx/logger.h>

#include <query/Operator.h>
#include <system/Exceptions.h>
#include "percentileArray.h"


namespace scidb 
{

using namespace std;

/*
 * @Synopsis:
 * ic_window()
 * ic_window( srcArray {, leftEdge, rightEdge} {, AGGREGATE_CALL} )
 * AGGREGATE_CALL := AGGREGATE_FUNC(intputAttr) [as resultName]
 * AGGREGATE_FUNC := sum | avg | min | max | count | stdev | var 
 *
 * @output array:
 * < aggregate calls' resultNames>
 * [ srcDims ]
*/

class LogicalPercentile : public LogicalOperator
{
public:

    LogicalPercentile(const string& logicalName, const string& alias):
        LogicalOperator(logicalName, alias)
    {
		// input an array 
    	ADD_PARAM_INPUT();
		// followed by a variable list of paramaters.	
		ADD_PARAM_VARIES();
    }


	// Given the schemas of input array, the parameters supplied so far, return a list of all possible types of the next parameter
	std::vector<boost::shared_ptr<OperatorParamPlaceholder> > nextVaryParamPlaceholder(const std::vector<ArrayDesc>& schemas)
	{
		// There must be as many {, leftEdge, rightEdge} pairs as there are dimensions in srcArray
		// There must be at least one aggregate_call

		std::vector<boost::shared_ptr<OperatorParamPlaceholder> > res;

		if ( _parameters.size() < schemas[0].getDimensions().size()*2 ) 		//still window boundaries
			res.push_back(PARAM_CONSTANT("int64"));
		else { 
			res.push_back(PARAM_IN_ATTRIBUTE_NAME("void"));
			res.push_back(PARAM_CONSTANT("int64"));
			res.push_back(PARAM_CONSTANT(TID_STRING));
			res.push_back(END_OF_VARIES_PARAMS());
		}
		return res;
	}



	//param desc --> the input array schema
	ArrayDesc createWindowDesc(ArrayDesc const& desc)
	{
		//get dimensions for output array
		Attributes const &attrs = desc.getAttributes();
		/*
		Dimensions aggrDims(dims.size());
		for (size_t i = 0; i < dims.size(); i++)
		{
			DimensionDesc const& srcDim = dims[i];
			aggrDims[i] = DimensionDesc(srcDim.getBaseName(),
									    srcDim.getNamesAndAliases(),
								   	    srcDim.getStartMin(),
									    srcDim.getCurrStart(),
									    srcDim.getCurrEnd(),
									    srcDim.getEndMax(),
									    srcDim.getChunkInterval(),
									    0);
		}
		*/

		Attributes newAttributes;
		size_t n = 0;
		for (size_t i=desc.getDimensions().size()*2; i < _parameters.size()-1; i=i+2)
		{
			const AttributeDesc &attr = attrs[((boost::shared_ptr<OperatorParamReference>&)_parameters[i])->getObjectNo()]; 
			newAttributes.push_back(AttributeDesc(n, attr.getName(), 
												  attr.getType(), 
												  attr.getFlags(),
												  attr.getDefaultCompressionMethod(),
												  attr.getAliases()));
			

		}
	
		return ArrayDesc(desc.getName(), newAttributes, desc.getDimensions());
	}



	// output array schema
    ArrayDesc inferSchema(std::vector< ArrayDesc> schemas, shared_ptr< Query> query)
    {
		// input array to be only one.
    
		SCIDB_ASSERT(schemas.size() == 1);

		ArrayDesc const& desc = schemas[0];
		size_t nDims = desc.getDimensions().size();
		vector<WindowBoundaries> window(nDims);
		size_t windowSize = 1;
		for (size_t i = 0, size = nDims * 2, boundaryNo = 0; i < size; i+=2, ++boundaryNo)
		{
			int64_t boundaryLower = 
					evaluate(((boost::shared_ptr<OperatorParamLogicalExpression>&)_parameters[i])->getExpression(), query, TID_INT64).getInt64();
			
			//if (boundaryLower < 0)
		
			int64_t boundaryUpper = 	
					evaluate(((boost::shared_ptr<OperatorParamLogicalExpression>&)_parameters[i+1])->getExpression(), query, TID_INT64).getInt64();

			//if (boundaryUpper < 0)

			window[boundaryNo] = WindowBoundaries(boundaryLower, boundaryUpper);
			windowSize *= window[boundaryNo]._boundaries.second + window[boundaryNo]._boundaries.first + 1; 
		}
		if (windowSize == 1)
			throw USER_QUERY_EXCEPTION(SCIDB_SE_INFER_SCHEMA, SCIDB_LE_OP_WINDOW_ERROR4,
				_parameters[0]->getParsingContext());

        return createWindowDesc(desc);
    }
};

REGISTER_LOGICAL_OPERATOR_FACTORY(LogicalPercentile, "percentile");

} //namespace scidb
