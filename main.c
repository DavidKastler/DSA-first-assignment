#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define INT_SIZE (int)sizeof(int)
#define MIN_MEMORY_SIZE 24
#define MIN_BLOCK_SIZE 16
#define SIZE_OF_HEAD_WITH_TAIL 2 * (int)sizeof(int)
#define FIRST_FREE_BLOCK_LOCATION 4
#define EMPTY 0

void *head;
// x/50db head

int setIntIntoRegion(int byteLocation, int value) {
    return *(int*) (head + byteLocation) = value;
}

/**
 * Gets integer from region
 * @param byteLocation  first byte of int
 * @return  located int
 */
int getIntFromRegion(int byteLocation) {
    return *(int *) (head + byteLocation);
}

int getHeadOfNextFreeBlock(int blockLocation) {
    return getIntFromRegion(blockLocation + INT_SIZE);
}

int getHeadOfPrevFreeBlock(int blockLocation) {
    return getIntFromRegion(blockLocation + 2 * INT_SIZE);
}

void setPointerToNextBlock(int blockLocation, int headOfNextBlock) {
    setIntIntoRegion(blockLocation + INT_SIZE, headOfNextBlock);
}

void setPointerToPrevBlock(int blockLocation, int headOfPrevBlock) {
    setIntIntoRegion(blockLocation + 2 * INT_SIZE, headOfPrevBlock);
}

int getPointerLocation(void* pointer) {
    long long div = (pointer - head);
    return (int) div;
}

void* getPointerFromLocation(int location) {
    return (head + location);
}

/**
 * Function that removes free memory block from list and lets you do anything with it
 * \n To remove block from list you have 4 cases:
 * \n 1. block is in the middle (just swap pointers)
 * \n 2. block is on start of a list (you need to change firstFreeBlock pointer)
 * \n 3. block is on the end (delete reference to it)
 * \n 4. it is last block (set firstFreeBlock to 0)
 * @param blockHeadLocation head location of block you want to remove from list
 */
bool removeBlockFromMemoryList(int blockHeadLocation) {
    // if block not is free
    if (getIntFromRegion(blockHeadLocation) < MIN_BLOCK_SIZE) {
        return false;
    }
    int nextFreeBlockHead = getHeadOfNextFreeBlock(blockHeadLocation);
    int prevFreeBlockHead = getHeadOfPrevFreeBlock(blockHeadLocation);
    // case 4 - it is last free block
    if (nextFreeBlockHead == EMPTY && prevFreeBlockHead == EMPTY) {
        setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, 0);
    }
    // case 3 - it is last block in chain
    else if (nextFreeBlockHead == EMPTY) {
        setPointerToNextBlock(prevFreeBlockHead, 0);
    }
    // case 2 - it is first block in chain
    else if (prevFreeBlockHead == EMPTY) {
        setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, nextFreeBlockHead);
    }
    // case 1 - it is in the middle of chain
    else {
        setPointerToNextBlock(prevFreeBlockHead, nextFreeBlockHead);
        setPointerToPrevBlock(nextFreeBlockHead, prevFreeBlockHead);
    }
    // deleting references from current block
    setPointerToNextBlock(blockHeadLocation, 0);
    setPointerToPrevBlock(blockHeadLocation, 0);

    return true;
}

/**
 * Function that creates free block without pointers into region through head pointer
 * @attention does not check if allocation does not override other blocks
 * @param blockLocation     location of start byte of block
 * @param blockSize         size of block you want to create
 * @param nextBlockHead     position of next block head
 * @param previousBlockHead position of previous block head
 */
void createFreeBlock(int blockLocation, int blockSize, int nextBlockHead, int previousBlockHead) {
    memset((head + blockLocation), 0, blockSize);
    setIntIntoRegion(blockLocation, blockSize);
    setIntIntoRegion(blockLocation + blockSize - INT_SIZE, blockSize);
    // next block
    setIntIntoRegion(blockLocation + INT_SIZE, nextBlockHead);
    // prev block
    setIntIntoRegion(blockLocation + 2 * INT_SIZE, previousBlockHead);
}

/**
 * Function that creates allocated block, it checks pointers of previously free block and acts accordingly
 * @attention this is using explicit list logic
 * @param blockHead start of the block you want to allocate
 * @param realBlockSize real block size -> wanted_size + head(4B) + tail(4B)
 */
