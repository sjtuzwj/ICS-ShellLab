
void eval(char *cmdline)
{
    //step 1 初始化各变量以及信号阻塞合集
    char* argv[MAXARGS];
    char buf[MAXLINE];
    int state;
    int argc;
    pid_t curr_pid;//储存当前前台pid
    sigset_t mask_all, mask_one, mask_prev;
 
    //设置阻塞集合
    sigemptyset(&mask_one);
    sigaddset(&mask_one, SIGCHLD);
    sigfillset(&mask_all);
 
    //step 2 解析命令行，得到是否是后台命令，置位state
    strcpy(buf, cmdline);
    state = parseline(buf, argv, &argc)? BG : FG;
 
    //step 3 判断是否时内置命令
    if(!builtin_cmd(argv, argc)){
        //不是内置命令，阻塞SIGCHLD,防止子进程在父进程之间结束，也就是addjob和deletejob之间，必须保证这个拓扑顺序
        sigprocmask(SIG_BLOCK, &mask_one, &mask_prev);
        if((curr_pid = fork()) == 0){
            //子进程，先解除对SIGCHLD阻塞
            sigprocmask(SIG_SETMASK, &mask_prev, NULL);
            //改进进程的进程组，不要跟tsh进程在一个进程组，然后调用exevce函数执行相关的文件。
            setpgid(0, 0);
            if(execve(argv[0], argv, environ) < 0){
                //没找到相关可执行文件的情况下，打印消息，直接退出
                printf("%s: Command not found.\n", argv[0]);
            }
            //这里务必加上exit(0)，否则当execve函数无法执行的时候，子进程开始运行主进程的代码，出现不可预知的错误。
            exit(0);
        }
        //step 4
        //创建完成子进程后，父进程addjob,整个函数执行期间，必须保证不能被中断。尤其是玩意在for循环过程中中断了不堪设想
        //因此，阻塞所有信号，天塌下来也要让我先执行完
        sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(jobs, curr_pid, state, cmdline);
        //再次阻塞SIGCHLD
        sigprocmask(SIG_SETMASK, &mask_one, NULL);
 
        //step 5 判断是否是bg,fg调用waifg函数等待前台运行完成，bg打印消息即可
        //还有一个问题是，如果在前台任务，如果我使用默认的waitpid由于该函数是linux定义的原子性函数，无法被信号中断，那么前台
        //函数在执行的过程中，无法相应SIGINT和SIGSTO信号，这里我使用sigsuspend函数加上while判断fg_stop_or_exit标志的方法。具体见waitfg函数
        if(state == FG){
            waitfg(curr_pid);
        }
        else{
            //输出后台进程的信息
            //读取全局变量，阻塞所有的信号防止被打断
            sigprocmask(SIG_BLOCK , &mask_all, NULL);
            struct job_t* curr_bgmask = getjobpid(jobs, curr_pid);
            printf("[%d] (%d) %s", curr_bgmask->jid, curr_bgmask->pid, curr_bgmask->cmdline);
        }
        //解除所有的阻塞
        sigprocmask(SIG_SETMASK, &mask_prev, NULL);
    }
    return;
}