//This is Project 4 for CMP_SCI-4760 Spring 2025 for Logan Bessinger, 4/15/2025
//Will take arg -n (for Number) -s (for Simaltainis) -t (for worker self end time), -i (for time bewteen forking) and -f (for filename)
// but will default to set vars if not provived correct arguments
//and create n processes every i milliseconds,but no more that s number at a time, and those files will run until they end themselves.
//created processes are named worker.cpp and take 2 argument other than file name and both are ints
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>

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
  int serviceTimeSec;
  int serviceTimeNanoSec;
  int eventWaitSec;
  int eventWaitNanoSec;
  int blocked;
  int message_Count;
	};
struct PCB process_Table[20];

//fuction to end oss after 3 reak world seconds
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
  alarm(3);

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
  string file_name = "defaultFile.txt";
	system("touch msgq.txt");
  int total_Messages = 0;
  int new_Worker_Flag;
  int Flash_Pass[20];
  int Regular[20];
  int Parking_Lot[20];
  int Blocked[20];
  int Current_Worker;
  int Run_Time;
  int Current_Blocked;
  int ALL_BLOCKED;
  int Print_Table_Delta = 500000000;
  string Current_Queue;
  int Queue_Count[4]; //0 for highest, 1 for middle, 2 for lowest, 3 for blocked
  for (int i = 0; i < 4; i++){
  Queue_Count[i] = 0;
  }
  
  
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
	
	if (n_Value <= 0){
	printf("N Value not accepted. Making it 100. \n");
  n_Value = 100;
	}
 //if we only have 20 slots in our table then we should have a max of 20 process active at once
	if (s_Value <= 0 || s_Value > 15){
	printf("S Value not accepted. Making it 18. \n");
	s_Value = 15;
	}
	if (t_Value <= 0){
	printf("T Value not accepted. Making it 1. \n");
	t_Value = 1;
	}
	if (i_Value <= 0){
	printf("T Value not accepted. Making it 10. \n");
	i_Value = 10;
	}
  if (file_name == "defaultFile.txt"){
  printf("No change in file name, using defaultFile.txt \n");
  }
 for (int i = 0; i<20; i++)
 {
 Flash_Pass[i] = -1;
 Regular[i] = -1;
 Parking_Lot[i] = -1;
 Blocked[i] = -1;
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
    
    
    ofstream Working_File(file_name);
    
    while (running_Count > 0 || total_Count < n_Value){
    
    for (int i = 0; i <20; i++)
    {
    if (Blocked[i] != -1)
    {
    Current_Blocked = Blocked[i];
      if (process_Table[Current_Blocked].eventWaitSec <= *oss_Seconds) 
      {
      //testing for both so if nano seconds reset to 0 it will still see to end.
        if(process_Table[Current_Blocked].eventWaitSec + 1 <=  *oss_Seconds || process_Table[Current_Blocked].eventWaitNanoSec < *oss_Nano_Seconds)
          { 
          process_Table[Current_Blocked].blocked = 0;
          Blocked[i] = -1;
          for(int j = 0; j <20; j++)
          {
          if (Flash_Pass[j] == -1)
            {
            Flash_Pass[j] = Current_Blocked;
            j = 20;
            Queue_Count[0]++;
            }
          }
          }
      }
    }
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
            Working_File<<"Process failed to fork. Maybe switch to a spoon. \n\n";
            Working_File.close();
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
      cout<<"OSS: Generating process: "<<i<<" with PID: "<<c_pid<<" at time "<<*oss_Seconds<<":"<<*oss_Nano_Seconds<<"\n";
      process_Table[i].occupied = 1;
	    process_Table[i].pid = c_pid;
	    process_Table[i].start_Seconds = last_Call_Sec;
      process_Table[i].start_Nano = last_Call_Nano;
      for(int j = 0; j <20; j++)
          {
          if (Flash_Pass[j] == -1)
            {
            Flash_Pass[j] = i;
            j = 20; 
            }
          }
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
   
   
   
   //send/recive message to/from child
  if (Flash_Pass[0] != -1)
  {
  Current_Worker = Flash_Pass[0];
  for(int i = 0; i < 19; i++)
    {
    Flash_Pass[i] = Flash_Pass[i+1];
    }
  Flash_Pass[19] = -1;
  Run_Time = 20000000;
  Current_Queue = "Highest";
  }
  else if (Regular[0] != -1)
  {
  Current_Worker = Regular[0];
  for(int i = 0; i < 19; i++)
    {
    Regular[i] = Regular[i+1];
    }
  Regular[19] = -1;
  Run_Time = 40000000;
  Current_Queue = "Middle";
  }
  else if (Parking_Lot[0] != -1)
  {
  Current_Worker = Parking_Lot[0];
  for(int i = 0; i < 19; i++)
    {
    Parking_Lot[i] = Parking_Lot[i+1];
    }
  Parking_Lot[19] = -1;
  Run_Time = 80000000;
  Current_Queue = "Lowest";
  }
  else
  {
  *oss_Nano_Seconds = *oss_Nano_Seconds + 10000000;
          if (*oss_Nano_Seconds >= 1000000000)
            {
            *oss_Nano_Seconds = *oss_Nano_Seconds - 1000000000;
            *oss_Seconds = *oss_Seconds + 1;
            }
  Run_Time = -1;
  }
  
  
  if(Run_Time > 0){
     if (process_Table[Current_Worker].occupied != 0)
     {
     buf0.mtype = process_Table[Current_Worker].pid;
     buf0.intData = Run_Time;
     strcpy(buf0.strData,"Message to Child ");
     
     cout<<"OSS: Dispatching worker "<<Current_Worker;
     cout<<" PID "<<process_Table[Current_Worker].pid<<" from "<<Current_Queue<<" queue at time "<<*oss_Seconds<<":"<<*oss_Nano_Seconds<<"\n";
     if (total_Messages < 10000){
     Working_File<<"OSS: Dispatching worker "<<Current_Worker;
     Working_File<<" PID "<<process_Table[Current_Worker].pid<<" from "<<Current_Queue<<" queue at time "<<*oss_Seconds<<":"<<*oss_Nano_Seconds<<"\n";
     }
     *oss_Nano_Seconds = *oss_Nano_Seconds + 1000;
          if (*oss_Nano_Seconds >= 1000000000)
            {
            *oss_Nano_Seconds = *oss_Nano_Seconds - 1000000000;
            *oss_Seconds = *oss_Seconds + 1;
            }
     cout<<"OSS: Dispatch took 1000 nanoseconds"<<"\n";
     Working_File<<"OSS: Dispatch took 1000 nanoseconds"<<"\n";
     
     
     if (msgsnd(msqid, &buf0, sizeof(msgbuffer)-sizeof(long), 0) == -1) 
        {
        perror("msgsnd to child 1 failed\n");
        exit(1);
        }
        
    
     
      if (msgrcv(msqid, &rcvbuf,sizeof(msgbuffer), getpid(),0) == -1) 
        {
        perror("failed to receive message in parent\n");
        exit(1);
        }
      process_Table[Current_Worker].message_Count++;
      total_Messages++;
      //if child is done clear data from table and output message
      if (rcvbuf.intData < 0)
        {
        cout<<"OSS: Worker "<<Current_Worker<<" PID "<<process_Table[Current_Worker].pid<<" ran for "<<rcvbuf.intData * -1<<" and terminated \n";
        if (total_Messages < 10000){
        Working_File<<"OSS: Worker "<<Current_Worker<<" PID "<<process_Table[Current_Worker].pid<<" ran for "<<rcvbuf.intData * -1<<" and terminated. \n";
        }
        *oss_Nano_Seconds = *oss_Nano_Seconds + rcvbuf.intData * -1;
        if (*oss_Nano_Seconds >= 1000000000)
          {
          *oss_Nano_Seconds = *oss_Nano_Seconds - 1000000000;
          *oss_Seconds = *oss_Seconds + 1;
          }
        process_Table[Current_Worker].occupied = 0;
        process_Table[Current_Worker].pid = 0;
	      process_Table[Current_Worker].start_Seconds = 0;
        process_Table[Current_Worker].start_Nano = 0;
        process_Table[Current_Worker].message_Count = 0;
        process_Table[Current_Worker].serviceTimeSec = 0;
        process_Table[Current_Worker].serviceTimeNanoSec = 0;
	      process_Table[Current_Worker].eventWaitSec = 0;
        process_Table[Current_Worker].blocked = 0;
        process_Table[Current_Worker].message_Count = 0;
        running_Count--;
        }
        else
       {
        *oss_Nano_Seconds = *oss_Nano_Seconds + rcvbuf.intData;
          if (*oss_Nano_Seconds >= 1000000000)
            {
            *oss_Nano_Seconds = *oss_Nano_Seconds - 1000000000;
            *oss_Seconds = *oss_Seconds + 1;
            }
          
          if (rcvbuf.intData < Run_Time && rcvbuf.intData > 0)
            {
            process_Table[Current_Worker].serviceTimeNanoSec = process_Table[Current_Worker].serviceTimeNanoSec + rcvbuf.intData;
          if (process_Table[Current_Worker].serviceTimeNanoSec >=1000000000)
          {
          process_Table[Current_Worker].serviceTimeNanoSec = process_Table[Current_Worker].serviceTimeNanoSec - 1000000000;
          process_Table[Current_Worker].serviceTimeSec++;
          }
            cout<<"OSS: Worker "<<Current_Worker<<" PID "<<process_Table[Current_Worker].pid<<" ran for "<<rcvbuf.intData<<" and was blocked.";
            if (total_Messages < 10000){
            Working_File<<"OSS: Worker "<<Current_Worker<<" PID "<<process_Table[Current_Worker].pid<<" ran for "<<rcvbuf.intData<<" and was blocked.";
            }
            process_Table[Current_Worker].blocked = 1;
            process_Table[Current_Worker].eventWaitSec = *oss_Seconds + 2;
            process_Table[Current_Worker].eventWaitNanoSec = *oss_Nano_Seconds;
              for (int i = 0; i <20; i++)
              {
                if (Blocked[i] == -1)
                  {
                  Blocked[i] = Current_Worker;
                  i = 20;
                  Queue_Count[3]++;
                  }
              }
            }
            else
            {
            process_Table[Current_Worker].serviceTimeNanoSec = process_Table[Current_Worker].serviceTimeNanoSec + rcvbuf.intData;
            if (process_Table[Current_Worker].serviceTimeNanoSec >=1000000000)
            {
            process_Table[Current_Worker].serviceTimeNanoSec = process_Table[Current_Worker].serviceTimeNanoSec - 1000000000;
            process_Table[Current_Worker].serviceTimeSec++;
            }
            cout<<"OSS: Worker "<<Current_Worker<<" PID "<<process_Table[Current_Worker].pid<<" ran for "<<rcvbuf.intData<<" and entered";
            if (total_Messages < 10000){
            Working_File<<"OSS: Worker "<<Current_Worker<<" PID "<<process_Table[Current_Worker].pid<<" ran for "<<rcvbuf.intData<<" and entered";
            }
              if (Run_Time == 20000000)
              {
              cout<<" Middle Queue\n";
              if (total_Messages < 10000){
              Working_File<<" Middle Queue\n";
              }
                for (int i = 0; i <20; i++)
                {
                  if (Regular[i] == -1)
                    {
                    Regular[i] = Current_Worker;
                    i = 20;
                    Queue_Count[1]++;
                    }
                }
              }
              else if (Run_Time == 40000000)
              {
              cout<<" Lowest Queue\n";
              if (total_Messages < 10000){
              Working_File<<" Lowest Queue\n";
              }
                for (int i = 0; i <20; i++)
                {
                  if (Parking_Lot[i] == -1)
                    {
                    Parking_Lot[i] = Current_Worker;
                    i = 20;
                    Queue_Count[2]++;
                    }
                }
              }
              else
              {
              cout<<" Lowest Queue\n";
              if (total_Messages < 10000){
              Working_File<<" Lowest Queue\n";
              }
                for (int i = 0; i <20; i++)
                {
                  if (Parking_Lot[i] == -1)
                    {
                    Parking_Lot[i] = Current_Worker;
                    i = 20;
                    Queue_Count[2]++;
                    }
                }
              }
            }
          }
   }
    
    if (rcvbuf.intData >0)
    {
    Print_Table_Delta = Print_Table_Delta - 1000 - rcvbuf.intData;
    }
    else
    {
    Print_Table_Delta = Print_Table_Delta - 1000 - rcvbuf.intData * -1;
    }
    //table outputter
    if (Print_Table_Delta <= 0)
    {
    Print_Table_Delta = 500000000;
      cout<<"OSS PID:" << getpid() <<" SysClockS: "<<*oss_Seconds<<" SysclockNano: "<< *oss_Nano_Seconds<< "\n";
      cout<<"Process table: \n";
      cout<<"Entry, Occupied, PID, StartS, StartN, SecWorked, NanoWorked, Blocked, MessagesSent \n";
   
   if (total_Messages < 10000){
      Working_File<<"OSS PID:" << getpid() <<" SysClockS: "<<*oss_Seconds<<" SysclockNano: "<< *oss_Nano_Seconds<< "\n";
      Working_File<<"Process table: \n";
      Working_File<<"Entry, Occupied, PID, StartS, StartN, SecWorked, NanoWorked, Blocked, MessagesSent \n";
      }
      
      
      for (int i = 0; i <20; i++)
      {
      if (process_Table[i].occupied == 1)
      {
      cout<<i<<", ";
      cout<<process_Table[i].occupied<<", ";
      cout<<process_Table[i].pid<<", ";
      cout<<process_Table[i].start_Seconds<<", "; 
      cout<<process_Table[i].start_Nano<<", ";
      cout<<process_Table[i].serviceTimeSec<<", ";
      cout<<process_Table[i].serviceTimeNanoSec<<", ";
      cout<<process_Table[i].eventWaitSec<<", ";
      cout<<process_Table[i].blocked<<", ";
      cout<<process_Table[i].message_Count<<"\n";
      if (total_Messages < 10000){
      Working_File<<i<<", ";
      Working_File<<process_Table[i].occupied<<", ";
      Working_File<<process_Table[i].pid<<", ";
      Working_File<<process_Table[i].start_Seconds<<", "; 
      Working_File<<process_Table[i].start_Nano<<", ";
      Working_File<<process_Table[i].serviceTimeSec<<", ";
      Working_File<<process_Table[i].serviceTimeNanoSec<<", ";
      Working_File<<process_Table[i].eventWaitSec<<", ";
      Working_File<<process_Table[i].blocked<<", ";
      Working_File<<process_Table[i].message_Count<<"\n";
      }
      }
      }
      
      if (total_Messages < 10000){
      cout<<"Highest Queue: |";
      Working_File<<"Highest Queue: |";
      for (int i = 0; i <20; i++)
      {
      if (Flash_Pass[i] == -1)
        {
        cout<<"X";
        Working_File<<"X";
        }
      else 
        {
        cout<<Flash_Pass[i];
        Working_File<<Flash_Pass[i];;
        }
      }
      cout<<"|\n";
      cout<<"Regular Queue: |";
      Working_File<<"|\n";
      Working_File<<"Regular Queue: |";
      for (int i = 0; i <20; i++)
      {
      if (Regular[i] == -1)
        {
        cout<<"X";
        Working_File<<"X";
        }
      else 
        {
        cout<<Regular[i];
        Working_File<<Regular[i];
        }
      }
      cout<<"|\n";
      cout<<"Lowest Queue:  |";
      Working_File<<"|\n";
      Working_File<<"Lowest Queue:  |";
      for (int i = 0; i <20; i++)
      {
      if (Parking_Lot[i] == -1)
        {
        cout<<"X";
        Working_File<<"X";
        }
      else 
        {
        cout<<Parking_Lot[i];
        Working_File<<Parking_Lot[i];
        }
      }
      cout<<"|\n";
      cout<<"Blocked Queue: |";
      Working_File<<"|\n";
      Working_File<<"Blocked Queue: |";
      for (int i = 0; i <20; i++)
      {
      if (Blocked[i] == -1)
        {
        cout<<"X";
        Working_File<<"X";
        }
      else 
        {
        cout<<Blocked[i];
        Working_File<<Blocked[i];
        }
      }
      cout<<"|\n";
      Working_File<<"|\n";
    }
    }//
		}
   
   
            
   }
   //close everything and retire for the night
   if (msgctl(msqid, IPC_RMID, NULL) == -1)   
        {
        perror("msgctl to get rid of queue in parent failed");
        exit(1);
        }
        
        cout<<"Final output\n";
        cout<<"Times Highest Queue entered:"<<Queue_Count[0]<<"\n";
        cout<<"Times Middle Queue entered:"<<Queue_Count[1]<<"\n";
        cout<<"Times Lowest Queue entered:"<<Queue_Count[2]<<"\n";
        cout<<"Times Blocked Queue entered:"<<Queue_Count[3]<<"\n";
        
        
      Working_File<<"Final output\n";
  shmdt(oss_Nano_Seconds);
  shmdt(oss_Seconds);
  shmctl(shmid1,IPC_RMID,NULL);
  shmctl(shmid2,IPC_RMID,NULL);
  Working_File.close();
	printf("Ending by no more processes to run. \n");
	return 0;
}
