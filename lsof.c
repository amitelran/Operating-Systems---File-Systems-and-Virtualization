#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


#define BLENGTH 512
#define PROC_INUM 300
#define NOFILE 16
#define DIRENTSIZE 16
#define NPROC 32
#define PROCESS_MULTIPLY (5+NOFILE) 


struct fdinfoData {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  int inode_num ;
};

int findPids (int pids[NPROC]);
void writeString (char * dirData, char * text, int * dataWritten);
void writeNumber (char * dirData, int number, int * dataWritten);
int readFDs(int pid , int file);
void printFdinfo(int pid,int file_num,char buffer[BLENGTH],int size_read);
char * num2type(int type);

/***********************    MAIN  ***********************/


int main(int argc, char *argv[])
{  
	int fd1 = open("/proc/1",O_RDONLY);
	int fd2 = open("/proc/2/fdinfo",O_RDONLY);

    printf(1, "**************************   lsof START  **************************\n\n");
    int pids[NPROC];
    int pids_length = findPids(pids);

    printf(1, "<PID> <FD number> <refs for file descriptor> <Inode number> <file type>\n");
    for(int i = 0; i < pids_length; i++)
	{	
		printf(1, "\n");
		for(int fileNum = 0; fileNum < NOFILE;fileNum++)
		{
			readFDs(pids[i] ,fileNum);
		}
	}										
    printf(1, "\n\n**************************   lsof FINISH  **************************\n\n");

    close(fd1);
	close(fd2);
    exit();
}

int findPids (int pids[NPROC])
{	
	struct dirent dir;
	int dataWritten = 0;
	int pidIndex = 0;
	char fdbuffer[BLENGTH];
	char procbuffer[BLENGTH];
	writeString(fdbuffer,"/proc",&dataWritten);
	int fd = open(fdbuffer, O_RDONLY);
	if(fd == -1)
	{
		 printf(1, " Cannot open /proc\n");
		 return -1;
	}
	else
	{	
		int off = 0	;
		 int read_val = read(fd, procbuffer, BLENGTH);
		 if(read_val > 0)
		 {
			 int iters = read_val / DIRENTSIZE;
			 for(int i = 0; i < iters; i++)
			 {
			 	memmove(&dir,procbuffer + off, DIRENTSIZE);
				if(dir.inum > PROC_INUM)
				{
					pids[pidIndex] = dir.inum - PROC_INUM;
					pidIndex++;
				}	
			 	off += DIRENTSIZE;
			 }
		}
		else
		{
			 printf(1, " Cannot read from /proc\n");
		}
	}
	close(fd);
	return pidIndex;


}


int readFDs(int pid , int file)
{	
	int dataWritten = 0;
	char fdbuffer[BLENGTH];
	//char procbuffer[BLENGTH];

	//init buffer
	for(int i = 0 ;i < BLENGTH; i++)
	{
		fdbuffer[i]= 0;
	}

	writeString(fdbuffer, "/proc/", &dataWritten);
	writeNumber(fdbuffer, pid, &dataWritten);
  	writeString(fdbuffer, "/fdinfo/", &dataWritten);
  	writeNumber(fdbuffer, file, &dataWritten);

  	int fd = open(fdbuffer, O_RDONLY);

	if(fd < 0)
	{	
		return 0;
	}
	else
	{	
		printFdinfo(pid,file,fdbuffer,0);
	}
	close(fd);
	return 1;
}



//printing <PID> <FD number> <refs for file descriptor> <Inode number> <file type>
void printFdinfo(int pid,int file_num,char buffer[BLENGTH],int size_read)
{	
	struct fdinfoData fdData;

	getFileInfo(file_num,&fdData);

	printf(1, " %d        %d                    %d                 %d            %s\n", 
		pid, file_num, fdData.ref, fdData.inode_num, num2type(fdData.type));
}


void
writeString (char * dirData, char * text, int * dataWritten)
{					
	memmove(dirData + *dataWritten,text,strlen(text));
	*dataWritten += strlen(text);	
}

void
writeNumber (char * dirData, int number, int * dataWritten)
{				
	char itoaBuff[20];
	char * text = itoa(itoaBuff,number);	
	writeString(dirData, text, dataWritten);	 
}


char * num2type(int type)
{
	switch(type)
	{
		case 0: return "FD_NONE";
		case 1: return "FD_PIPE";
		case 2: return "FD_INODE";
	}
	return "";
}
