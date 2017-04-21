/**
 * seeWhat.c -> CLIENT
 * 
 * Gozde Dogan 131044019
 * System programming MIDTERM
 * 17 April, 2017
 *
 * References:
 *      1. determinant alma
 *      http://stackoverflow.com/questions/41384020/c-program-to-calculate-the-determinant-of-a-nxn-matrix
 *      2. inverse
 *      http://www.geeksforgeeks.org/adjoint-inverse-matrix/
 *      3. convolution
 *      http://groups.inf.ed.ac.uk/vision/GRASSIN/SkinSpotTool/skinSpotTool/Convolution.java      
 */
 
 
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define MAXSIZE 256

//// function prototypes
void convolutionMatrix();
void inverseMatrix();
void inverse(double matrix[MAXSIZE][MAXSIZE], int n, double inversing[MAXSIZE][MAXSIZE]);
void getCofactor(double A[MAXSIZE][MAXSIZE], double temp[MAXSIZE][MAXSIZE], int p, int q, int n);
void adjoint(double A[MAXSIZE][MAXSIZE],double adj[MAXSIZE][MAXSIZE], int N);
void convolution(double matrix[][MAXSIZE], int n, double convMatrix[MAXSIZE][MAXSIZE]);
double conv(double matrix[][MAXSIZE], int x, int y);
void calculateDeterminant(double matrix[][MAXSIZE], int n, double *result);
double calculateDet(double matrix[][MAXSIZE], int n);
double transpose(double matrix[][MAXSIZE], int n);
void partitionMatrix();
void unificationMatrix(int uMAtrix);
void addPid(pid_t pid);
void sigHandle(int sig);

// variables
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
int flag=0;
FILE *fPtrLog;
char fileName[MAXSIZE];
struct timespec n_start, n_stop;
int fd;
int fdBack;
int fdShow;			
int i, j, status=0; //dongu icin kullanilan degiskenler

pid_t showPid;
pid_t serverPid;

double timeSI, timeConv;
double detSI, detConv;
double result1, result2;

double kernelMatrix[3][3] = {{0, 0, 0}, 
                            {0, 1, 0}, 
                            {0, 0, 0}}; //convolution almak icin kullanilan matrix

double transposeMatrix[MAXSIZE][MAXSIZE]; //tranpose fonksiyonun icinde kullaniliyor

double SIMatrix[MAXSIZE][MAXSIZE]; // shifted inverse matrix
double convMatrix[MAXSIZE][MAXSIZE]; // convolution matrix

// matrix'in 4 parcasi
double firstMatrix[MAXSIZE][MAXSIZE];
double secondMatrix[MAXSIZE][MAXSIZE];
double thirdMatrix[MAXSIZE][MAXSIZE];
double fourthMatrix[MAXSIZE][MAXSIZE];

// 4 parcanin inverse alinmis hali
double inverseFirstMatrix[MAXSIZE][MAXSIZE];
double inverseSecondMatrix[MAXSIZE][MAXSIZE];
double inverseThirdMatrix[MAXSIZE][MAXSIZE];
double inverseFourthMatrix[MAXSIZE][MAXSIZE];

// 4 parcanin convolution yapilmis hali
double convFirstMatrix[MAXSIZE][MAXSIZE];
double convSecondMatrix[MAXSIZE][MAXSIZE];
double convThirdMatrix[MAXSIZE][MAXSIZE];
double convFourthMatrix[MAXSIZE][MAXSIZE];


