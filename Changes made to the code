Changes made to the trace replay application(as suggested by Tom Henderson) : 

--In trace-replay-example.cc
1. Used WifiMacHelper instead of the depreciated NqosWifiMacHelper
2. Assignstreams() used with variable streamIndex, in case more than one stream has to be assigned.

--In trace-replay-helper.cc
1. Placed all NS_LOG_FUNCTION log statements at the top of the method, and included
input parameters; e.g. NS_LOG_FUNCTION (this << dataRate);
2. Time objects used at all places

--In trace-replay-client.h
1. Passed addresses in our APIs as references to const; e.g. 'const Address &ipClient')

--In trace-replay-client.cc
1. Comments aligned with code

--In trace-replay-server.h
1. Passed addresses in our APIs as references to const; e.g. 'const Address &ipClient')

--In trace-replay-server.cc
1. Comments aligned with code