void allocateBlock(int blockHead, int realBlockSize) {
    // saving pointers for later use after allocating new block
    int nextFreeBlockHead = getHeadOfNextFreeBlock(blockHead);
    int prevFreeBlockHead = getHeadOfPrevFreeBlock(blockHead);
    int freeBlockSize = getIntFromRegion(blockHead);
    // handling case when block was not empty or not big enough
    if (freeBlockSize < realBlockSize) {
        exit(1);
    }

    // if block can be split
    if ((freeBlockSize - realBlockSize) >= MIN_BLOCK_SIZE) {
        createFreeBlock(blockHead + realBlockSize, freeBlockSize - realBlockSize,
                        nextFreeBlockHead, prevFreeBlockHead);
        setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, blockHead + realBlockSize);
    } else {
        realBlockSize = freeBlockSize;
        // pointer shuffle since current free block is all allocated
        // it was last free block
        if (prevFreeBlockHead == EMPTY && nextFreeBlockHead == EMPTY) {
            setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, 0);
        }
        // if it is last block in chain
        else if (prevFreeBlockHead != EMPTY && nextFreeBlockHead == EMPTY) { //TODO opravit vnutro podmienok
            setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, nextFreeBlockHead);
            setPointerToPrevBlock(nextFreeBlockHead, 0);
        }
        // if it is first block in chain
        else if (prevFreeBlockHead == EMPTY && nextFreeBlockHead != EMPTY) {
            setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, nextFreeBlockHead);
            setPointerToPrevBlock(nextFreeBlockHead, 0);
        }
        // if it is in the middle of chain
        else {
            setPointerToNextBlock(prevFreeBlockHead, nextFreeBlockHead);
            setPointerToPrevBlock(nextFreeBlockHead, prevFreeBlockHead);
        }
    }

    memset((head + blockHead), -1, realBlockSize);
    // head
    setIntIntoRegion(blockHead, -realBlockSize);
    // foot
    setIntIntoRegion(blockHead + realBlockSize - INT_SIZE, -realBlockSize);


}

/**
 * Function that inits the memory
 * @example 0:4 bytes = size\n
 *          4:8 bytes = first free block location\n
 *          then first free block starts containing whole memory
 * @param region pointer to start of memory
 * @param size size of the memory
 */
void memory_innit(void *region, int size) {
    if (size < MIN_MEMORY_SIZE) {
        printf("Unable to allocate region that small");
        exit(1);
    }
    head = region;
    // clearing whole region of memory with zeros
    memset(head, 0, size);
    // first number refers to size
    setIntIntoRegion(0, size);
    // second number refers to fist free block
    setIntIntoRegion(4, 8);
    // create first free block
    int firstFreeBlockSize = size - 2 * INT_SIZE;
    setIntIntoRegion(8, firstFreeBlockSize);
    setIntIntoRegion(size - INT_SIZE, firstFreeBlockSize);
}

/**
 * Best fit algorithm that will output head location of best block to allocate
 * @param newBlockSize  size of block we want to allocate
 * @return  head location of best block to allocate
 */
int bestFit(const int newBlockSize) {
    int freeBlockHead = getIntFromRegion(FIRST_FREE_BLOCK_LOCATION);
    if (freeBlockHead == EMPTY) {
        return NULL;
    }
    int freeBlockSize = getIntFromRegion(freeBlockHead);

    int bestBlockHead = EMPTY;
    int bestBlockSize = INT_MAX;
    // if free block does not have pointer (it is 0) to next block it is last block
    while (freeBlockHead != EMPTY) {
        // if we found exact block we were looking for
        if (freeBlockSize == newBlockSize) {
            bestBlockHead = freeBlockHead;
            break;
        }
        // if block is big enough and is closer to desired size
        else if (freeBlockSize > newBlockSize && freeBlockSize < bestBlockSize) {
            bestBlockHead = freeBlockHead;
            bestBlockSize = freeBlockSize;
        }
        freeBlockHead = getHeadOfNextFreeBlock(freeBlockHead);
        freeBlockSize = getIntFromRegion(freeBlockHead);
    }
    return bestBlockHead;
}

void* memoryAlloc(int newBlockSize) {

    // TODO pridat mem_check

    // if first free block pointer is 0 than memory is full
    if (getIntFromRegion(FIRST_FREE_BLOCK_LOCATION) == EMPTY) {
        //printf("Memory is full.");
        return NULL;
    }
    // rounding to even number
    int realNewBlockSize = (newBlockSize % 2 == 1 ? newBlockSize + 1: newBlockSize) + SIZE_OF_HEAD_WITH_TAIL;
    int freeBlockHead = bestFit(realNewBlockSize);

    allocateBlock(freeBlockHead, realNewBlockSize);
    return (void *) (head + freeBlockHead);
}