int main(int argc, char const *argv[]){
    
    if(argc != 3 && argc != 2){
        printf("\n----------------------------------------------------\n");
        printf("USAGE>>>>>>>>\n");
        printf("gcc -c seeWhat.c\n");
        printf("gcc seeWhat.o -o seeWhat\n");
        printf("./seeWhat <mainpipefifo> <convolution according to which matrix>\n");
        printf("----------------------------------------------------\n\n");
        return 0;
    }
    
    strcpy(mainFifoName, argv[1]);
    
    if(argc == 2)
        flag = 0;
    
    if(argc == 3)
        flag = atoi(argv[2]);
    
	okey.myPid = getpid();
    sprintf(okey.backFifo, "fifo%d", okey.myPid);
        
    // server ile matrixin gelecegi fifo olusturuldu
    fdBack = mkfifo(okey.backFifo, 0666);
    if(fdBack == -1){
        perror("Didnt create back fifo\n");
        exit(1);
    }    
    okey.OK=0; 
    iPidsSize = 0;
    while(1){ 
        signal( SIGINT, sigHandle);
         
        //show result fifosu acildi 
        fdShow = open("seeWhat_showResult", O_RDWR);
        if(fdShow == -1){
            perror("Didnt open seeWhat_showResult fifo\n");
            exit(1);
        }

        write(fdShow, &operands, sizeof(struct Arguments));
        sleep(1);
        read(fdShow, &fileShow, sizeof(fileShow));
        //printf("fdShowdan fileShow okundu\n");
        //sleep(1);
        read(fdShow, &operands, sizeof(struct Arguments));

        write(fdShow, &okey.myPid, sizeof(okey.myPid)); // show result'a yollanan pid   
        //printf("fdShowa pid yazildi\n");

        fd = open(mainFifoName, O_RDWR);
        if(fd == -1){
            perror("Didnt open main fifo\n");
            exit(1);
        }

        okey.OK++;

        write(fd, &okey, sizeof(struct Okey)); //sinyal ve client-server icin ozel fifo adi
        //printf("fdye okey yazildi\n");	    
        operands.clientPid = okey.myPid;


        sprintf(fileName, "client%d_%d.log", operands.clientPid, okey.OK);
        strcpy(fileClient, fileName);	
        printf("\npid:%d\tfile:%s\n", operands.clientPid, fileClient);
        
        fdBack = open(okey.backFifo, O_RDWR);
        if(fdBack == -1){
            perror("Didnt open back fifo\n");
            exit(1);
        }

        addPid(okey.myPid); 

        sleep(1);   
        read(fdBack, &serverPid, sizeof(serverPid));    
        addPid(serverPid);


        write(fdShow, &serverPid, sizeof(serverPid));
        //printf("fdShowa serverPid yazildi\n");
        
        sleep(1);
        read(fdShow, &showPid, sizeof(showPid));
        //printf("fdShowdan showPid okundu\n");

        addPid(showPid);

        write(fdBack, &showPid, sizeof(showPid));
        //printf("fdBacke showPid yazildi\n");

        sleep(1);
        read(fdBack, &operands, sizeof(struct Arguments));
        //printf("fdBackten operands okundu\n");

        write(fdBack, &fileShow, sizeof(fileShow));
        //printf("fdBacke fileShow yazildi\n");

        write(fdBack, &fileClient, sizeof(fileClient)); 
        //printf("fdBacke fileClient yazildi\n");

        sleep(1); 
        read(fdBack, &fileServer, sizeof(fileServer)); 
        //printf("fdBackten fileServer okundu\n"); 

        write(fdShow, &fileServer, sizeof(fileServer));
        //printf("fdShowa fileServer yazildi\n"); 

        write(fdShow, &fileClient, sizeof(fileClient));
        //printf("fdShowa fileClient yazildi\n");   
        fPtrLog = fopen(fileName, "a");
        
        // original matrix dosyayalog dosyasina yazdirildi.
        fprintf(fPtrLog, "\nOriginal Matrix:\n");
        printf("\nOriginal Matrix:\n");
        for(i=0; i<2*operands.n; i++){
            for(j=0; j<2*operands.n; j++){
                fprintf(fPtrLog, "%.4f", operands.matrix[i][j]);
                printf("%.4f", operands.matrix[i][j]);
                if(j < 2*operands.n-1){
                    fprintf(fPtrLog, ", ");  
                    printf(", ");
                }  
            }
            fprintf(fPtrLog, "\n");
            printf("\n"); 
        }
        fprintf(fPtrLog, "\n");
        printf("\n");

        // orjinal matrisin determinanti hesaplandi
        double detOR = 0;
        calculateDeterminant(operands.matrix, 2*operands.n, &detOR);
        operands.detOR = detOR;
        printf("\ndetOR: %.4f\n", operands.detOR);

        pid_t pid;

        pid=fork();   //shifted matrix'i bulmak icin ve result1'i hesaplamak icin		
        if(pid==0){
            if( clock_gettime( CLOCK_REALTIME, &n_start) == -1 ) {
                perror( "clock gettime" );
                return EXIT_FAILURE;
            }	            

            inverseMatrix();
            calculateDeterminant(SIMatrix, 2*operands.n, &detSI);
            printf("detSI:%.4f\n", detSI);
            // Bitis zamani
            if( clock_gettime( CLOCK_REALTIME, &n_stop) == -1 ) {
                perror( "clock gettime" );
                return EXIT_FAILURE;
            }

            result1 = operands.detOR - detSI;

            // matrisin olusturulma sureci
            timeSI = ((n_stop.tv_sec - n_start.tv_sec) *1000 / CLOCKS_PER_SEC) + ( n_stop.tv_nsec - n_start.tv_nsec );
            signal( SIGINT, sigHandle);

            write(fdShow, &result1, sizeof(result1));
            write(fdShow, &timeSI, sizeof(timeSI)); 

            printf("result1: %.4f \t time1: %.4f\n", result1, timeSI);
            fprintf(fPtrLog, "Shifted Inverse Matrix:\n");
            for(i=0; i<2*operands.n; i++){
                for(j=0; j<2*operands.n; j++){
                    fprintf(fPtrLog, "%.4f", SIMatrix[i][j]);
                    if(j < 2*operands.n-1)
                        fprintf(fPtrLog, ", ");    
                }
                fprintf(fPtrLog, "\n"); 
            }  
            fprintf(fPtrLog, "\n"); 

            fclose(fPtrLog); //isi biten processte kopyasi olusan bu dosya kapatilmak zorunda
            close(fd);
            close(fdBack);
            close(fdShow);
            exit(status);					
        }
        else {
            wait(NULL);
        }


        pid=fork();   //convolution matrix'i bulmak icin ve result2'yi hesaplamak icin		
        if(pid==0){
            if( clock_gettime( CLOCK_REALTIME, &n_start) == -1 ) {
                perror( "clock gettime" );
                return EXIT_FAILURE;
            }	            

            convolutionMatrix();
            calculateDeterminant(convMatrix, 2*operands.n, &detConv);
            printf("detConv:%.4f\n", detConv);
            
            // Bitis zamani
            if( clock_gettime( CLOCK_REALTIME, &n_stop) == -1 ) {
                perror( "clock gettime" );
                return EXIT_FAILURE;
            }

            result2 = operands.detOR - detConv;
            // matrisin olusturulma sureci
            timeConv = ((n_stop.tv_sec - n_start.tv_sec) *1000 / CLOCKS_PER_SEC) + ( n_stop.tv_nsec - n_start.tv_nsec );

            printf("result2: %.4f \t time2: %.4f\n\n", result2, timeConv);

            write(fdShow, &result2, sizeof(result2));
            write(fdShow, &timeConv, sizeof(timeConv));

            fprintf(fPtrLog, "2D Convolution Matrix:\n");
            for(i=0; i<2*operands.n; i++){
                for(j=0; j<2*operands.n; j++){
                    fprintf(fPtrLog, "%.4f", convMatrix[i][j]);
                    if(j < 2*operands.n-1)
                        fprintf(fPtrLog, ", ");    
                }
                fprintf(fPtrLog, "\n"); 
            }  
            fprintf(fPtrLog, "\n");       

            fclose(fPtrLog); //isi biten processte kopyasi olusan bu dosya kapatilmak zorunda
            close(fd);
            close(fdBack);
            close(fdShow);
            exit(status);					
        }
        else {
            wait(NULL);
        }

        write(fdBack, &operands, sizeof(struct Arguments));
        
        signal( SIGINT, sigHandle);
    }
    fclose(fPtrLog);
    close(fd);
    close(fdBack);
    close(fdShow);
    unlink("seeWhat_showResults");
    unlink(okey.backFifo);
    unlink(mainFifoName);

    return 0;
}


