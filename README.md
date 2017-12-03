# CN-Assignment-Trace Replay Application 
## Course Code :CO300
### Trace based application layer modeling in ns-3

### Overview:

TraceReplay[1] is a new application layer model in ns-3.User collects pcap trace files and replays application layer traffic using this simulation model.

### Resources:

[1]Trace based application layer modeling in ns-3 (https://www.cse.iitb.ac.in/~mythili/research/papers/2016-trace-replay.pdf)
[2] https://codereview.appspot.com/289550043/
[3] https://github.com/networkedsystemsIITB/trace-replay

### Issues:

When running the trace-replay-example with a pcap file as input, the following error observed:
terminating with uncaught exception of type std::__1::regex_error

The issues have been resolved and the list of changes made can be found in the file 'Changes made to the code'

The graphs obtained can be viewed - uploadtraffic.png and downloadtraffic.png

### Final Description of the Work Done:

1. We have tested the trace replay application in ns-3.27.
2. We corrected the regex error in trace-replay-helper.cc.
3. We have implemented the changes in code pointed out by Tom Henderson.
4. After that, we run the test cases on the trace replay application using 20 pcap files that we have captured,
each of half an hour duration. We have noted down the statistics( input/output number of packets/bytes) and plotted the        number of kilobytes per second graphs for input and output pcap files. These graphs are located in 'Test cases- Graphs' directory.
5. For instructions on how to plot, read the plot_instructions file.
