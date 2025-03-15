//This is Project 2 for CMP_SCI-4760 Spring 2025 for Logan Bessinger, 2/27/2025
//Will take arg -n (for Number) -s (for Simaltainis) -t (for worker self end time) and -i (for time bewteen forking)
//and create n processes every i milliseconds,but no more that s number at a time, and those files will run for up to t seconds.
//created processes are named worker.cpp and take 2 argument other than file name and both are ints
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define SHMKEY1 02262025
#define SHMKEY2 02262026
#define BUFF_SZ sizeof(int)

using namespace std;

#define PERMS 0644
typedef struct msgbuffer {
long mtype;
char strData[100];
int intData;
} msgbuffer;

msgbuffer buf0;
  msgbuffer rcvbuf;
  int msqid;
  key_t key;
  

//set up global vars for setting up and killing worker processess and freeing up memory later
int shmid1 = shmget (SHMKEY1, BUFF_SZ, 0777|IPC_CREAT);
int shmid2 = shmget (SHMKEY2, BUFF_SZ, 0777|IPC_CREAT);
int * oss_Nano_Seconds = ( int * )( shmat ( shmid1, 0, 0 ) );
int * oss_Seconds = ( int * )( shmat ( shmid2, 0, 0 ) );


struct PCB {
	int occupied;
	pid_t pid;
	int start_Seconds;
	int start_Nano;
  int message_Count;
	};
struct PCB process_Table[20];

//fuction to end oss after 60 seconds, used to break out of eternal while loop in main
void signal_handler(int sig) {

   printf("Ending Program \n");
   for(int i = 0; i < 20; i++)
   {
   if(process_Table[i].occupied == 1)
     {
     cout<<"Killing child process: "<<process_Table[i].pid<< "\n";
     kill(process_Table[i].pid, SIGKILL);
     }
   }
   shmdt(oss_Nano_Seconds);
   shmdt(oss_Seconds);
   shmctl(shmid1,IPC_RMID,NULL);
   shmctl(shmid2,IPC_RMID,NULL);
   if (msgctl(msqid, IPC_RMID, NULL) == -1)   
        {
        perror("msgctl to get rid of queue in parent failed");
        exit(1);
        }
  exit(1);
}