/**
 * operands.matrix'i parcalara ayrilir.
 * 4 parca da convolution edilir.
 * convolution edilen 4 parca birlestirilir.
 */
void convolutionMatrix(){
    partitionMatrix();
    
    convolution(firstMatrix, operands.n, inverseFirstMatrix);
    convolution(secondMatrix, operands.n, inverseSecondMatrix);
    convolution(thirdMatrix, operands.n, inverseThirdMatrix);
    convolution(fourthMatrix, operands.n, inverseFourthMatrix);
    
    /*printf("firstMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", firstMatrix[i][j]);
        }
        printf("\n");
    }
    
    printf("\nsecondMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", secondMatrix[i][j]);
        }
        printf("\n");
    }
    
    printf("\nthirdMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", thirdMatrix[i][j]);
        }
        printf("\n");
    }
    printf("\nfourthMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", fourthMatrix[i][j]);
        }
        printf("\n");
    }*/
    
    unificationMatrix(2);
}

/**
 * operands.matrix'i parcalara ayrilir.
 * 4 parca da inverse edilir.
 * inverse edilen 4 parca birlestirilir.
 */
void inverseMatrix(){
    partitionMatrix();
    
   /* printf("firstMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", firstMatrix[i][j]);
        }
        printf("\n");
    }
    
    printf("\nsecondMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", secondMatrix[i][j]);
        }
        printf("\n");
    }
    
    printf("\nthirdMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", thirdMatrix[i][j]);
        }
        printf("\n");
    }
    printf("\nfourthMatrix:\n");
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            printf("%.2f ", fourthMatrix[i][j]);
        }
        printf("\n");
    }*/
    
    
    inverse(firstMatrix, operands.n, inverseFirstMatrix);
    inverse(secondMatrix, operands.n, inverseSecondMatrix);
    inverse(thirdMatrix, operands.n, inverseThirdMatrix);
    inverse(fourthMatrix, operands.n, inverseFourthMatrix);
    
    unificationMatrix(1);
}

