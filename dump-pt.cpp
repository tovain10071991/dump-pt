// g++ -o dump-pt dump-pt.c
// ./dump-pt <program> <args...>

#include "maps-helper.h"
#include "elf-helper.h"
#include "debug-helper.h"

#include "pt-module.h"
// #define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/mman.h>

#include <vector>

using namespace std;

const char* pt_module_path = "/home/user/Documents/test/pt_module/pt-module.ko";
const char* module_param = "";
const char* pt_dev = "/dev/pt-module";

const char* pt_out_path = "./pt.out";

int main(int argc, char** argv) {
    assert(argc >= 2);
    // int ret;
/*    ret = access(pt_module_path, F_OK);
    if(ret) {
        perror("access pt-module failed");
        return -1;
    }
    // install module
    int module_fd = open(pt_module_path, O_RDONLY);
    if(module_fd == -1) {
        perror("open pt-module.ko failed");
        return -1;
    }
    ret = syscall(SYS_finit_module, module_fd, module_param, 0);
    if(ret && errno != EEXIST) {
        perror("install module failed");
        return -1;
    }
*/
    int pt_fd = open(pt_dev, O_RDONLY | O_CLOEXEC);
    if(pt_fd == -1) {
        perror("open pt_dev failed");
        return -1;
    }

    void* pt_info = mmap(NULL, 4UL << 19, PROT_READ, MAP_PRIVATE, pt_fd, 0);
    if(!pt_info) {
        perror("mmap failed");
        return -1;
    }

    int pt_out = open(pt_out_path, O_RDWR|O_CREAT | O_TRUNC);

    // start child
    int pid = fork();
    if(pid == -1) {
        perror("fork failed");
        return -1;
    }
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        switch(argc-2) {
            case 0: {
                execlp(argv[1], argv[1], NULL);
                break;
            }
            case 1: {
                execlp(argv[1], argv[1], argv[2], NULL);
                break;
            }
            case 2: {
                execlp(argv[1], argv[1], argv[2], argv[3], NULL);
                break;
            }
            case 3: {
                execlp(argv[1], argv[1], argv[2], argv[3], argv[4], NULL);
                break;
            }
            case 4: {
                execlp(argv[1], argv[1], argv[2], argv[3], argv[4], argv[5], NULL);
                break;
            }
            default: {
                printf("unsupported num of agrs: %d\n", argc-2);
                return -1;
            }
        }
    }
    int status;
    wait(&status);
    assert(WIFSTOPPED(status));

    // reach to entry point
    uint64_t entry_addr = get_entry(argv[1]);
    cont_break(pid, entry_addr);

    // dump maps
    // vector<map_t> maps = get_maps(pid);
    char filename[100] = "";
    sprintf(filename, "/proc/%d/maps", pid);
    copy_maps(filename, "./maps.out");

    // set cpu affinity
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu);
    sched_setaffinity(pid, sizeof(cpu_set_t), &cpu);

    if(ioctl(pt_fd, PT_SET_PID, pid)) {
        perror("set pid failed");
        return -1;
    }
    if(ioctl(pt_fd, PT_SET_STATUS, 1)) {
        perror("start pt failed");
        return -1;
    }

    while(1) {
        ptrace(PTRACE_CONT, pid, 0, 0);
        waitpid(pid, &status, 0);
        if(WIFSIGNALED(status)) {
            printf("child receives signal: %s\n", strsignal(WTERMSIG(status)));
        }
        if(WIFSTOPPED(status)) {
            printf("child is stopped: %s\n", strsignal(WSTOPSIG(status)));
            if(WSTOPSIG(status) == SIGTRAP) {
                printf("reach here, cache pt info\n");
                unsigned long buffer_size;
                if(ioctl(pt_fd, PT_GET_SIZE, &buffer_size)) {
                    perror("get buffer size failed");
                    goto end;
                }
                unsigned long size = write(pt_out, pt_info, buffer_size);
                if(size != buffer_size) {
                    printf("write pt info only: 0x%lx(buffer_size: 0x%lx)\n", size, buffer_size);
                    goto end;
                }
                if(fsync(pt_out)) {
                    perror("sync file failed");
                    goto end;
                }
                if(ioctl(pt_fd, PT_SET_STATUS, 1)) {
                    perror("start pt failed");
                    goto end;
                }
            }
        }
        else if(WIFEXITED(status)) {
            printf("child is exited: %d\n", WEXITSTATUS(status));
            break;
        }
    }
end:
    if(ioctl(pt_fd, PT_SET_STATUS, 2)) {
        perror("finish pt failed");
        return -1;
    }
    unsigned long buffer_size;
    if(ioctl(pt_fd, PT_GET_SIZE, &buffer_size)) {
        perror("get buffer size failed");
        return -1;
    }
    unsigned long size = write(pt_out, pt_info, buffer_size);
    if(size != buffer_size) {
        printf("write pt info only: 0x%lx(buffer_size: 0x%lx)\n", size, buffer_size);
        return -1;
    }
    if(fsync(pt_out)) {
        perror("sync file failed");
        goto end;
    }
    ptrace(PTRACE_DETACH, pid, 0, 0);
    wait(&status);
    printf("reach here\n");
    return 0;
}