#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/shared_region.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach/vm_region.h>
#include "config.h"

void list_regions(mach_port_t task, mach_msg_type_number_t count, vm_region_flavor_t flavor, FILE *output_fd) {
    bool done = false;
    vm_region_basic_info_data_64_t info;
    vm_address_t addr = 1; // start at 1 and you'll get offsets for the proper regions
    vm_size_t size;
    mach_port_t object_name;
    vm_address_t prev_addr;
    vm_size_t prev_size;
    kern_return_t kr;
    pointer_t buffer_pointer;
    vm_offset_t *buf;
    uint32_t sz;

    do {
        // lookup info for region so we can determine its address, size,
        // and if it's writable and not shared
        kr = vm_region_64(task, &addr, &size, flavor,
                            (vm_region_info_t)&info, &count, &object_name);

        done = addr + size == 0x7000000000;

        if (!done && kr == KERN_SUCCESS) {
            // filter to writable, non-shared regions
            if(((info.protection & VM_PROT_WRITE) == VM_PROT_WRITE) && !info.shared) {
                buf = malloc(size);
                // read memory from subject
                kr = vm_read(task, addr, size, &buffer_pointer, &sz);
                // copy data into a buffer in our address space
                memcpy(buf, (const void *)buffer_pointer, sz);

                if (!done && kr == KERN_SUCCESS) {
                    fwrite(buf, size, 1, output_fd);
                } else {
                    fprintf(stderr, "%s\n", mach_error_string(kr));
                }

                free(buf);
            }

            prev_addr = addr;
            prev_size = size;
            addr = prev_addr + prev_size; 
        } else {
            done = true;
        }
     } while(!done);

}

void parse_args(int argc, const char *argv[], int *pid) {
    int opt;

    while ((opt = getopt(argc, argv, ":p:")) != -1) {
        switch(opt) {
            case 'p':
            *pid = atoi(optarg);
            break;
        }
    }

    if (optind < argc - 1) {
        fprintf(stderr, "Usage: %s -p <pid>", argv[0]);
        exit(1);
    }
}

int main(int argc, const char **argv) {
    int pid = -1;
    mach_port_t task;    

    parse_args(argc, argv, &pid);

    // get the task port of the proces to dump
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);

    if (kr != KERN_SUCCESS) {
        perror("failed to get task");
        exit(1);
    }

    if(getuid() && geteuid()) {
        perror("must be root");
        exit(1);        
    }

    FILE *output_fd = stdout;
    list_regions(task, VM_REGION_BASIC_INFO_COUNT_64, VM_REGION_BASIC_INFO, output_fd);
    fclose(output_fd);

    return 0;
}