/**
 * parametre olarak gelen matrix'i inverse edip inversing'e yazar!
 */
void inverse(double matrix[MAXSIZE][MAXSIZE], const int n, double inversing[MAXSIZE][MAXSIZE]){
    double det = 0;
    calculateDeterminant(matrix, n, &det);
    //printf("det:%.4f\n", det);
    double ratio, a; 
    int i, j, k;
    if(det != 0){ 
        double adj[MAXSIZE][MAXSIZE];
        adjoint(matrix, adj, n);
     
        // Find Inverse using formula "inverse(A) = adj(A)/det(A)"
        for (i=0; i<n; i++)
            for (j=0; j<n; j++)
                inversing[i][j] = adj[i][j]/(double)det;
    }
    else{
        for (i=0; i<n; i++)
            for (j=0; j<n; j++)
                inversing[i][j] = 0.0;
    }
}


void adjoint(double A[MAXSIZE][MAXSIZE], double adj[MAXSIZE][MAXSIZE], int N){
    if (N == 1){
        adj[0][0] = 1;
        return;
    }
    
    int sign = 1;
    double temp[MAXSIZE][MAXSIZE];
 
    for (i=0; i<N; i++){
        for (j=0; j<N; j++){
            getCofactor(A, temp, i, j, N);
            sign = ((i+j)%2==0)? 1: -1;
            double det = calculateDet(temp, N-1);
            adj[j][i] = (sign)*(det);
        }
    }
}