int main( int argc, char** argv)
{
//set up signal as soon as possible to avoid it not being set up;
  signal(SIGALRM, signal_handler);
  alarm(60);

	int opt;
	int n_Value = 0;
	int s_Value = 0;
	int t_Value = 0;
	int i_Value = 0;
	int total_Count = 0;
	int running_Count = 0;
  int work_Call_Delta;
  int last_Call_Sec = 0;
  int last_Call_Nano = 0;
	pid_t c_pid;
  int dead_pid = 0;
	int status = 0;
	int random_Sec;
  int random_Nano;
  //control "speed" of process
  int Nano_Add = 250000000; //250 milliseconds to be divided by # of childs later
  string file_name = "defaultFile.txt";
	system("touch msgq.txt");
  
  
  // get a key for our message queue
  if ((key = ftok("msgq.txt", 1)) == -1) {
    perror("ftok");
    exit(1);
  }
  // create our message queue
  if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {
    perror("msgget in parent");
    exit(1);
  }
  
 printf("Parent %d has access to the queue\n",getpid());

	while ((opt = getopt(argc, argv, "n:s:t:i:f:h")) != -1)
	{
	switch (opt)
		{
			case 'n':
				n_Value = atoi(optarg);
				break;
			case 's':
				s_Value =  atoi(optarg);
				break;
			case 't':
				t_Value = atoi(optarg);
				break;
			case 'i':
				i_Value = atoi(optarg);
				break;
      case 'f':
        file_name = optarg;
      break;
			case 'h':
				printf("Called with (./oss -n x -s y -t z -i a -f fileName) where x, y, z and a are ints, and fileName is the name of output file. \n");
				return 0;
		}
	}
	printf("Value check \n");
	//value/input checking
	if (n_Value <= 0){
	printf("N Value not accepted. Ending process. \n");
	return -1;
	}
 //if we only have 20 slots in our table then we should have a max of 20 process active at once
	if (s_Value <= 0 || s_Value > 20){
	printf("S Value not accepted. Ending process. \n");
	return -1;
	}
	if (t_Value <= 0){
	printf("T Value not accepted. Ending process. \n");
	return -1;
	}
	if (i_Value <= 0){
	printf("T Value not accepted. Ending process. \n");
	return -1;
	}
  if (file_name == "defaultFile.txt"){
  printf("No change in file name, please select a name or different name for output file.");
  return -1;
  }
 
 

	//cheching here if creating shared memory worked
  //why here? becuase this is where I first tested it in code and did not want to move it 
	if (shmid1 == -1){
	printf("Error in shmget");
	return -2;
	}
 
	if (shmid2 == -1){
	printf("Error in shmget");
	return -2;
	}

  
    *oss_Nano_Seconds = 0;             
    *oss_Seconds = 0;
    
    //Eternal while loop that will be ended with the signal from earlier
    int wait = 1;
    while (running_Count > 0 || total_Count < n_Value){
    
    if (running_Count == 0)
    {
    *oss_Nano_Seconds = *oss_Nano_Seconds + Nano_Add;
    }
    else
    {
    *oss_Nano_Seconds = *oss_Nano_Seconds + Nano_Add / running_Count;
    }
    if (*oss_Nano_Seconds >= 1000000000){
      *oss_Nano_Seconds = *oss_Nano_Seconds - 1000000000;
      *oss_Seconds = *oss_Seconds + 1;
      }
      
      //I think this puts the Delta into milliseconds if my math is right
    work_Call_Delta = ((*oss_Nano_Seconds - last_Call_Nano) + (*oss_Seconds - last_Call_Sec) * 1000000000) / 1000000;
    //read as if more workers to call & running more is possible & last call greater than call interval, then fork
    if(n_Value > total_Count && s_Value > running_Count && work_Call_Delta >= i_Value)
      {
      c_pid = fork();
      if (c_pid == -1)
		    {
		    printf("Fork You. Fork Fail. \n");
		    return 0;
		    }
      total_Count++;
      running_Count++;
      last_Call_Sec = *oss_Seconds;
      last_Call_Nano = *oss_Nano_Seconds;
      
      
      //table setter, possible race condition between table starting value and when the worker process sees itslef starting
      for (int i = 0; i <20; i++)
      {
      if (process_Table[i].occupied == 0){
      process_Table[i].occupied = 1;
	    process_Table[i].pid = c_pid;
	    process_Table[i].start_Seconds = last_Call_Sec;
      process_Table[i].start_Nano = last_Call_Nano;
      i = 20;
      }
      }
	    }
         
   if (c_pid == 0) 
		{
    srand(*oss_Nano_Seconds);
    random_Sec = rand() % t_Value;
    random_Nano = rand() % 1000000000;
    
		string arg0 = "./worker";
		string arg1 = std::to_string(random_Sec);
    string arg2 = std::to_string(random_Nano);	
		execlp(arg0.c_str(), arg0.c_str(), arg1.c_str(), arg2.c_str(), (char *)0);
		printf("If printed, an error has occured in forking.");
		return -2;
		}
   
   //since I put child pid first and child will become a worker or end before this code, I think it is okay if I do not check pid here.
   
   for (int i = 0; i < 20; i++)
   {
     if (process_Table[i].occupied != 0)
     {
     buf0.mtype = process_Table[i].pid;
     buf0.intData = process_Table[i].pid;
     strcpy(buf0.strData,"Message to Child ");
     
     if (msgsnd(msqid, &buf0, sizeof(msgbuffer)-sizeof(long), 0) == -1) 
        {
        perror("msgsnd to child 1 failed\n");
        exit(1);
        }
        cout<<"Test after send\n";
        
      if (msgrcv(msqid, &rcvbuf,sizeof(msgbuffer), getpid(),0) == -1) 
        {
        perror("failed to receive message in parent\n");
        exit(1);
        }
      process_Table[i].message_Count++;
      
      if (rcvbuf.intData == 0)
        {
        process_Table[i].occupied = 0;
        process_Table[i].pid = 0;
	      process_Table[i].start_Seconds = 0;
        process_Table[i].start_Nano = 0;
        process_Table[i].message_Count = 0;
        running_Count--;
        }
      
     }
   }
    
    
    //table outputter
    if (*oss_Nano_Seconds == 0 || *oss_Nano_Seconds == 500000000)
    {
      cout<<"OSS PID:" << getpid() <<" SysClockS: "<<*oss_Seconds<<" SysclockNano: "<< *oss_Nano_Seconds<< "\n";
      cout<<"Process table: \n";
      cout<<"Entry, Occupied, PID, StartS, StartN, MessagesSent \n";
      for (int i = 0; i <20; i++)
      {
      cout<<i<<", ";
      cout<<process_Table[i].occupied<<", ";
	    cout<<process_Table[i].pid<<", ";
	    cout<<process_Table[i].start_Seconds<<", ";
      cout<<process_Table[i].start_Nano<<", ";
      cout<<process_Table[i].message_Count<<"\n";      
      }
    }
    
    
		
		}
   if (msgctl(msqid, IPC_RMID, NULL) == -1)   
        {
        perror("msgctl to get rid of queue in parent failed");
        exit(1);
        }
  shmdt(oss_Nano_Seconds);
  shmdt(oss_Seconds);
  shmctl(shmid1,IPC_RMID,NULL);
  shmctl(shmid2,IPC_RMID,NULL);
	printf("Ending by no more processes to run. \n");
	return 0;
}
