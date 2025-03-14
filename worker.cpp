//Project 2 for CMP_SCI-4760 Spring 2025 for Logan Bessinger, 2/27/2025
//Outputs a string that is mostly the same execpt for every instance of it
//Takes two ints as argv, 1st for secounds, 2nd for nanoseconds and after that ammount of time passes, it ends itself
//outputs its pid, ppid, the current time, and time it ends
//outputs once while starting, everytime secounds changes, and once when exiting
//time is not system time but oss time. 
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>



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
  int End_Sec = Sec_To_Kill + start_Time_Sec;
  int End_Nano = Nano_Sec_To_Kill + start_Time_Nano;
  int sec_Current = start_Time_Sec;
  int sec_Count = 0;

  //Logic to ensure end nano is not greater than legal nano value, otherwise while loop later does not work
  if (End_Nano >= 1000000000){
  End_Nano = End_Nano - 1000000000;
  End_Sec++;
  }

  
  std::cout<< "Worker PID:" << getpid();
	std::cout<< " PPID:" << getppid();
  std::cout<< " Sysclock: " << start_Time_Sec;
  std::cout<< " SysclockNano: " << start_Time_Nano;
  std::cout<< " TermTimeS: " <<End_Sec;
  std::cout<< " TermTimeNano: " <<End_Nano << "\n";
  std::cout<< "--Just Starting \n";
    
    
    //read as while less than end time, keep working
	while(End_Sec > *Seconds_Reader || End_Nano > *Nano_Seconds_Reader){
	  if (sec_Current != *Seconds_Reader){
        sec_Current = *Seconds_Reader;
        sec_Count++;
        std::cout<< "Worker PID:" << getpid();
	      std::cout<< " PPID:" << getppid();
        std::cout<< " Sysclock: " << *Seconds_Reader;
        std::cout<< " SysclockNano: " << *Nano_Seconds_Reader;
        std::cout<< " TermTimeS: " <<End_Sec;
        std::cout<< " TermTimeNano: " <<End_Nano << "\n";
        std::cout<< "--"<<sec_Count<<" seconds have passed since starting \n";
    }
	} 
 
    std::cout<< "Worker PID:" << getpid();
	  std::cout<< " PPID:" << getppid();
    std::cout<< " Sysclock: " << *Seconds_Reader;
    std::cout<< " SysclockNano: " << *Nano_Seconds_Reader;
    std::cout<< " TermTimeS: " <<End_Sec;
    std::cout<< " TermTimeNano: " <<End_Nano << "\n";
    std::cout<< "--Terminating \n";
 
   shmdt(Seconds_Reader);
   shmdt(Nano_Seconds_Reader);
	return getpid();
}