void getCofactor(double A[MAXSIZE][MAXSIZE], double temp[MAXSIZE][MAXSIZE], int p, int q, int n){
    int i = 0, j = 0;
    int row, col;
    
    for (row = 0; row < n; row++) {
        for (col = 0; col < n; col++){
            if (row != p && col != q){
                temp[i][j++] = A[row][col];
                if (j == n - 1){
                    j = 0;
                    i++;
                }
            }
        }
    }
}

/**
 * calculate Transpose
 */
double transpose(double matrix[MAXSIZE][MAXSIZE], int n){
    int i, j;
    for(i=1;i<=n;i++)
        for(j=1;j<=n;j++)
            transposeMatrix[i][j]=matrix[j][i];
    return transposeMatrix[n][n];
}

/**
 * gelen matrisin determinantini bulur pointer'a atar.
 */
void calculateDeterminant(double matrix[MAXSIZE][MAXSIZE], int n, double *result){
    *result = calculateDet(matrix, n);
}

/**
 * gelen matrix'in determinantini bulur ve return eder.
 */
double calculateDet(double matrix[MAXSIZE][MAXSIZE], int n){
    double Minor[MAXSIZE][MAXSIZE];
    int i,j,k,c1,c2;
    double determinant;
    double c[MAXSIZE];
    int O=1;
    if(n==1)
        return matrix[0][0];
    else if(n == 2)
        return matrix[0][0]*matrix[1][1]-matrix[0][1]*matrix[1][0];
    else{
        for(i = 0 ; i < n ; i++){
            c1 = 0, c2 = 0;
            for(j = 0 ; j < n ; j++){
                for(k = 0 ; k < n ; k++){
                    if(j != 0 && k != i){
                        Minor[c1][c2] = matrix[j][k];
                        c2++;
                        if(c2>n-2){
                            c1++;
                            c2=0;
                        }
                    }
                }
            }
            determinant = determinant + O*(matrix[0][i]*calculateDet(Minor,n-1));
            O=-1*O;
        }
        return determinant;
    }
}

/**
 * gelen matrix'in convolution'unu alir ve convMatrix'e yazar
 * kernel matrix'in her cell de dolasmasina gore matrisi bulur.
 */
void convolution(double matrix[][MAXSIZE], int n, double convMatrix[MAXSIZE][MAXSIZE]){
    int smallWidth = n - 3 + 1;
    int smallHeight = n - 3 + 1;
    
    for(i=0; i<smallWidth; ++i)
        for(j=0; j<smallHeight; ++j)
            convMatrix[i][j]=0;
            
    for(i=0; i<smallWidth; ++i)
        for(j=0; j<smallHeight; ++j)
            convMatrix[i][j] = conv(matrix, i, j);    
}

/**
 * Gelen matrix'in yukarida tanimlanan kernel matrixe gore convolutionunu alir
 * bi cell icin yapar!
 */
double conv(double matrix[][MAXSIZE], int x, int y){
    double output = 0;
    int k, m;
    
    for(k=0; k<3; ++k)
        for(m=0; m<3; ++m)
            output = output + (matrix[x+k][y+m] * kernelMatrix[k][m]);
        
    return output;
}

/**
 * serverden gelen matrix'i 4 parcaya boler.
 */
