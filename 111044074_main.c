/**
 * Multi processor 1D Discrete Fourier Transform (DFT) calculator
 *
 * ./multiprocess_DFT -N 5 -X file.dat -M 100
 * CSE344 System Programming HomeWork 2
 * Sinan Elveren - Gebze Technical University - Computer Engineering
 */

#define _POSIX_SOURCE
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>   //random number
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


#define NDEBUG                      //Debug mode

#define PI 3.14159265
#define MAX_RAND_NUM 100            //generate random number up to '100'
//using for info of line number. I wrote total read/write byte of last sequence but didint use it
#define INFO_FILE_NAME "info.txt"
#define SEQUENCE_FILE_PATH "calcualteSequence.txt"
#define PROCESS_A_LOG_PATH "processA.log"
#define PROCESS_B_LOG_PATH "processB.log"
#define CREAT_FILE_FLAGS O_WRONLY | O_CREAT | O_TRUNC
#define OPEN_FILE_FLAGS_A O_WRONLY | O_NONBLOCK
#define OPEN_FILE_FLAGS_B O_RDWR | O_NONBLOCK
#define OPEN_FILE_FLAGS_C O_WRONLY | O_APPEND | O_NONBLOCK
#define OPEN_FILE_FLAGS_COMMON O_RDWR | O_NONBLOCK



//global variable
int     fd_action_b;
int     fd_action_a;
int     fd_info;
int     fd;
pid_t   parentPID = 0;      //for check child pid



int getNewSequence(int sequence[], int N);
int strToNumber(char str[]);
int calcDFT(int sequence[], int N);





pid_t myWait(int *status) {
    pid_t rtrn;

    while (((rtrn = wait(status)) == -1) && (errno == EINTR));

    return rtrn;
}



void myAtexit(void){
    //childreen is coming
    if (getpid() != parentPID) {
        fflush(stdout);
        fprintf(stdout, "\n    > [%ld] Child came here ", getpid());

        close(fd);
        close(fd_info);
        close(fd_action_b);
        fflush(stdout);
        fprintf(stdout,"\n    Process[%ld] has been exit succesfuly\n", getpid());
        //exit
        return;
    }
    while (myWait(NULL ) > 0);
    fflush(stdout);
    fprintf(stdout,"\n    > [%d] Parent came here %ld", getpid());


    close(fd);
    close(fd_info);
    close(fd_action_a);

    fprintf(stdout,"\n\n    >All file descriptors are closed\n");

    fflush(stdout);
    fprintf(stdout,"\n    Process[%ld] has been exit succesfuly\n", getpid());
    //exxit
}



// or exit funcs
void finish(int exitNum) {
  //  myAtexit();         //for wait children
    exit(exitNum);
}



void signalCatcher(int signum) {
    switch (signum) {
        case SIGUSR1: puts("\ncaught SIGUSR1");
            break;
        case SIGUSR2: puts("\ncaught SIGUSR2");
            break;
        case SIGINT:
            fprintf(stderr,"\n\nSIGINT:Ctrl+C signal detected, exit\n",stderr);
             //atexit();
            finish(1);
        default:
            fprintf(stderr,"catcher caught unexpected signal %d\n", signum);
            finish(1);
    }
}


