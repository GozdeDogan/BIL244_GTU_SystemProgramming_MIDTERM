/**
 * timeServer.c -> SERVER
 * 
 * Gozde Dogan 131044019
 * System programming MIDTERM
 * 17 April, 2017
 *
 * References:
 *      1. random fonksiyonuna bakmak icin;
 *      https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
 *          
 *      2. 
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


struct Arguments {
    int n;
    double matrix[MAXSIZE][MAXSIZE]; 
    double detOR;
    pid_t clientPid;
} operands;


pid_t clientPidsArr[MAXSIZE];
int iPidsSize;


struct Okey {
    int OK;
    char backFifo[MAXSIZE]; //client'in mesaj yollayacagi fifo
    pid_t myPid;
} okey;


char fileServer[MAXSIZE];
char fileClient[MAXSIZE];
char fileShow[MAXSIZE];


char mainFifoName[MAXSIZE];
FILE *fPtrLog;
char fileName[MAXSIZE];
int ticksINms;
double creationTime;
struct timespec n_start, n_stop;
int fd;
int fdBack;			
int i, j, status=0; //dongu icin kullanilan degiskenler

pid_t showPid;

int main(int argc, char const *argv[]){

	if(argc != 4){
		printf("\n----------------------------------------------------\n");
        printf("USAGE>>>>>>>>\n");
        printf("gcc -c timeServer.c\n");
        printf("gcc timeServer.o -o timeServer\n");
        printf("./timeServer <ticks in miliseconds> <n> <mainpipefifo>\n");
        printf("----------------------------------------------------\n\n");
        return 0;	
	}
			
	strcpy(fileServer,"timeServer.log");
    fPtrLog = fopen(fileServer,"a");
    pid_t pid = getpid();

    iPidsSize = 0;   

	//argumanlarin kopyalanmasi
	ticksINms = atoi(argv[1]);
	operands.n = atoi(argv[2]);
	strcpy(mainFifoName, argv[3]);
    
    
	//MAINFIFO'yu olusturdum
	unlink(mainFifoName);
	fd = mkfifo(mainFifoName,  0666);
	if(fd==-1){
	    perror("Didnt create main fifo\n");
	    exit(1);
	}
 
           
    addPid(pid);
    
	while(1){
		signal( SIGINT, sigHandle);
		
	    //MAINFIFO'yu actim
		fd = open(mainFifoName, O_RDWR);
		if(fd==-1){
		    perror("Didnt open main fifo\n");
		    exit(1);
		}
		
		//client dan matrisin gonderilecegi fifo adi aliniyor
		sleep(1);
        read(fd, &okey, sizeof(struct Okey)); 
        printf("\nokey.OK:%d\nokey.backFifo:%s\nokey.myPid:%d\n", okey.OK, okey.backFifo, (int)okey.myPid);
        
        operands.clientPid = okey.myPid;
        
        fdBack = open(okey.backFifo, O_RDWR);
        if(fdBack==-1){
            perror("Didnt open client fifo\n");
            exit(1);
        }
        
        addPid(okey.myPid);
        
        write(fdBack, &pid, sizeof(pid));
        
	    sleep(1);
	    read(fdBack, &showPid, sizeof(showPid));
	   
	    addPid(showPid);
	   
        if(okey.OK != 0 && okey.backFifo != NULL){
            int fpid = fork();
            if(fpid == 0){
                //Baslangic zamani
	            if( clock_gettime( CLOCK_REALTIME, &n_start) == -1 ) {
                  perror( "clock gettime" );
                  return EXIT_FAILURE;
                }	            
                
                createMatrix();
                printf("\noriginal matrix:\n");
                for(i=0; i<2*operands.n; i++){
                    for(j=0; j<2*operands.n; j++){
                        printf("%.4f", operands.matrix[i][j]);
                        if(j < 2*operands.n-1)
                            printf(", ");
                    }
                    printf("\n");
                }
                printf("\n");
                // Bitis zamani
                if( clock_gettime( CLOCK_REALTIME, &n_stop) == -1 ) {
                  perror( "clock gettime" );
                  return EXIT_FAILURE;
                }
                // matrisin olusturulma sureci
                creationTime = ((n_stop.tv_sec - n_start.tv_sec) *1000 / CLOCKS_PER_SEC) + ( n_stop.tv_nsec - n_start.tv_nsec );
                
                write(fdBack, &operands, sizeof(struct Arguments)); //matrix yollandi
                //printf("fdBack'e operands yazildi\n");
                sleep(1);
                read(fdBack, &fileShow, sizeof(fileShow));
                //printf("fileShow: %s\n", fileShow);
                //sleep(1);
	            read(fdBack, &fileClient, sizeof(fileClient)); 
                //printf("fileClient: %s\n", fileClient);
                write(fdBack, &fileServer, sizeof(fileServer));
                //printf("fdBack'e fileServer yazildi\n");
                sleep(1);
                read(fdBack, &operands, sizeof(struct Arguments)); //backFifo dan tekrar operands okundu, pid ve det icin
                //printf("fdBack'ten operands okundu\n");
                
                
                
                fprintf(fPtrLog, "miliseconds:%.4f\nclientPid:%d\ndet(matrix):%.4f\n\n", creationTime, operands.clientPid, operands.detOR);
                
                printf("creation time:%.4f\nclientPid:%d\ndet(matrix):%.4f\n\n", creationTime, operands.clientPid, operands.detOR);
                
                fclose(fPtrLog);
                close(fd);
                close(fdBack);
                exit(status);
	        }
	        else{
	            sprintf(fileShow, "showResult.log");
	            sprintf(fileClient, "client%d_%d.log", okey.myPid, okey.OK);
	            wait(NULL);
	        }
	    }  
        signal( SIGINT, sigHandle);       
        
        // girilen milisaniye saniyeye cevrildi ve diger connect icin bu kadar beklenildi!   
        sleep(ticksINms/1000);
	}

    fclose(fPtrLog);
    close(fd);
    close(fdBack);
	unlink(mainFifoName);

	return 0;
}

/**
 * random olarak matrix olusturur.
 * client'a yollanacak olan matrix
 */
