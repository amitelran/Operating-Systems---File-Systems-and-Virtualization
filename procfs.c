#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "buf.h"


int getDirData(char * dirData);
int getPIDData(char * dirData, int ip_num);
int getFILEData(char * dirData , int ip_num);
int getFDdata(char * dirData, int ip_num);



int addDots(char * dirData, int dataSize, int currDirInum,int minor);
void printBuffer (char * dirData , int dataSize);
struct dirent generateDir(char * name, int inum);
char * num2type(int type);
char * num2state(int state);

void writeString(char * dirData ,char * text ,int * dataWritten);
void writeNumber(char * dirData ,int number ,int * dataWritten);
void writeStatus(char * dirData, struct proc * process, int * dataWritten);
void writeBlockstat (char * dirData, struct blockstatData blocksCacheData, int * dataWritten);
void writeInodestat (char * dirData, struct inodeStatData inodesCacheData, int * dataWritten);
void writeSingleFdData (char * dirData,struct proc * process, int * dataWritten,int file_type);




#define min(a, b) ((a) < (b) ? (a) : (b))

#define DOUBLEDOT 297
#define BLOCKSTAT_NUM 298
#define INODESTAT_NUM 299 
#define PROC_INUM 300
#define PROCESS_MULTIPLY (5+NOFILE) 


char * fileNames[5] = {"blockstat", "inodestat", "cwd", "status", "fdinfo"};

enum fileTypeOffset {CWD_NUM, STATUS_NUM, FDINFO_NUM};
enum fileMinor {PROC, FILE, PID, FDINFO_DIR};				// Enum for file types (minor)





/******************************************************************************************************************/
/*************************** 	Get /proc directory data (store into char * dirData)	***************************/


int
getPROCData(char * dirData)
{
	int i = 0;
	int dataWritten = 0;
	struct dirent dir;
	int pids[NPROC];
	char itoaBuff[20];
	char * convertedItoaName = 0;
	getPIDs(pids);					// Insert all living processes PIDs into 'pids' array

	while(pids[i] != -1)
	{
		// Set 'dir' name as the PID of the process
		convertedItoaName = itoa( itoaBuff,pids[i]);
		dir = generateDir(convertedItoaName, PROC_INUM + pids[i]);

		// Copy data from dir (in size of dirent) into dirData + current offset, and update 'dataWritten'
		memmove(dirData + (i * sizeof(struct dirent)), &dir, sizeof(struct dirent));
		dataWritten += sizeof(dir);	
		i++;	
	}

	dir = generateDir(fileNames[0], BLOCKSTAT_NUM);
	memmove(dirData + (i * sizeof(struct dirent)), &dir, sizeof(struct dirent));
	dataWritten += sizeof(dir);	
	i++;

	dir = generateDir(fileNames[1], INODESTAT_NUM);
	memmove(dirData + (i * sizeof(struct dirent)), &dir, sizeof(struct dirent));
	dataWritten += sizeof(dir);	

	// Return number of bytes written into 'dirData'
	return dataWritten;
}





/******************************************************************************************************************/
/************************* 	Get /proc/PID directory data (store into char * dirData)	***************************/


int
getPIDData(char * dirData, int ip_num)
{
	int dataWritten = 0;
	struct dirent dir;
	int inumRange = ip_num * PROCESS_MULTIPLY;
	for(int i = 0; i < 3; i++)
	{
		memmove(dir.name ,fileNames[i + 2], strlen(fileNames[i + 2]));
		dir.name[strlen(fileNames[i+2])] = '\0';
		if(i == 0)
		{	
			struct proc * process = getProcess(ip_num - PROC_INUM);
			struct inode * cwd = process->cwd;
			dir.inum = cwd->inum;
		}
		else
		{
			dir.inum = inumRange + i;	
		}
		memmove(dirData + (i * sizeof(struct dirent)), &dir, sizeof(struct dirent));
		dataWritten += sizeof(dir);	
	}
	return dataWritten;

}

/******************************************************************************************************************/
/************************* 	Get FDINFO data for PID (store into char * dirData)	***************************/