void partitionMatrix(){
    int row=0, col=0;
    for(i=0; i<operands.n; i++){
        for(j=0; j<operands.n; j++){
            firstMatrix[row][col] = operands.matrix[i][j];
            col++;
        }
        col = 0;
        row++;
    }
    
    
    row = 0;
    col = 0;
    
    for(i=0; i<operands.n; i++){
        for(j=operands.n; j<2*operands.n; j++){
            secondMatrix[row][col] = operands.matrix[i][j];
            col++;
        }
        col = operands.n;
        row++;
    }
    
    row = 0;
    col = 0;
    
    for(i=operands.n; i<2*operands.n; i++){
        for(j=0; j<operands.n; j++){
            thirdMatrix[row][col] = operands.matrix[i][j];
            col++;
        }
        col = 0;
        row++;
    }
    
    row = 0;
    col = 0;
    
    for(i=operands.n; i<2*operands.n; i++){
        for(j=operands.n; j<2*operands.n; j++){
            fourthMatrix[row][col] = operands.matrix[i][j];
            col++;
        }
        col = operands.n;
        row++;
    }
}

/**
 * uzerinde gerekli islem yapilmis olan 
 * firstMatrix, secondMatrix, thirdMatrix, fourthMatrix
 * matrislerini birlestirir!
 */
void unificationMatrix(int uMatrix){
    if(uMatrix == 1){
        int row=0, col=0;
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                SIMatrix[row][col] = inverseFirstMatrix[i][j];
                col++;
            }
            col = 0;
            row++;
        }
        
        
        row = 0;
        col = operands.n;
        
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                SIMatrix[row][col] = inverseSecondMatrix[i][j];
                col++;
            }
            col = operands.n;
            row++;
        }
        
        row = operands.n;
        col = 0;
        
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                SIMatrix[row][col] = inverseThirdMatrix[i][j];
                col++;
            }
            col = 0;
            row++;
        }
        
        row = operands.n;
        col = operands.n;
        
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                SIMatrix[row][col] = inverseFourthMatrix[i][j];
                col++;
            }
            col = operands.n;
            row++;
        }
    }else{
        int row=0, col=0;
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                convMatrix[row][col] = inverseFirstMatrix[i][j];
                col++;
            }
            col = 0;
            row++;
        }
        
        
        row = 0;
        col = operands.n;
        
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                convMatrix[row][col] = inverseSecondMatrix[i][j];
                col++;
            }
            col = operands.n;
            row++;
        }
        
        row = operands.n;
        col = 0;
        
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                convMatrix[row][col] = inverseThirdMatrix[i][j];
                col++;
            }
            col = 0;
            row++;
        }
        
        row = operands.n;
        col = operands.n;
        
        for(i=0; i<operands.n; i++){
            for(j=0; j<operands.n; j++){
                convMatrix[row][col] = inverseFourthMatrix[i][j];
                col++;
            }
            col = operands.n;
            row++;
        }
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
 * CTRL-C sinyali yakalandiginda gereken islemleri yapar
 */
void sigHandle(int sig)
{     
    unlink(okey.backFifo);
	unlink(mainFifoName);
	printf("\n*** seeWhat'a CTRL-C sinyali geldi ***\n");
	fprintf(fPtrLog,"*** seeWhat'a CTRL-C sinyali geldi *** PID:[%ld]\n", (long)getpid());
	
	//printf("fileServer:%s\n", fileServer);
	FILE *fPtrServerLog = fopen(fileServer, "a");
	fprintf(fPtrServerLog, "*** timeServer killed by seeWhat ***\n");
	
	//printf("fileshow:%s\n", fileShow);
	FILE *fPtrShowResLog = fopen(fileShow, "a");
	fprintf(fPtrShowResLog, "*** showResult killed by seeWhat ***\n");
	
	/*printf("iPidsSize:%d\n", iPidsSize);
	for(i=0; i<iPidsSize; i++)
	    printf("pid:%d\n", clientPidsArr[i]);*/
	
	//printf("kill clients\n");
	for(i=0; i<iPidsSize; i++)
	    kill(clientPidsArr[i], SIGINT);
	    
	
	fclose(fPtrServerLog);
	fclose(fPtrLog);
	fclose(fPtrShowResLog);
	
	printf("CTRL-C sinyali geldi\n2 saniye bekleyip kapanacak!\n");
	sleep(2);
	exit(sig);
}
