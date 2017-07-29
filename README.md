# Operating-Systems---File-Systems-and-Virtualization
Xv6 operating system - File Systems, Virtualization of procfs pseudo-file-system.

Assignment specifications: https://www.cs.bgu.ac.il/~os172/wiki.files/assignment%204%20-%20172%20V2[1].pdf

Procfs psuedo-file-system implementation.
Each process has its own virtual folder with files to represent process' state (such as status, current working directory, etc.).
Also, folder containing block cache and inodes stats.

Running the project (Terminal commands):

* git clone http://www.cs.bgu.ac.il/~os172/git/Assignment4
* make
* make clean qemu
* Shell is opened, can run programs within xv6 ("lsof" program to test procfs implementation, use "/" within the procfs system).