int
getFDdata(char * dirData, int ip_num)
{
	int dataWritten = 0;
	struct dirent dir;
	int pid;
	char itoaBuff[20];

	if(ip_num >= PROC_INUM)
	{
		pid = (ip_num / PROCESS_MULTIPLY) - PROC_INUM;
	}
	else
	{
		cprintf("getFDdata: ip_num does not represent a pid\n");
		return 0;
	}
	
	struct proc * process = getProcess(pid);

	// Present directory for every process' open file (name of directory is the file descriptor)
	for (int i = 0; i < NOFILE; i++)
	{
		if (process->ofile[i] != 0)
		{
			char * fdItoa = itoa( itoaBuff,i);
			dir = generateDir(fdItoa , ip_num + i + 1); 	//ip num is '(ip_num + i + 1)' because ip_num is fdinfo's num. hence we start counting from ip_num +1.
			memmove(dirData + (i * sizeof(struct dirent)), &dir, sizeof(struct dirent));
			dataWritten += sizeof(dir);	
		}
	}
	return dataWritten;
}




/******************************************************************************************************************/
/************************* 	Get FILE data (store into char * dirData)	***************************/



int
getFILEData(char * dirData , int ip_num)
{
	struct proc * process;
	struct blockstatData blocksCacheData;
	struct inodeStatData inodesCacheData;
	int  dataWritten = 0;
	int file_type , pid;
	if(ip_num >= PROC_INUM)
	{
		pid = (ip_num / PROCESS_MULTIPLY) - PROC_INUM;
		file_type = ip_num % PROCESS_MULTIPLY;
	}
	else
	{
		file_type = ip_num;
	}	

	switch(file_type)
	{
		// status file include process' run state and its memory usage (proc->sz)."
		case STATUS_NUM: 
			process = getProcess(pid);
			writeStatus(dirData, process, &dataWritten);
			break;	

		// blockstat file include: Free blocks number, total blocks number, and hit/total accesses ratio
		case BLOCKSTAT_NUM: 
			blocksCacheData = getBlockstatData();
			writeBlockstat(dirData, blocksCacheData, &dataWritten);
			break;


		case INODESTAT_NUM: 
			inodesCacheData = getinodeStatData();
			writeInodestat(dirData, inodesCacheData, &dataWritten);
			break;	

		default: 

			if(file_type > 2) //then the file type is file descriptor file. i.e this is fd in FDINFO directory 
			{	
				process = getProcess(pid);
				writeSingleFdData(dirData, process, &dataWritten, file_type);
			}
			break;

	}
		
	return dataWritten;
}



/******************************************************************************************/
/*************************** 	Add dots (".", "..") to data	***************************/


int
addDots(char * dirData, int dataSize, int currDirInum,int minor)
{
	int inum = dataSize / sizeof(struct dirent);			// Get last inum in dirData
	int newDataSize = dataSize;
	int doubleDotInum = 0;

	// Add the "." directory to dirData
	struct dirent dot;
	dot = generateDir(".", currDirInum);
	memmove(dirData + (inum * sizeof(struct dirent)), &dot, sizeof(struct dirent));
	newDataSize += sizeof(dot);

	// Add the ".." directory to dirData
	struct dirent doubleDot;
	switch(minor)
	{
		case PROC:
			doubleDotInum = 1;
			break;
		
		case PID:
			doubleDotInum = DOUBLEDOT;
			break;
		
		case FDINFO_DIR:
			doubleDotInum = (currDirInum - 2) / PROCESS_MULTIPLY;
			break;
	}

	doubleDot = generateDir("..", doubleDotInum);
	memmove(dirData + ((inum + 1) * sizeof(struct dirent)), &doubleDot, sizeof(struct dirent));
	newDataSize += sizeof(doubleDot);
	return newDataSize;
}


/**********************************************************************************************/
/******************************** 	Generate psuedo dir	***************************************/


struct dirent
generateDir(char * name, int inum)
{
	struct dirent dir;
	for (int i=0; i < 14; i++)
	{
		dir.name[i] = 0;
	}
	
	memmove(&dir.name, name, strlen(name)+1);
	dir.inum = inum;
	return dir;
}



/**********************************************************************************************/
/*************************** 	Printing the dirData for debugging	***************************/


void 
printBuffer (char * dirData , int dataSize)
{
	struct dirent dir;
	int iters = dataSize / sizeof(struct dirent);
	cprintf("\n ----------------- PrintBuffer -----------------\n");
	for(int i = 0; i < iters; i++)
	{
		memmove(&dir ,dirData + i * sizeof(struct dirent) , sizeof(dir));
		cprintf("PrintBuffer : #%d dirent name : %s , dirent inum : %d\n", i+1 , dir.name , dir.inum);
	}
	cprintf("\n");
}



