# ICS-ShellLab
Unix Shell Simulator

## Trick
### 代码复用
 
struct job_t* getjobid(char** argv)：直接根据argv获得对应的job，高抽象的封装    
void cont(struct job_t * job, int state)：根据当前状态选择是否发送继续信号，并且设置job状态     
sigset_t getMask(int signal)： 返回单一信号的掩码    

### 优化
volatile int fgPid：  为了优化垃圾的fg_pid线性查找函数特意定义的变量，O（n）-> O(1),让fg_pid现在直接返回这个全局变量了。      
