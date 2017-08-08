ITS60503 - Assignment 2
Operating Systems
Student Name: Heng Hian Wee
Student ID: 0321343

COMMAND LINE GUIDE

##########################################################################################

To perform all necessary compilation(s) and\or recompilation(s)

$ make

or

$ make all

This will in turn invoke the following command:

$ gcc -lpthread -o mainexe mainexe.c

##########################################################################################

Execute the code in the following format:

$ ./mainexe <input_file> <output_file> <number_of_threads>

Note: Number of threads must be indicated with a positive integer.

##########################################################################################

The following command cleans the generated output object.

$ make clean

This will in turn invoke the following command:

$ rm -rf mainexe *~

Note: 
The output file generated after running the program depends on what the user types to execute the code. 
Should thou wish to delete-eth the output file, thou shall doth so manually.

$ rm -rf <output_file>

END