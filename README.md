Percentile-in-SciDB
===================

To implement  window aggregates of percentile into SciDB as a plugin user-defined-operator
SciDB Version: 13.12

===================
Usage:
copy libpercentile.so into the scidb plugin dir, 
run the following command within 'iquery' client of SciDB
AFL% load_library('percentile');

Then percentile can be used .

