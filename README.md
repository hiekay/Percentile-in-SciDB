Percentile-in-SciDB
===================

To implement  window aggregates of percentile into SciDB as a plugin user-defined-operator                                  
SciDB Version: 13.12

===================
Usage:
copy libpercentile.so into the scidb plugin dir, 
run the following command within 'iquery' client of SciDB
AFL% load_library('percentile');

Then percentile can be used.                                                                                                                                                                                                                                          

percentile(Array_name, dim1_low, dim1_high, [dim2_low, dim2_high, ...], attr_name, percentile_percentage, [Method]);            
  As a window aggregate, the pairs of dim_low, dim_high are used to define the window size, just the same as 'window' operator   in SciDB, can be refered in SciDB Userguide.                                                                                          
  attr_name refers to the attribute to be computed over.                                                                        
  [Method] refers to the method used for the query.                                                                           
  --'qsort' refers to quicksort method                                                                                        
  --'BST' refers to incremental computation.



====================
To Make:                                                                                                                        
Need to modify the Makefile, set the SciDB-Source Directory with correct location in your machine.                          
A few libraries needed to compile, same ones as building SciDB from Source Code.
