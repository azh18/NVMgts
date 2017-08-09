#include "p_mmap.h"

static int SHM_SIZE = 0;

static char *pBaseAddr = NULL;
static int iBitsCount = 0;

static void* p_mmap(void* addr,unsigned long len,unsigned long prot,unsigned long id) {
	return (void*)syscall(__NR_p_mmap, addr, len, prot, id);
}

static int p_search_big_region_node(unsigned long id) {
	return (int)syscall(__NR_p_search_big_region_node, id);
}

static int p_delete_big_region_node(unsigned long id) {
	return (int)syscall(__NR_p_delete_big_region_node, id);
}

static int p_alloc_and_insert(unsigned long id, int size) {
    return (int)syscall(__NR_p_alloc_and_insert, id, size);
}

static int p_get_small_region(unsigned long id,unsigned long size) {
	return (int)syscall(__NR_p_get_small_region, id,size);
}

static int p_search_small_region_node(unsigned long id, void *poffset, void *psize) {
	return (int)syscall(__NR_p_search_small_region_node, id, poffset, psize);
}

static int p_bind_(unsigned long id, unsigned long offset, unsigned long size, unsigned long hptable_id) {
    return (int)syscall(__NR_p_bind, id, offset, size, hptable_id);
}


#define HPID    12345
/*
int p_init() {
    key_t key;
    int shmid;
    int mode;
    int iRet = 0;

    if ((key = ftok("/bin", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }

    if ((shmid = shmget(key, SHM_SIZE, 0777 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

    pAddr = shmat(shmid, (void *)0, 0);
    if (pAddr == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

    printf("get and attach succee! addr=%p\n", pAddr);

    return 0;
}
*/

int p_init(int size) {
    int iRet = 0;

    if (pBaseAddr != NULL) {
        return -1;
    }
    if (size < 0) {
        return -1;
    }

    SHM_SIZE = size;

    iRet = p_get_small_region(HPID, size);
    if (iRet < 0) {
        printf("Error:p_get_small_region call failed!\n");
        return -1;
    }

    pBaseAddr = p_mmap(NULL, size, PROT_READ | PROT_WRITE, HPID);
    if (!pBaseAddr) {
        printf("p_mmap return NULL!\n"); // No sense
        return -1;
    }

    /* If this pre-allocated native heap is not in use, clear it with setting all the memory bytes to be 0s */
    int *startLoc = (int *)pBaseAddr;
    pBaseAddr += 4;
    if (*startLoc != 4 || *(startLoc + 1) < 0 || *(startLoc + 1) > size-4*sizeof(int)) {
        *startLoc = 4;
        p_clear(size);
    }

    // printf("pBaseAddr = %p.\n", pBaseAddr);
}

void *p_get_base()
{
	return pBaseAddr;
}

int p_clear() {
    if (!pBaseAddr) {
        printf("Error: call p_init() first!\n");
        return -1;
    }
    // set all the bytes to be 0s in this heap
    //memset(pBaseAddr+4, 0, SHM_SIZE-4);
    /* Set the first contiguous free chunk */
    *(int *)pBaseAddr = 0; // id = 0, free chunk
    *((int *)pBaseAddr + 1) = SHM_SIZE - 4 * sizeof(int); // length = size - (sizeof(startLoc) + sizeof(nid) + sizeof(length) + sizeof(nextLoc)
    *((int *)pBaseAddr + 2) = 0; // a flag for stop searching
    return 0;
}

/* 2018-08-07: a new p_malloc implementation without bitmap but pid as input parameter */
void *p_malloc(int pid, int size) {
    if (!pBaseAddr) {
        printf("Error: call p_init first!\n");
        return NULL;
    }

    void *pLeapAddr = pBaseAddr;
    int nid, length;
    int nextLoc = 1;
    int toAllocateSize = size + 3 * sizeof(int);

    /* First fit for allocation in user heap */
    while (nextLoc != 0) {
        nid = *(int *)pLeapAddr; // nid = 0 means free
        length = *((int *)pLeapAddr + 1); // length is the size of free/used chunk
        nextLoc = *((int *)pLeapAddr + 2); // the offset of next starting address for free/used chunk
        // nosense = *((int *)pLeapAddr + 3); // need it or not?

        /* find a free chunk with enough space */
        if (nid == 0 && length >= toAllocateSize) {
            //
            printf("Find a free chunk with enough space %d bytes for applied size %d.\n", length, size);
            /* set important metada for current and next chunk */
            /* For current chunk */
            *(int *)pLeapAddr = pid;
            *((int *)pLeapAddr + 1) = size;
            *((int *)pLeapAddr + 2) = toAllocateSize;
            /* For next chunk */
            void *pNextAddr = pLeapAddr + toAllocateSize;
            *(int *)pNextAddr = 0; // free chunk
            *((int *)pNextAddr + 1) = length - toAllocateSize; //
            *((int *)pNextAddr + 2) = 0; // stop flag for search


            /* return the used address for users */
            return (pLeapAddr + 3 * sizeof(int));
        }

        /* if in this round a free chunk not found or size not satisfied, continue */
        pLeapAddr += 3 * sizeof(int) + length;
    }

    /* nextLoc == 0 but no free chunk satisifies the applied space */
    printf("No free memory, please p_init a larger space or clear the heap memory instead.\n");
    return NULL;
}


