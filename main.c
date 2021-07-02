#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/shared_region.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach/vm_region.h>
    
void list_regions(mach_port_t task, mach_msg_type_number_t count, vm_region_flavor_t flavor) {
    bool done = false;
    vm_region_basic_info_data_64_t info;
    vm_address_t addr = 1;
    vm_size_t size;
    mach_port_t object_name;
    vm_address_t prev_addr;
    vm_size_t prev_size;
    kern_return_t kr;

    while(!done) {
        kr = vm_region_64(task, &addr, &size, flavor,
                            (vm_region_info_t)&info, &count, &object_name);

        if (kr == KERN_SUCCESS) {
            printf("%lx\n", addr);

            prev_addr = addr;
            prev_size = size;
            addr = prev_addr + prev_size; 

            if(!addr)
                break;
        } else {
            printf("Error: can't lookup region: %s\n", mach_error_string(kr));
            done = true;
        }
     }
}

int main(int argc, const char * argv[]) {
    int pid = atoi(argv[1]);
    
    printf("Examining pid %d\n", pid);

    mach_port_t task;    
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);

    if (kr != KERN_SUCCESS) {
        perror("failed to get task");
    }

    if(getuid() && geteuid()) {
        perror("must be root");
    }

    list_regions(task, VM_REGION_BASIC_INFO_COUNT_64, VM_REGION_BASIC_INFO);
    //list_regions(task, VM_REGION_TOP_INFO_COUNT, VM_REGION_TOP_INFO);

    return 0;
}