int memoryFree(void *blockHeadPointer) {
    int memSize = getIntFromRegion(0);
    int sizeOfBlock =  -(*(int *) blockHeadPointer);
    int blockHead = getPointerLocation(blockHeadPointer);
    int firstFreeBlockHead = getIntFromRegion(FIRST_FREE_BLOCK_LOCATION);

    int predBlockSize = EMPTY;
    int predBlockHead = EMPTY;
    int succBlockSize = EMPTY;
    int succBlockHead = EMPTY;

    // if it is not trying to read memory head
    if (blockHead > 2 * INT_SIZE) {
        predBlockSize = getIntFromRegion(blockHead - INT_SIZE);
        if (predBlockSize >= MIN_BLOCK_SIZE) {
            predBlockHead = blockHead - predBlockSize;
        }
    }
    // if it is not trying to read beyond memory
    if (blockHeadPointer + sizeOfBlock <= head + memSize) {
        succBlockSize = *(int *) (blockHeadPointer + sizeOfBlock);
        if (succBlockSize >= MIN_BLOCK_SIZE) {
            succBlockHead = blockHead + sizeOfBlock;
        }
    }
    // if I have free block in on both sides
    if (predBlockHead > 0 && succBlockHead > 0) {
        int predNextBlockHead = getHeadOfNextFreeBlock(predBlockHead);
        int predPrevBlockHead = getHeadOfPrevFreeBlock(predBlockHead);
        int succNextBlockHead = getHeadOfNextFreeBlock(succBlockHead);
        int succPrevBlockHead = getHeadOfPrevFreeBlock(succBlockHead);
        // setting predecessor block pointers
        if (predPrevBlockHead != EMPTY && predNextBlockHead != succBlockHead) {
            setPointerToNextBlock(predPrevBlockHead, predNextBlockHead);
        }
        if (predNextBlockHead != EMPTY && predNextBlockHead != succBlockHead) {
            setPointerToPrevBlock(predNextBlockHead, predPrevBlockHead);
        }
        // setting successor block pointers
        if (succPrevBlockHead != EMPTY && succNextBlockHead != predBlockHead) {
            setPointerToNextBlock(succPrevBlockHead, succNextBlockHead);
        }
        if (succNextBlockHead != EMPTY && succPrevBlockHead != predBlockHead) {
            setPointerToPrevBlock(succNextBlockHead, succPrevBlockHead);
        }

        // setting first free block
        setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, predBlockHead);
        createFreeBlock(predBlockHead, predBlockSize + sizeOfBlock + succBlockSize,
                        firstFreeBlockHead, 0);
        setPointerToPrevBlock(firstFreeBlockHead, predBlockHead);

//        printf("NOT IMPLEMENTED: if I have free block in on both sides ");
    }
    // if successor block is free
    else if (predBlockHead <= 0 && succBlockHead > 0) {
        int succNextBlockHead = getHeadOfNextFreeBlock(succBlockHead);
        int succPrevBlockHead = getHeadOfPrevFreeBlock(succBlockHead);
        // shuffling pointers
        if (succNextBlockHead != EMPTY) {
            setPointerToPrevBlock(succNextBlockHead, succPrevBlockHead);
        }
        if (succPrevBlockHead != EMPTY) {
            setPointerToNextBlock(succPrevBlockHead, succNextBlockHead);
        }
        createFreeBlock(blockHead, succBlockSize + sizeOfBlock, firstFreeBlockHead, 0);
        if (firstFreeBlockHead == succBlockHead) {
            setPointerToPrevBlock(succNextBlockHead, blockHead);
        } else if (firstFreeBlockHead != 0) {
            setPointerToPrevBlock(firstFreeBlockHead, succBlockHead);
        } else {
            setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, blockHead);
        }
    }
    // if predecessor block is free
    else if (predBlockHead > 0 && succBlockHead <= 0) {
        int predNextBlockHead = getHeadOfNextFreeBlock(predBlockHead);
        int predPrevBlockHead = getHeadOfPrevFreeBlock(predBlockHead);
        // shuffling pointers
        if (predNextBlockHead != EMPTY) {
            setPointerToPrevBlock(predNextBlockHead, predPrevBlockHead);
        }
        if (predPrevBlockHead != EMPTY) {
            setPointerToNextBlock(predPrevBlockHead, predNextBlockHead);
        }
        // creating block and setting pointers to it
        createFreeBlock(predBlockHead, predBlockSize + sizeOfBlock, firstFreeBlockHead, 0);
        if (firstFreeBlockHead != 0) {
            setPointerToPrevBlock(firstFreeBlockHead, predBlockHead);
        }
    }
    // if nothing close is free
    else {
        createFreeBlock(blockHead, sizeOfBlock, firstFreeBlockHead, 0);
        if (firstFreeBlockHead != EMPTY) {
            setPointerToPrevBlock(firstFreeBlockHead, blockHead);
        }
        setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, blockHead);
    }
    return 1;
}

