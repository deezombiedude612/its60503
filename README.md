# its60503
ITS60503: Operating Systems

### Assignment 1
The console application created for this assignment is supposed to demonstrate how child processes and pipes work in Linux through the usage of the fork() command. A text file's content is passed on to the application which forks 3 child processes, each of which acts as a filter for the content; in other words, the content is selectively removed when it reaches each child process before being passed to the parent process again. A log is created to show what has been removed from the original content of the text file at each process spawned.

### Assignment 2
The console application created for this assignment aims to display the process of passing data via multi-threading which is often used by most modern computer programs to take advantage of multi-core CPUs and to avoid the problems of blocking I/O system calls. This multi-threaded application reads data from an input file and writes it to an output file using the specified number of threads to be used (specified by the user via UNIX command line).


NOTE: Both console applications for these assignments are programmed using the C programming language, each of which are accompanied by makefiles and their respective required text files.
