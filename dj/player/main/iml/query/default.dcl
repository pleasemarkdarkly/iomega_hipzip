# default.dcl: default configuration for the IML query result manager module
# edwardm@iobjects.com 11/16/01

name query
type iml

requires iml_iml debug_util djupnp_main

export QueryResult.h QueryResultManager.h

compile QueryResult.cpp QueryResultManager.cpp

tests query_result_manager_test.cpp

arch QueryResult.o QueryResultManager.o

dist include/QueryResult.h include/QueryResultManager.h default.a