/**********************************************************************************************/
/*************************** 	Writing to buffer functions	***********************************/


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



void
writeStatus(char * dirData, struct proc * process, int * dataWritten)
{
	writeString(dirData, "\n", dataWritten);
	writeString(dirData, "Process ", dataWritten);
	writeNumber(dirData, process->pid, dataWritten);
	writeString(dirData," status:\n",dataWritten);

	writeString(dirData, "Run state = ", dataWritten);
	writeString(dirData, num2state(process->state), dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "Memory usage = ", dataWritten);
	writeNumber(dirData, process->sz, dataWritten);	
	writeString(dirData, "\n\n", dataWritten);
}



void
writeBlockstat (char * dirData, struct blockstatData blocksCacheData, int * dataWritten)
{
	writeString(dirData, "\nFree blocks: ", dataWritten);
	writeNumber(dirData, blocksCacheData.freeBlocks, dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "Total blocks: ", dataWritten);
	writeNumber(dirData, blocksCacheData.totalBlocks, dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "Hit ratio: ", dataWritten);
	writeNumber(dirData, blocksCacheData.numOfHits, dataWritten);
	writeString(dirData, " / ", dataWritten);
	writeNumber(dirData, blocksCacheData.totalNumOfBlockAccs, dataWritten);
	writeString(dirData, "\n\n", dataWritten);
}



void
writeInodestat (char * dirData, struct inodeStatData inodesCacheData, int * dataWritten)
{
	writeString(dirData, "\nFree inodes: ", dataWritten);
	writeNumber(dirData, inodesCacheData.freeInodes, dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "Valid inodes: ", dataWritten);
	writeNumber(dirData, inodesCacheData.validInodes, dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "Refs per inode: ", dataWritten);
	writeNumber(dirData, inodesCacheData.totalNumOfRefs, dataWritten);
	writeString(dirData, " / ", dataWritten);
	writeNumber(dirData, inodesCacheData.numOfUsedInodes, dataWritten);
	writeString(dirData, "\n\n", dataWritten);
}


void
writeSingleFdData (char * dirData,struct proc * process, int * dataWritten,int file_type)
{
	int fd_number = file_type - 3; //Subtraction by 3 . because this is the offset of fd
	struct file * file = process->ofile[fd_number];

	writeString(dirData, "\nFile type: ", dataWritten);
	writeString(dirData, num2type(file->type), dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "File position: ", dataWritten);
	writeNumber(dirData, file->off, dataWritten);
	writeString(dirData, "\n", dataWritten);

	writeString(dirData, "File flags: ", dataWritten);
	writeString(dirData, "\n     ", dataWritten);
	writeString(dirData, "Readable bit: ", dataWritten);
	if(file->readable) {writeString(dirData, "ON", dataWritten);}
	else{writeString(dirData, "OFF", dataWritten);}
	writeString(dirData, "\n     ", dataWritten);
	writeString(dirData, "Writable bit: ", dataWritten);
	if(file->writable) {writeString(dirData, "ON", dataWritten);}
	else{writeString(dirData, "OFF", dataWritten);}
	writeString(dirData, "\n", dataWritten);


	struct inode * ip = file->ip;

	writeString(dirData, "File Inode number: ", dataWritten);
	writeNumber(dirData, ip->inum, dataWritten);	
	writeString(dirData, "\n", dataWritten);	


	writeString(dirData,"Reference count: ", dataWritten);
	writeNumber(dirData, file->ref, dataWritten);	
	writeString(dirData, "\n\n", dataWritten);	
}


/**********************************************************************************************/
/***************** 	Convert number indicating process state to string	***********************/