void createMatrix(){
    int i=0;
    
    srand(time(NULL));
    
    for(i=0; i<2*operands.n; i++){
        for(j=0; j<2*operands.n; j++)
            operands.matrix[i][j] = rand()%10;
    }
}

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
 * CTRL-C sinyali gelince degerlerimi log dosyasina yaziyor 
 * acik butun clientlari kill yapip cikar  
 */
void sigHandle(int sig)
{
    unlink(okey.backFifo);
	unlink(mainFifoName);
    int i=0;
	printf("\n*** timeServer'a CTRL-C sinyali geldi  ***\n");
	fprintf(fPtrLog,"*** timeServer'a CTRL-C sinyali geldi *** PID:[%ld]\n", (long)getpid());
	fclose(fPtrLog);
	
	//printf("fileShow: %s\n", fileShow);
	FILE *fPtrShowLog = fopen(fileShow, "a");
	fprintf(fPtrShowLog, "*** showResult killed by timeServer ***\n");
	fclose(fPtrShowLog);	

	//printf("fileClient: %s\n", fileClient);
	FILE *fPtrServerLog = fopen(fileClient, "a");
	fprintf(fPtrServerLog, "*** seeWhat killed by timeServer ***\n");
	fclose(fPtrServerLog);
	
	/*printf("iPidsSize:%d\n", iPidsSize);
	for(i=0; i<iPidsSize; i++)
	    printf("pid:%d\n", clientPidsArr[i]);*/
	
    //printf("killss\n");
	for(i=0; i<=iPidsSize; i++)
	    kill(clientPidsArr[i],SIGINT);	    
	
	printf("CTRL-C sinyali geldi\n2 saniye bekleyip kapanacak!\n");
	sleep(2);
	exit(sig);
}
