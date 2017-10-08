Trace-based application layer modeling (TraceReplay)
----------------------------------------------------

Model Description
*****************

The goal of TraceReplay is to make simulations more realistic. TraceReplay uses network trace collected
from user to infer and replay only application layer delays like user think times. TraceReplay extracts
application layer characteristics from a single trace, and replays this information across many users in
simulation, by using suitable randomization.

TraceReplay contains following classes:
 - TraceReplayClient: Implementation of client. In each cycle, client sends server ``n1`` packets (total ``B1`` bytes) as request and expects to receive ``B2`` bytes as the reply.
 - TraceReplayServer: Implementation of a server. In each cycle server expects to receive ``B1`` bytes as request from client and sends ``n2`` packets (total ``B2`` bytes) as reply.
 - TraceReplayPacket: Stores details (size, delay) about each packet

Delay for a packet are of two type:
 - HTTP request delay: inter-packet gap between the first packet of a HTTP request and the packet immediately preceding it on the same TCP connection (can be considered as user think time for HTTP connections)
 - SSH delay: inter-packet gap between the two packet on the same TCP connection if it is greater than 1 second (can be considered as user think time for non-HTTP connections)

If the delay for a packet is greater than 0 seconds, we also store total byte (sent and received) count for
all parallel connections (a parallel connection is in which source and destination IPs are
same but port numbers are different). While replaying the packet we compare the total number
of bytes on each of those parallel connections in the trace before the current
point of time with the actual total number of bytes on those connections in the simulation.
If any of those connections is found to have sent and received fewer bytes in simulation than in the
experimental trace, TraceReplay delays the transmission of the current packet to a point
where all parallel connections have made sufficient progress. For further details see [Paper]_

Users can either provide a pcap or trace file as input. In case, both pcap and trace file are provided, trace file will be ignored and pcap will be used to generate a new trace file.

Different behavior for each client can be simulated by providing different pcap/trace file to clients.

Random variable stream is provided to avoid synchronization between the start times of multiple clients.

The source code for TraceReplay is located in ``src/applications/model`` and consists of the following 6 files:
 - trace-replay-server.h,
 - trace-replay-server.cc,
 - trace-replay-client.h,
 - trace-replay-client.cc,
 - trace-replay-utility.h and
 - trace-replay-utility.cc

Helpers
*******
The helper code for TraceReplay is located in ``src/applications/helper`` and consists of the following 2 files:
 - trace-reaply-helper.h and 
 - trace-replay-helper.cc


Examples
********
The example for TraceReplay can be found at ``examples/trace-replay/trace-replay-example.cc``

The sample pcap file for TraceReplay can be found at ``examples/trace-replay/trace-replay-sample.pcap``

References
**********
.. [paper] ``Trace-based application layer modeling in ns3``, Prakash Agrawal and Mythili Vutukuru. Presented at Twenty-Second National Conference on Communications 2016. Link https://goo.gl/Z4ZW2K

Scope and Limitations
*********************
TraceReplay is useful in replaying network traces where application or user behavior dictates
the network traffic, rather than lower layers or wired side.

For example, it will be useful in simulating a web browsing scenario or case where user is
watching an interactive video (like in coursera or bodhitree),
beacuse in that case application/user behavior will be dominant over other factors.

An example where TraceReplay is not useful is when we replay a network trace of file download over web.
In this scenario, there is not much application or user behavior that TraceReplay can capture as the
download traffic is largely determined by the rate of the wired network bottleneck link,
and not by other factors. Results of the TraceReplay will be similar to any other application layer model.

TraceReplay is particularly useful in simulating wireless networks, because traffic characteristics (such as upload traffic)
have a significant impact on wireless channel contention, and small improvements in traffic models
can greatly enhance the realistic nature of simulation results.

The input pcap to TraceReplay must be collected from a single client, as all the connections
present in pcap will be replayed for a simulated node.