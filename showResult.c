/**
 * showResult.c -> SERVER(sadece sonuclari yazan server)
 * 
 * Gozde Dogan 131044019
 * System programming MIDTERM
 * 17 April, 2017
 *
 * References:
 *      1. random fonksiyonuna bakmak icin;
 *      https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
 *           
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h> 
#include <unistd.h>  
#include <stdlib.h>  
#include <fcntl.h>   
#include <sys/wait.h>
#include <time.h>

#define MAXSIZE 256

void createMatrix();
void addPid(pid_t pid);
void sigHandle(int sig);


struct results {
    double result1;
    double time1;
    double result2;
    double time2;
    pid_t clientPid;
} inputs;


struct Arguments {
    int n;
    double matrix[MAXSIZE][MAXSIZE]; 
    double detOR;
    pid_t clientPid;
} operands;


pid_t clientPidsArr[MAXSIZE];
int iPidsSize;


char fileServer[MAXSIZE];
char fileClient[MAXSIZE];
char fileShow[MAXSIZE];

char mainFifoName[MAXSIZE];
FILE *fPtrLog;
int fd;	
int fdShow;		
int i, j; //dongu icin kullanilan degiskenler

pid_t showPid;
pid_t serverPid;

int main(int argc, char const *argv[]){

	if(argc != 1){
		printf("\n----------------------------------------------------\n");
        printf("USAGE>>>>>>>>\n");
        printf("gcc -c showResult.c\n");
        printf("gcc showResult.o -o showResult\n");
        printf("./showResult\n");
        printf("----------------------------------------------------\n\n");
        return 0;	
	}
	
	//FILE *fPtrLog = fopen("showResult.log", "a");
	
	unlink("seeWhat_showResult");
    fdShow = mkfifo("seeWhat_showResult", 0666);
	if(fdShow == -1){
	    perror("Didnt create seeWhat_showResult fifo\n");
	    exit(1);
	}
	
    
	printf("pid\t result1\t result2\n");
	iPidsSize = 0;
	while(1){
	    signal( SIGINT, sigHandle);
	    
	    fdShow = open("seeWhat_showResult", O_RDWR);
        if(fdShow == -1){
            perror("Didnt open seeWhat_showResult fifo\n");
            exit(1);
        }
        sleep(1);
        read(fdShow, &operands, sizeof(struct Arguments));
        //printf("fdShowdan operands okundu\n");
        fPtrLog = fopen("showResult.log", "a");
        sprintf(fileShow, "showResult.log");
        
        write(fdShow, &fileShow, sizeof(fileShow));
        //printf("fdShowa fileShow yazildi\n");
        write(fdShow, &operands, sizeof(struct Arguments));        
        //printf("fdShowa operands yazildi\n");
        sleep(1);
        read(fdShow, &inputs.clientPid, sizeof(inputs.clientPid));
        //printf("fdShowdan pid okundu\n"); 
	   
	    //sleep(1);
	    read(fdShow, &serverPid, sizeof(serverPid));
	    //printf("fdShowa clientPids yazildi\n"); 
	    
	    showPid = getpid();
	    
	    addPid(showPid);
	    addPid(inputs.clientPid);
        addPid(serverPid);
	    
	    write(fdShow, &showPid, sizeof(showPid));
        //printf("fdShowa showPid yazildi\n"); 
        
        sleep(1);
        read(fdShow, &fileServer, sizeof(fileServer));
        //printf("fdShowdan fileServer okundu\n");
        //sleep(1);
        read(fdShow, &fileClient, sizeof(fileClient));
        //printf("fdShowdan fileclient okundu\n");
        //sleep(1);
	    read(fdShow, &inputs.result1, sizeof(inputs.result1));
	    //printf("fdShowdan result1 okundu\n");
	    //sleep(1);
	    read(fdShow, &inputs.time1, sizeof(inputs.time1));
	    //printf("fdShowdan time1 okundu\n");
	    //sleep(1);
	    read(fdShow, &inputs.result2, sizeof(inputs.result2));
	    //printf("fdShowdan result2 okundu\n");
	    //sleep(1);
	    read(fdShow, &inputs.time2, sizeof(inputs.time2));
	    //printf("fdShowdan time2 okundu\n");
	    
	    int a  = fprintf(fPtrLog, "pid:%d\nresult1:%.4f\ttime1:%.4f\nresult2:%.4f\ttime2:%.4f\n", inputs.clientPid, inputs.result1, inputs.time1, inputs.result2, inputs.time2);
	    
	    printf("%d\t %.4f\t %.4f\n", inputs.clientPid, inputs.result1, inputs.result2);
	
	    
	    signal( SIGINT, sigHandle);
	    
	}
	
	fclose(fPtrLog);
	close(fdShow);
	unlink("seeWhat_showResult");
	return 0;
}

/**
 * parametre olarak gelen pidyi pid arrayine ekler
 */
void addPid(pid_t pid){
    int res = 0;
    for(i=0; i<=iPidsSize; i++){
        if(clientPidsArr[i] == pid)
            res = 1;
    }
    if(res != 1)
        clientPidsArr[iPidsSize++] = pid;
}


/**
 * CTRL-C sinyali yakalandiginda gereken islemleri yapar
 */
void sigHandle(int sig)
{ 
	printf("\n*** showResult'a CTRL-C sinyali geldi ***\n");
	fprintf(fPtrLog,"*** showResult'a CTRL-C sinyali geldi *** PID:[%ld]\n", (long)getpid());
	
	//printf("fileServer:%s\n", fileServer);
	FILE *fPtrServerLog = fopen(fileServer, "a");
	fprintf(fPtrServerLog, "*** timeServer killed by seeWhat ***\n");
	
	//printf("fileClient:%s\n", fileClient);
	FILE *fPtrClientLog = fopen(fileClient, "a");
	fprintf(fPtrClientLog, "*** seeWhat killed by seeWhat ***\n");
	
	/*printf("iPidsSize:%d\n", iPidsSize);
	for(i=0; i<=iPidsSize; i++)
	    printf("pid:%d\n", clientPidsArr[i]);*/
	
	//printf("kill clients\n");
	for(i=0; i<iPidsSize; i++)
	    kill(clientPidsArr[i],SIGINT);
	
	fclose(fPtrServerLog);
	fclose(fPtrClientLog);
	fclose(fPtrLog);
	
	printf("CTRL-C sinyali geldi\n2 saniye bekleyip kapanacak!\n");
	sleep(2);
	exit(sig);
}