int main(int argc, char *argv[]) {

    parentPID = getpid();
    if(atexit(myAtexit) !=0 ) {
        perror("atexit");
        return 1;
    }
    /* * * * * * ** * * * * * * * * * * * * * * * CHECK_USAGE * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    /* There are wrong argument count */
    if (argc != 7) {
        printf("\n'Too many/less paramters'\nUsage Error  : ./multiprocess_DFT -N 5 -X file.dat -M 100\n");
        return 1;
    }

    /* check parameter -N*/
    if (strcmp(argv[1], "-N")) {
        printf("\nmissing/wrong parameter '-N':%s\nUsage Error  : ./multiprocess_DFT -N 5 -X file.dat -M 100\n", argv[1]);
        return 1;
    }

    /* check value of N*/
    if (atoi(argv[2]) < 1 ) {
        printf("\nmissing/wrong value '-N value':%s\nUsage Error  : ./multiprocess_DFT -N 5 -X file.dat -M 100\n",argv[2]);
        return 1;
    }

    /* check parameter -X*/
    if (strcmp(argv[3], "-X")) {
        printf("\nmissing/wrong parameter '-X':%s\nUsage Error  : ./multiprocess_DFT -N 5 -X file.dat -M 100\n", argv[3]);
        return 1;
    }

    /* check parameter -M*/
    if (strcmp(argv[5], "-M")) {
        printf("\nmissing/wrong parameter '-M':%s\nUsage Error  : ./multiprocess_DFT -N 5 -X file.dat -M 100\n", argv[5]);
        return 1;
    }

    /* check value of M*/
    if (atoi(argv[6]) < 0 ) {
        printf("\nmissing/wrong value '-M value':%s\nUsage Error  : ./multiprocess_DFT -N 5 -X file.dat -M 100\n",argv[6]);
        return 1;
    }

    /* * * * * * ** * * * * * * * * * * * * * * * DECLARATION * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    srand(time(NULL));
    mode_t  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH; //permisions
    char    str[sizeof(int) + 1];       //size of int + ';' char
    char    info[sizeof(int)];          //size of int
    pid_t   PIDprocessB;
    int     errNo = 0;                  // if it s zero, no error
    //char    infoFile[8];
    int     firstNumBytes = 0;          //for info file to debug mode - mean first index(begining of last line)
    int     lastNumBytes = 0;           //for info file to debuge mode - mean end of last line
    int     m = 0;                      //line number for info file
    struct  flock lock;                 //will lock to file

    int     N = 0;                      //parameter N value
    int     M = 0;                      //parameter M value

    N = atoi(argv[2]);
    M = atoi(argv[6]);
    int     sequence[N];                //keep generated sequence
    int     DFTofSequence[N];           //keep calculated sequence
    char    strLog[100];                //keep info of executed proc
    sigset_t sigset, oldMask, newMask;
    struct  sigaction sact;

    strLog[0] = '\0';


    /*create file for sequence*/
    fd = open(argv[4], CREAT_FILE_FLAGS, mode);
    /*create file for info*/
    fd_info = open(INFO_FILE_NAME, CREAT_FILE_FLAGS, mode);
    /*create file for actionf of processes*/
    fd_action_a = open(PROCESS_A_LOG_PATH, CREAT_FILE_FLAGS, mode);
    fd_action_b = open(PROCESS_B_LOG_PATH, CREAT_FILE_FLAGS, mode);

    if (fd == -1 || fd_info == -1 || fd_action_a == -1 || fd_action_b == -1) {
        /* An error occured while create the file*/
        perror("Error : create file");
        return 1;
    }

    //written byte info - first
    sprintf(str, "%09d;", firstNumBytes);
    write(fd_info, str, strlen(str));
    //written byte info - last byte
    sprintf(str, "%09d;", lastNumBytes);
    write(fd_info, str, strlen(str));
    //written byte info - M count
    sprintf(str, "%09d;", m);
    write(fd_info, str, strlen(str));

#ifndef NDEBUG
    fprintf(stdout, " \nfirst :%d last :%d m :%d \n", firstNumBytes, lastNumBytes, m);
#endif

    while (close(fd) == -1 || close(fd_info) == -1 || close(fd_action_a) == -1 || close(fd_action_b) == -1) {
        perror("close problem - close the file again");
    }

/*

    */
    /**Signal**/

    sigemptyset(&oldMask);
    sigemptyset(&newMask);

    struct sigaction newact;
    newact.sa_handler = signalCatcher;
    /* set the new handler */

    newact.sa_flags = 0;

    /* no special options */
    if ((sigemptyset(&newact.sa_mask) == -1) || (sigaction(SIGINT, &newact, NULL) == -1))
        perror("Failed to install SIGINT signal handler");



    //create a fork - main process named process A will generate sequence
    // The fork named process B will calculate DFT
    PIDprocessB = fork();


    /* * * * * * * * * * * * * * * * * * * * * READ_WRITE_INFINITE_LOOP * * * * * * * * * * * * * * * * * * * * * * * */


    //generate sequence -> write to file & read from file -> calculate DFT  (inf)
    while (errNo == 0) {
        if (PIDprocessB == -1) {
            errNo = -1;
        }

        /* Main process - process A*/
        if (PIDprocessB != 0) {
            //open file for process A
            fd = open(argv[4], OPEN_FILE_FLAGS_A, mode);
            fd_info = open(INFO_FILE_NAME, OPEN_FILE_FLAGS_COMMON, mode);
            fd_action_a = open(PROCESS_A_LOG_PATH, OPEN_FILE_FLAGS_C, mode);

            if (fd == -1 || fd_info == -1 || fd_action_a == -1) {
                /* An error occured while create the file*/
                perror("Error : open file (process A)");
                errNo = 1;
            }


            //lock the files
            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(fd_info, F_SETLKW, &lock);

            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(fd, F_SETLKW, &lock);

            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(fd_action_a, F_SETLKW, &lock);
            //Process A log file updated
            memset(strLog, '\0', strlen(strLog));
            sprintf(strLog, "\nProcess A: files has been LOCKED");
            write(fd_action_a, strLog, strlen(strLog));



            //written last to first byte info of total bytes
            if (lseek(fd_info, 0, SEEK_SET) == (off_t) - 1) {
                perror("lseek error");
                errNo = 1;
            }
            read(fd_info, str, strlen(str));
            firstNumBytes = strToNumber(str);

            read(fd_info, str, strlen(str));
            lastNumBytes = strToNumber(str);

            read(fd_info, str, strlen(str));
            m = strToNumber(str);

            firstNumBytes = lastNumBytes;

            if (m == M){
                //Process B log file updated
                memset(strLog, '\0', strlen(strLog));
                sprintf(strLog, "\nProcess B: There are NO room for generate/write to sequence");
                write(fd_action_a, strLog, strlen(strLog));
            }

            if (m < M) {
                getNewSequence(sequence, N);
                //Process A log file updated
                memset(strLog, '\0', strlen(strLog));
                sprintf(strLog, "\nProcess A produced a random sequence for line %d : ", m+1);
                write(fd_action_a, strLog, strlen(strLog));
                for (int i = 0; i < N; ++i) {
                    memset(str, ';', strlen(str));
                    sprintf(str, "%09d ", sequence[i]);
                    write(fd_action_a, str, strlen(str));
                }



                fflush(stdout);
                fprintf(stdout, "\n[%ld]Process A: i’m producing a random sequence for line %d: ", getpid(), m + 1);
                fflush(stdout);

                //mean - M th row (M-m th row from last) {50-0  = 0 th seek}
                if (lseek(fd, m * N * strlen(str), SEEK_SET) == (off_t) - 1) {
                    perror("lseek error");
                    errNo = 0;
                }
                //process A writes a line to file
                //convert sequence to string and write to file
                for (int i = 0; i < N; ++i) {
                    memset(str, ';', strlen(str));
                    sprintf(str, "%09d;", sequence[i]);
                    lastNumBytes += write(fd, str, strlen(str));        //&str[0]
                    fprintf(stdout, " %d", sequence[i]);
                    fflush(stdout);
                }
                m++;


                //written last to first byte info of total bytes
                if (lseek(fd_info, 0, SEEK_SET) == (off_t) - 1) {
                    perror("lseek error");
                    errNo = 1;
                }
                sprintf(str, "%09d;", firstNumBytes);
                write(fd_info, str, strlen(str));

                //written last to first byte info of total bytes
                sprintf(str, "%09d;", lastNumBytes);
                write(fd_info, str, strlen(str));

                //written total line count
                sprintf(str, "%09d;", m);
                write(fd_info, str, strlen(str));


                //Process A log file updated
                memset(strLog, '\0', strlen(strLog));
                sprintf(strLog, "\nProcess A: Sequence has been written succesfly to line %d : ", m);
                write(fd_action_a, strLog, strlen(strLog));

            }
            //unlock
            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &lock);
            fcntl(fd_info, F_SETLKW, &lock);
            fcntl(fd_action_a, F_SETLKW, &lock);
            //Process A log file updated
            memset(strLog, '\0', strlen(strLog));
            sprintf(strLog, "\nProcess A: files has been UNLOCKED");
            write(fd_action_a, strLog, strlen(strLog));

            while (close(fd) == -1 || close(fd_info) == -1 || close(fd_action_a) == -1) {
                perror("close problem - process A - close the file again");
            }

        }



        //child process - process B
        if (PIDprocessB == 0) {
            //   fprintf(stdout, " \n[x]first :%d last :%d m :%d \n", firstNumBytes, lastNumBytes, m);

            //open files for process B
            fd = open(argv[4], OPEN_FILE_FLAGS_B, mode);
            fd_info = open(INFO_FILE_NAME, OPEN_FILE_FLAGS_COMMON, mode);
            fd_action_b = open(PROCESS_B_LOG_PATH, OPEN_FILE_FLAGS_C, mode);

            if (fd == -1 || fd_info == -1 || fd_action_b == -1) {
                /* An error occured while create the file*/
                perror("Error : open file (process B)");
                errNo = 1;
            }


            //lock the files
            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(fd_info, F_SETLKW, &lock);

            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(fd, F_SETLKW, &lock);

            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            fcntl(fd_action_b, F_SETLKW, &lock);

            //Process B log file updated
            memset(strLog, '\0', strlen(strLog));
            sprintf(strLog, "\nProcess B: files has been LOCKED");
            write(fd_action_b, strLog, strlen(strLog));

            if (lseek(fd_info, 0, SEEK_SET) == (off_t) - 1) {
                perror("lseek error");
                errNo = 1;
            }
            read(fd_info, str, strlen(str));
            firstNumBytes = strToNumber(str);
            read(fd_info, str, strlen(str));
            lastNumBytes = strToNumber(str);
            read(fd_info, str, strlen(str));
            m = strToNumber(str);

            if (m == 0){
                //Process B log file updated
                memset(strLog, '\0', strlen(strLog));
                sprintf(strLog, "\nProcess B: There are NO sequence in the file for calculate to DFT");
                write(fd_action_b, strLog, strlen(strLog));
            }

            if (m > 0) {
                fprintf(stdout, "\n[%ld]Process B: The DFT of line %d (", getpid(), m);
                fflush(stdout);

                if (lseek(fd, (m - 1) * N * (strlen(str)), SEEK_SET) == (off_t) - 1) {
                    perror("lseek error");
                    errNo = 1;
                }
                for (int i = 0; i < N; ++i) {
                    memset(str, ';', strlen(str));
                    lastNumBytes -= read(fd, str, strlen(str));
                    sequence[i] = strToNumber(str);
                    fprintf(stdout, "%d ", sequence[i]);
                    fflush(stdout);
                }
                fprintf(stdout, ") is: ", getpid(), m);
                fflush(stdout);


                //Calculate1D DFT and print it
                calcDFT(sequence, N);




                //delete M th row
                if (lseek(fd, (m - 1) * N * (strlen(str)), SEEK_SET) == (off_t) - 1) {
                    perror("lseek error");
                    errNo = 1;
                }
                for (int i = 0; i < N; ++i) {
                    memset(str, ' ', strlen(str));
                    write(fd, str, strlen(str));
                }



                //mean - the last row is deleted
                firstNumBytes += N * strlen(str);
                m--;


                if (lseek(fd_info, 0, SEEK_SET) == (off_t) - 1) {
                    perror("lseek error");
                    errNo = 1;
                }
                //write to info file -update
                sprintf(str, "%09d;", firstNumBytes);
                write(fd_info, str, strlen(str));

                //written last to first byte info of total bytes
                sprintf(str, "%09d;", lastNumBytes);
                write(fd_info, str, strlen(str));

                //written total line count
                sprintf(str, "%09d;", m);
                write(fd_info, str, strlen(str));



               //Process B log file updated
                memset(strLog, '\0', strlen(strLog));
                sprintf(strLog, "\nProcess B: Calculated DFT of line %d. Sequence was ", m+1);
                write(fd_action_b, strLog, strlen(strLog));
                for (int i = 0; i < N; ++i) {
                    memset(str, ';', strlen(str));
                    sprintf(str, "%09d ", sequence[i]);
                    write(fd_action_b, str, strlen(str));
                }

            }

            //unlock
            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &lock);
            fcntl(fd_info, F_SETLKW, &lock);
            fcntl(fd_action_b, F_SETLKW, &lock);

            //Process B log file updated
            memset(strLog, '\0', strlen(strLog));
            sprintf(strLog, "\nProcess B: files has been UNLOCKED");
            write(fd_action_b, strLog, strlen(strLog));

            while (close(fd) == -1 || close(fd_info) == -1 || close(fd_action_b) == -1) {
                perror("close problem - process B - close the file again");
            }
        }
    }


    if (errNo == 1) {
        perror("file open error.");
        exit(1);
    }
    if (errNo == -1) {
        perror("fork() create error.");
        exit(1);
    }

    return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * EN_OF_MAIN * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/*Generate a sequence with random number*/
