The link-time library component of Auklet, libauklet, is an instrumentation
library. It generates the raw metrics that inform the performance and event data
presented in the user interface.

The basic mechanism for generating profile data is the compile-time
GCC-compatible flag, `-finstrument-functions` (see [here][1]), which injects a
pair of function calls into every function in the current compilation unit.
These are used as "hooks" that enable the instrument to track a program's
stack. The program's true stack is not examined; only a reconstruction of it.

libauklet generates a stacktrace when the process receives a POSIX signal
that typically represents an error---SIGILL, SIGFPE, or SIGSEGV. This appears in
the frontend as an event.

[1]: https://gcc.gnu.org/onlinedocs/gcc-4.3.3/gcc/Code-Gen-Options.html

libauklet generates a [callgraph][2], which is like a roadmap of a program's
execution. libauklet's callgraph is more detailed than traditional
directed-acyclic-graph (DAG) callgraphs, because it preserves callsite and
stacktrace information. This means that a node in the callgraph is not
necessarily unique. There may be multiple nodes with the same function address
and callsite, if they differ in their stacktrace. Thus it may be more accurate
to view libauklet's callgraph as an aggregation of stacktraces.

[2]: https://en.wikipedia.org/wiki/Call_graph

Callgraphs refer to functions by their addresses in the virtual memory space
provided by the kernel. These addresses can later be "symbolized" with debugging
and symbol information, which associates the function addresses with function
names and source code locations. A basic callgraph generator and symbolizer
script can be found [here][3].

[3]: https://git.2f30.org/callgraph/files.html

Conventional profilers, such as gprof and pprof, wait until a program
terminates to write callgraph data to a file. Since Auklet is supposed to work
in production and provide constantly-updating metrics, the libauklet generates
callgraphs at periodic intervals.  Instead of writing it to a file, it is sent
over a Unix domain socket to the client instance that started the app.

In the event of an unrecoverable error, libauklet falls back to a dormant state
rather than terminate the program. This is done by using function pointers for
the top-level functions and assigning them empty functions (no-ops) after an
error happens.