char * num2state(int state)
{
	switch(state)
	{
		case 0: return "UNUSED";
		case 1: return "EMBRYO";
		case 2: return "SLEEPING";
		case 3: return "RUNNABLE";
		case 4: return "RUNNING";
		case 5: return "ZOMBIE";
	}
	return "";
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



/*************************** 	procfs IS DIRECTORY - check if ip represents a directory 	***************************/
// ip - ip representing the file

/* If a directory, return non-zero value */

int 
procfsisdir(struct inode *ip) 
{
	if (ip->minor == FILE)
	{
		return 0;
	}
	return 1;					
}



/*************************** 	procfs iread 	***************************/

// Initialized ip->inum and initialized dp that represents ip’s parent directory. 
// This function can update all ip fields.
// This function determines ip->minor according to the dp->minor. For example: /proc/PID/status, if dp->PID, than ip->FILE

// command: ls /proc

void 
procfsiread(struct inode* dp, struct inode *ip) 
{	
	ip->type = T_DEV;								// Set as T_DEV
	ip->major = PROCFS;								// Set ip->major to PROCFS to indicate virtual framework usage
	ip->flags = ip->flags | I_VALID;				// Set I_VALID bit on (to avoid accidently reading from disk)
	ip->size = 0;									// Set not-important size value
	switch(dp->minor)
	{
		// dp->minor is '/proc'
		case PROC:

			// If inum > FILE_INUM (999) than we know ip represents a file (blockstat, inodestat)
			if (ip->inum >= PROC_INUM) 
			{		
				ip->minor = PID;
			}
			else if(ip->inum == DOUBLEDOT)
			{
				ip->inum = 1; //returning to root dir
			}
			else
			{
				ip->minor = FILE;	
			}
			break;


		// dp->minor is '/proc/some file' or '/proc/PID/some file'
		case FILE:

			ip->minor = dp->minor;
			break;


		// dp->minor is '/proc/some PID'
		case PID:
			
			if (ip->inum == DOUBLEDOT)
			{
				
				ip->minor = PROC;
			}
			else if ((ip->inum % PROCESS_MULTIPLY) == FDINFO_NUM)
			{
				
				ip->minor = FDINFO_DIR;				
			} 

			else
			{
				
				ip->minor = FILE;
			}
			break;


		// dp->minor is '/proc/pid/fdinfo'
		case FDINFO_DIR:

			if(ip->inum < 4000)
			{
			
				ip->minor = PID;
			}
			else
			{	
				
				ip->minor = FILE;
			}
			break;	

		default:
			ip->minor = dp->minor;
			break;
	}
}



/*************************** 	procfs READ 	***************************/

// ip - ip representing the file
// dst - buffer destination to read to
// off - offset of file to read from
// n - number of bytes to read from file

// procfsread is called as dervsw[].read function

int
procfsread(struct inode *ip, char *dst, int off, int n) 
{																	
	char dirData[512];									// Pseudo directory level data to store according to current command path traversal
	uint dataSize = 0;									// Store the size of data inserted into dirData in order to check with offset
	int diff;											// diff = dataSize - offset --> Check that offset is within our data boundaries
	

	//("ip->minor is %d , inode num : %d\n",  ip->minor, ip->inum);
	switch(ip->minor)
	{
		// ip->minor is '/proc'
		case PROC:
			dataSize = getPROCData(dirData);
			break;

		// ip->minor is '/proc/PID'
		case PID:
			dataSize = getPIDData(dirData, ip->inum);
			break;

		// ip->minor is '/proc/PID/something' or '/proc/something'
		case FILE:
			dataSize = getFILEData(dst, ip->inum);	
			break;

		// ip->minor is '/proc/PID/fdinfo'
		case FDINFO_DIR:
			dataSize = getFDdata(dirData, ip->inum);
			break;
	
	}	

	if (ip->minor != FILE)
	{
		dataSize = addDots(dirData, dataSize , ip->inum , ip->minor);

		// If offset is within the data size, there is data to store in 'dst'
		diff = dataSize - off; 
		if (diff > 0)										
		{	
			memmove(dst, dirData + off, min(diff, n));				// If diff > n, move n bytes into dst
			return min(diff, n);
		}
		else
		{	
			return 0;
		}
	}
	else
	{	
		if(off == 0)
			return dataSize;
		else
			return 0;
	}
}



/*************************** 	procfs WRITE - no need to write in this virtual device (read-only framework) !!!	***************************/

int
procfswrite(struct inode *ip, char *buf, int n)
{
	return 0;
}


/*************************** 	procfs Initialization with functions	***************************/


void
procfsinit(void)
{
	devsw[PROCFS].isdir = procfsisdir;
	devsw[PROCFS].iread = procfsiread;
	devsw[PROCFS].write = procfswrite;
	devsw[PROCFS].read = procfsread;
}