int getNewSequence(int sequence[], int N) {


    for (int i = 0; i < N; ++i) {
        sequence[i] = (rand() % MAX_RAND_NUM);
    }

#ifndef NDEBUG
    fflush(stdout);
    fprintf(stdout,"\nGenerated : ");
    for (int i = 0; i < N; ++i) {
        fprintf(stdout, "%d ", sequence[i]);
    }
#endif


    return 0;
}



/*Convert string to integer*/
int strToNumber(char str[]){
    int num = 0;

    if (str[0]== ';')
        return num;

    for (int i = 0; i < strlen(str)-1 && str[i] != ';'; ++i) {
        num = num +  (int)( (int)(str[i]-'0') * pow (10,  strlen(str) - i - 2)) ;
    }

    return num;
}


/* this implement include some referance from
                                batchloaf.wordpress.com
   parameter an sequence array. Computing 1D DFT for each element*/
int calcDFT(int sequence[], int N) {
    // time and frequency domain data arrays
    int n, k;             // indices for time and frequency domains
    float x[N];           // discrete-time signal, x
    float Xre[N], Xim[N]; // DFT of x (real and imaginary parts)


    for (n = 0; n < N; ++n)
        x[n] = sequence[n];

    // Calculate DFT of x using brute force
    for (k = 0; k < N; ++k) {
        // Real part of X[k]
        Xre[k] = 0;
        for (n = 0; n < N; ++n)
            Xre[k] += x[n] * cos(n * k * PI * 2 / N);

        // Imaginary part of X[k]
        Xim[k] = 0;
        for (n = 0; n < N; ++n)
            Xim[k] -= x[n] * sin(n * k * PI * 2 / N);
    }


    //print format of DFT of each element
    for (k = 0; k < N; ++k) {
        fprintf(stdout, " [%.02f + %.02fi]", Xre[k], Xim[k]);
        fflush(stdout);
    }

}


/* * * * * * ** * * * * * * * * * * * * * * * EN_OF_IMPLEMENTATION* * * * * * * * * * * * * * * * * * * * * * * * * * */