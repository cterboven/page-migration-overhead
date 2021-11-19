#include <iostream>
#include <cstdlib>

#include <numa.h>
#include <numaif.h>
#include <unistd.h>

#include <omp.h>



int main(int argc, char* argv[])
{
    // Intended usage: measure_page_migration_overhead n p x y
    //    n: number of pages
    //    p: pagesize
    //    x: initial numa node
    //    y: target numa node
    int n = atoi(argv[1]);
    int p = atoi(argv[2]);
    int x = atoi(argv[3]);
    int y = atoi(argv[4]);
    
    std::cout << "measure_page_migration_overhead: " << n << " pages of size " << p << " bytes to migrated from node " << x << " to node " << y << " ." << std::endl;

    // setup pointer array
    typedef void* ptr;
    ptr* pointers = new ptr[n];

    // initialize pointer array
    for (int i = 0; i < n; i++)
    {
        pointers[i] = numa_alloc_onnode(p, x);
        memset(pointers[i], 0, p);
    }
    
    // setup targets
    int* targets = new int[n];
    for (int i = 0; i < n; i++)
    {
        targets[i] = y;
    }
    
    int* status = new int[n];
    double t1 = omp_get_wtime();

    // long move_pages(int pid, unsigned long count, void **pages,
    //                 const int *nodes, int *status, int flags);
    long err = move_pages((int) getpid(), n, pointers, targets, status, MPOL_MF_MOVE);
    
    double t2 = omp_get_wtime();

    if (err == 0)
    {
        double time = t2 - t1;
        std::cout << "Time: " << time << "seconds." << std::endl;
    }
    else
    {
        switch(errno)
        {
            case E2BIG:
                std::cout << "Error E2BIG: Too many pages to move." << std::endl;
                break;
            case EACCES:
                std::cout << "Error EACCES: One of the target nodes is not allowed by the current cpuset." << std::endl;
                break;
            case EFAULT:
                std::cout << "Error EFAULT: Parameter array could not be accessed." << std::endl;
                break;
            case EINVAL:
                std::cout << "Error EINVAL: Flags other than MPOL_MF_MOVE and MPOL_MF_MOVE_ALL was specified or an attempt was made to migrate pages of a kernel thread." << std::endl;
                break;
            case ENODEV:
                std::cout << "Error ENODEV: One of the target nodes is not online." << std::endl;
                break;
            case ENOENT:
                std::cout << "Error ENOENT: No pages were found that require moving. All pages are either already on the target node, not present, had an invalid address or could not be moved because they were mapped by multiple processes." << std::endl;
                break;
            case EPERM:
                std::cout << "Error EPERM: The caller specified MPOL_MF_MOVE_ALL without sufficient privileges (CAP_SYS_NICE). Or, the caller attempted to move pages of a process belonging to another user but did not have privilege to do so (CAP_SYS_NICE)." << std::endl;
                break;
            case ESRCH:
                std::cout << "Error ESRCH: Process does not exist." << std::endl;
                break;
        }
    }
    
    // cleanup
    for (int i = 0; i < n; i++)
    {
        numa_free(pointers[i], p);
    }
    delete[] targets;
    delete[] pointers;
}
