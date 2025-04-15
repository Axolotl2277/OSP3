//Project 4 for CMP_SCI-4760 Spring 2025 for Logan Bessinger, 3/15/2025
//Outputs a string that is mostly the same execpt for every instance of it
//Takes two ints as argv, 1st for secounds, 2nd for nanoseconds and after that ammount of time passes, it ends itself
//outputs its pid, ppid, the current time, and time it ends
//outputs once while starting, everytime oss sends a message, and once when exiting
//time is not system time but oss time. 
#include <iostream>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define PERMS 0644
typedef struct msgbuffer {
long mtype;
char strData[100];
int intData;
} msgbuffer;


#define SHMKEY1 02262025
#define SHMKEY2 02262026
#define BUFF_SZ sizeof(int)

int shmid1 = shmget (SHMKEY1, BUFF_SZ, 0777);
int shmid2 = shmget (SHMKEY2, BUFF_SZ, 0777);

void signal_handler(int sig) {

   printf("If viewing, error has accoured. Ending Worker \n");
   
  exit(1);
}



int main(int argc, char** argv)
{
//I know that the oss should handle terminating the child processes
//but if like John Conner they cannot be terminated for some reason then this ensures the process will end.
  signal(SIGALRM, signal_handler);
  alarm(90);
	int Sec_To_Kill = atoi(argv[1]);
  int Nano_Sec_To_Kill = atoi(argv[2]);
  int * Nano_Seconds_Reader = ( int * )( shmat ( shmid1, 0, 0 ) );
  int * Seconds_Reader = ( int * )( shmat ( shmid2, 0, 0 ) );
  int start_Time_Sec = *Seconds_Reader;
  int start_Time_Nano = *Nano_Seconds_Reader;
  int sec_Current = start_Time_Sec;
  int sec_Count = 0;
  int loop_Counter = 0;
  int done_Check = 1;
  int Sec_Worked = 0;
  int Nano_Worked = 0;    
  int Current_Work;
  int Nano_Diff;
  int Sec_Diff;
  int Action_Option;
  
  msgbuffer buf;
  buf.mtype = 1;
  int msqid = 0;
  key_t key;
  
  if ((key = ftok("msgq.txt", 1)) == -1) {
  perror("ftok");
  exit(1);
  }

  if ((msqid = msgget(key, PERMS)) == -1) {
  perror("msgget in child");
  exit(1);
  }

  printf("Child %d has access to the queue\n",getpid());   
	do {
   
   
    if ( msgrcv(msqid, &buf, sizeof(msgbuffer), getpid(), 0) == -1) {
    perror("failed to receive message from parent\n");
    exit(1);
    }
    Current_Work = buf.intData;
    //std::cout<<"Working for: "<< Current_Work<<" Nano Seconds\n";
    srand (*Nano_Seconds_Reader);
   Action_Option = rand() % 100 + 1;
   //std::cout<<"Current Action: "<< Action_Option<<"\n";
    
    if (Action_Option > 90)
    {
    //end option
    done_Check = 0;
    Action_Option = rand() % Current_Work + 1;
    Current_Work = Current_Work - Action_Option;
    }
    else if (Action_Option < 10)
    {
    //interupt Option
    Action_Option = rand() % Current_Work + 1;
    Current_Work = Current_Work - Action_Option;
    }
    else
    {
    //used all time option
    }
    
    Nano_Worked = Nano_Worked + Current_Work;
    if (Nano_Worked >= 1000000000){
    Nano_Worked = Nano_Worked - 1000000000;
    Sec_Worked++;
    }
    
    buf.mtype = getppid();
    if (done_Check == 0){
    buf.intData = Current_Work * -1;
    }
    else{
    buf.intData = Current_Work;
    }
    strcpy(buf.strData,"Message back to muh parent ");
	  if (msgsnd(msqid,&buf,sizeof(msgbuffer)-sizeof(long),0) == -1) 
      {
      perror("msgsnd to parent failed\n");
      exit(1);
      }
    

  }  while(done_Check == 1);
 
    std::cout<< "Worker PID:" << getpid();
	  std::cout<< " Ending \n";
 
   shmdt(Seconds_Reader);
   shmdt(Nano_Seconds_Reader);
	return getpid();
}