int memoryCheck(void *region) {
    int size = *(int *) (region);
    if (size > 0) {
        // pozrie patu, ak sa rovna, pointer je platny
        if (*(int *) (region + size) == size) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

void test_random(unsigned  int size, int min, int max ) {
    char region[size];
    void *block_array[5];
    int allocated_blocks = 0;
    int expected_allocation = 0;
    float fragmenation = 0;
    int allocated_size = 0;
    int random = 0;
    int allocated_memory = 0;

    memory_innit(region, size);

    for (int i = 0; i < 5; ++i)  // nastavenie na NULL
    {
        block_array[i] = NULL;
    }

    for (int i = 0; i < 5; ++i)  // alokovanie 5 blokov
    {
        random = min + (rand() % (max - min + 1));
        block_array[i] = memoryAlloc(random);
        if (random % 2 == 1)
        {
            random++;
        }
        if (block_array[i] != NULL)
        {
            allocated_size += random;
        }
    }


    for (int i = 0; i < 5; ++i) {
        if (block_array[i] != NULL)
        {
            allocated_blocks++;
        }
    }


    if (*(int*) head == 0) //ak sa zaplni cela pamat tak to bude -8 co je v tomto pripade nula
    {
        allocated_memory = size;
    } else
    {
        allocated_memory = size -  *(int*) (head + (*(int*) head));
    }

    expected_allocation = allocated_size;
    fragmenation =  ((float) expected_allocation / (float) allocated_memory) * 100;

    printf("Po pokuse nacitania 5 blokov do pamate sa podarilo alokovat %d blokov.\n", allocated_blocks);
    printf("Povodna velkost pamate bola %d zostatok pamate po alokovani %d blokov nahodnej velkosti bola %d bajtov, za idealnych podmienok by to bolo %d bajtov.\n", size, allocated_blocks, size - allocated_memory, size - expected_allocation);
    printf("Vziknuta fragmentacia bola %.2lf %%. \n\n\n\n", 100 - fragmenation);

    memset(head, 0, size);
}

void test (int size) {
    char region[size];
    int allocated_blocks = 0;
    int expected_size = 0;
    float fragmenation = 0;
    int remaining_memory = 0;
    int offset = 0;

    memory_innit(region, size);

    void *allocatedBlock1 = memoryAlloc(8);
    void *allocatedBlock2 = memoryAlloc(8);
    void *allocatedBlock3 = memoryAlloc(8);
    void *allocatedBlock4 = memoryAlloc(8);
//    void *all_block_5 = memoryAlloc(8);

    memoryFree(allocatedBlock3);
    memoryFree(allocatedBlock2);
    memoryFree(allocatedBlock1);


//    if ( allocatedBlock != NULL) { allocated_blocks ++;}
//    if ( allocatedBlock2 != NULL) { allocated_blocks ++;}
//    if ( allocatedBlock3 != NULL) { allocated_blocks ++;}
//    if ( allBlock4 != NULL) { allocated_blocks ++;}
//    if ( all_block_5 != NULL) { allocated_blocks ++;}
    // Tato cast kodu sa mi nepodarila implementovat
    /*offset = *(int*) (head + 24 + 4);
    if (*(int*) head == 0)
    {
        remaining_memory = 0;
    } else
    {
        offset = *(int*) (head + *(int*) (head));
        while(1)
        {
            remaining_memory += offset;
            offset = *(int*) (head + *(int*) (head + (offset + 4)));
            if (offset == 0)
            {
                break;
            }
        }
        remaining_memory = size - 3 * INT_SIZE - allocated_blocks * (8 + 2 * INT_SIZE);
    }

    expected_size = size - allocated_blocks * 8;
    fragmenation = ((float) remaining_memory/ (float) expected_size) * 100;

    printf("Povodna velkost pamate bola %d zostatok pamate po alokovani %d blokov nahodnej velkosti bola %d bajtov.\n", size, allocated_blocks,  remaining_memory);
    printf("Idealna velkost volnej pamate by bola %d\n", expected_size);
    printf("Vziknuta fragmentacia bola %lf %%. \n", fragmenation);*/


}

int main(void) {

    // nepodarilo sa mi cele implementovat
    //scenar 1
    test(100);


    //scenar 2
    test_random(50, 8, 24);
    test_random(100, 8, 24);
    test_random(200, 8, 24);


    //scenar 3
    test_random(10000, 500, 5000);


    //scenar 4
    test_random(100000, 8, 50000);

    return 0;
}