/* p_free is corresponding to p_malloc */
int p_free(int pid) {
    if (!pBaseAddr) {
        printf("Error: call p_init first!\n");
        return -1;
    }
    void *pLeapAddr = pBaseAddr;

    int nid, length, nosense;
    int nextLoc = 1;
    while (nextLoc != 0) {
        nid = *(int *)pLeapAddr; // nid = 0 means free
        length = *((int *)pLeapAddr + 1); // length is the size corresponding to p_malloc
        nextLoc = *((int *)pLeapAddr + 2); // the offset of next starting address for free/used chunk
        // nosense = *((int *)pLeapAddr + 3); // need it or not?

        if (nid == pid) {
            printf("Find id %d in the heap.\n", pid);
	    /* clear this region by setting all bytes to be 0s */
            memset(pLeapAddr + 3*sizeof(int), 0, length);
            *(int *)pLeapAddr = 0; // pid = 0, free chunk; no need to change length

            // TODO: recycle contigous free memory in PM
            void *pNextAddr = pLeapAddr + nextLoc;
            if (*(int *)pNextAddr == 0) {
                *((int *)pLeapAddr + 1) = length + 3 * sizeof(int) + *((int *)pNextAddr + 1); // length update
                *((int *)pLeapAddr + 2) = *((int *)pNextAddr + 2); // nextLoc update

                *(int *)pNextAddr = *((int *)pNextAddr + 1) = *((int *)pNextAddr + 2) = 0;
            }
            // continue; // Delete all data with the same pid
            return 0;
        }

        pLeapAddr += (length + 3 * sizeof(int));
    }

    /* reach the end of the allocated memory region and pid not found */
    if (nextLoc == 0) {
        printf("Cannot find pid %d in native heap.\n", pid);
        return -1;
    }

    printf("Unknown type of error occured!\n");
    return -1;
}

char *p_get_malloc(int pid) {
    if (!pBaseAddr) {
        printf("Error: call p_init first!\n");
        return NULL;
    }
    char *pLeapAddr = (char *)pBaseAddr;

    int nid, length, nosense;
    int nextLoc = 1;
    while (nextLoc != 0) {
        nid = *(int *)pLeapAddr; // nid = 0 means free
        length = *((int *)pLeapAddr + 1); // length is the size corresponding to p_malloc
        nextLoc = *((int *)pLeapAddr + 2); // the offset of next starting address for free/used chunk
        // nosense = *((int *)pLeapAddr + 3); // need it or not?

        if (nid == pid) {
            printf("Find id %d in the heap.\n", pid);
            return (char*)(pLeapAddr + 3 * sizeof(int));
        }
        /* If not found in this round */
        pLeapAddr += (length + 3 * sizeof(int));
    }

    /* reach the end of the allocated memory region and pid not found */
    if (nextLoc == 0) {
        printf("Cannot find pid %d in native heap.\n", pid);
        return NULL;
    }

    printf("Unknown type of error occured!\n");
    return NULL;
}




int p_delete(int pId) {
    /*
    p_unmap(pId);
    p_tab_delete(pId);
    */
    return p_delete_big_region_node(pId);
}

void *p_get(int pId, int iSize) {
    int iRet = 0;

    iRet = p_search_big_region_node(pId);
    if (!iRet) {
        printf("cannot find %d in big region\n", pId);
        return NULL;
    }

    void *pAddr = p_mmap(NULL, iSize, PROT_READ | PROT_WRITE, pId);
    if (!pAddr) {
        printf("p_mmap return NULL\n");
    }

    return pAddr;
}

int p_bind(int id, void *ptr, int size) {
    int offset = (int)(ptr - (void *)pBaseAddr);
    if (offset < 0 || size < 0) {
        return -1;
    }

    return p_bind_(id, offset, size, HPID);
}
/*
void *p_get_bind_node(int pId, int *psize) {
    int iRet = 0;
    int offset;

    iRet = p_search_small_region_node(pId, &offset, psize);
    if (iRet < 0) {
        printf("p_search_small_region_node error\n");
        return NULL;
    }

    return (void *)pBaseAddr + offset;
}
*/
