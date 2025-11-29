#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_student_pid = 0;
volatile sig_atomic_t active_students = 0;

void teacher_usr1_handler(int sig, siginfo_t *info, void *ucontext){
    last_student_pid = info->si_pid;
}

void teacher_chld_handler(int sig){
    while(waitpid(-1, NULL, WNOHANG)>0){
        active_students--;
    }
}

void student_usr2_handler(int sig){}

void set_siginfo_handler(int sigNo, void (*f)(int, siginfo_t *, void *)){

    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    if(sigaction(sigNo,&act,NULL) == -1)ERR("sigaction");

}

void set_simple_handler(int sigNo, void(*f)(int)){
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    act.sa_flags = 0; // No SA_RESTART, we want to interrupt sigsuspend
    sigemptyset(&act.sa_mask);
    if (sigaction(sigNo, &act, NULL) == -1) ERR("sigaction");
}



int main(int argc, char** argv){
    if(argc < 4){
        fprintf(stderr, "Usage: %s p t prob1 [prob2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int p = atoi(argv[1]);
    int t = atoi(argv[2]);
    int num_students =  argc - 3;

    active_students = num_students;

    set_siginfo_handler(SIGUSR1, teacher_usr1_handler);
    set_simple_handler(SIGCHLD, teacher_chld_handler);

    // Block SIGUSR1 and SIGCHLD so we can wait for them safely
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);


    if (p <= 0 || t <= 0) ERR("Invalid p or t");

    printf("[TEACHER] Simulation started: %d students, %d parts, factor %d\n", num_students, p, t);

    

    for (int i = 0; i < num_students; i++)
    {
        pid_t pid = fork();
        if(pid<0)ERR("fork");

        int prob = atoi(argv[3 + i]);

        if(pid == 0){
            srand(time(NULL) ^ getpid());

            set_simple_handler(SIGUSR2, student_usr2_handler);

            // 2. Block SIGUSR2 so we don't miss the ACK
            sigset_t child_mask, child_oldmask;
            sigemptyset(&child_mask);
            sigaddset(&child_mask, SIGUSR2);
            sigprocmask(SIG_BLOCK, &child_mask, &child_oldmask);

            struct timespec work_time = {0, t * 100 * 1000000};
            printf("Student[%d,%d] has started doing task! probability of %d\n", i, getpid(), prob);

            for (int part = 1; part <= p; part++)
            {
                printf("Student[%d,%d] is starting doing part %d of %d\n", i, getpid(), part,p);

                nanosleep(&work_time, NULL);

                printf("Student[%d,%d] has finished part %d of %d\n", i, getpid(), part,p);
                if (kill(getppid(), SIGUSR1) < 0) ERR("kill");

                sigsuspend(&child_oldmask);
            }
            

            printf("Student[%d, %d] has completed the task\n", i, getpid());
        
            exit(EXIT_SUCCESS);
        }
    }

    while(active_students > 0){
        sigsuspend(&oldmask);

        if(last_student_pid != 0){
            printf("Teacher has accepted solution of student [%d].\n",last_student_pid);

            kill(last_student_pid,SIGUSR2);

            last_student_pid = 0;
        }
    }

    printf("[TEACHER] All students left.\n");
    return EXIT_SUCCESS;
    
}