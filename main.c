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

// global variable to store head of memory
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
 * @return true/false if operation was successful
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
 * @details if you do not specify any pointer it will add it as first block in chain
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
    // if at least one of pointers is set
    if (previousBlockHead != EMPTY || nextBlockHead != EMPTY) {
        // next block
        setPointerToNextBlock(blockLocation, nextBlockHead);
        // prev block
        setPointerToPrevBlock(blockLocation, previousBlockHead);
        return;
    }
    // if it is supposed to be first block in chain
    int firstFreeBlock = getIntFromRegion(FIRST_FREE_BLOCK_LOCATION);
    if (firstFreeBlock != EMPTY) {
        setPointerToPrevBlock(firstFreeBlock, blockLocation);
        setPointerToNextBlock(blockLocation, firstFreeBlock);
    }
    setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, blockLocation);
}

/**
 * Function that creates allocated block, it checks pointers of previously free block and acts accordingly
 * @attention this is using explicit list logic
 * @param blockHead start of the block you want to allocate
 * @param realBlockSize real block size -> wanted_size + head(4B) + tail(4B)
 */
void* allocateBlock(int blockHead, int realBlockSize) {
    // saving pointers for later use after allocating new block
    int nextFreeBlockHead = getHeadOfNextFreeBlock(blockHead);
    int prevFreeBlockHead = getHeadOfPrevFreeBlock(blockHead);
    int freeBlockSize = getIntFromRegion(blockHead);
    // handling case when block was not empty or not big enough
    if (freeBlockSize < realBlockSize) {
        return NULL;
    }
    // if block can be split
    if ((freeBlockSize - realBlockSize) >= MIN_BLOCK_SIZE) {
        createFreeBlock(blockHead + realBlockSize, freeBlockSize - realBlockSize,
                        nextFreeBlockHead, prevFreeBlockHead);
        // this is to counter act if it is last block and it creates reference to block that is about to get allocated
        if (nextFreeBlockHead == EMPTY && prevFreeBlockHead == EMPTY) {
            setPointerToNextBlock(blockHead + realBlockSize, 0);
        }
    } else {
        realBlockSize = freeBlockSize;
        removeBlockFromMemoryList(blockHead);
    }
    // creating allocated block
    memset((head + blockHead), -1, realBlockSize);
    // head
    setIntIntoRegion(blockHead, -realBlockSize);
    // foot
    setIntIntoRegion(blockHead + realBlockSize - INT_SIZE, -realBlockSize);
    return (void *) (head + blockHead);
}

/**
 * Function that inits the memory
 * @example 0:4 bytes = size\n
 *          4:8 bytes = first free block location\n
 *          then first free block starts containing whole memory
 * @param region pointer to start of memory
 * @param size size of the memory
 */
bool memoryInnit(void *region, int size) {
    if (size < MIN_MEMORY_SIZE) {
        printf("Unable to allocate region that small");
        return false;
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
    return true;
}

/**
 * Best fit algorithm that will output head location of best block to allocate
 * @param newBlockSize  size of block we want to allocate
 * @return  head location of best block to allocate
 */
int bestFit(const int newBlockSize) {
    int freeBlockHead = getIntFromRegion(FIRST_FREE_BLOCK_LOCATION);
    if (freeBlockHead == EMPTY) {
        return EMPTY;
    }
    int freeBlockSize = getIntFromRegion(freeBlockHead);
    int bestBlockHead = EMPTY;
    int bestBlockSize = freeBlockSize;
    // if free block does not have pointer (it is 0) to next block it is last block
    while (freeBlockHead != EMPTY) {
        // if we found exact block we were looking for
        if (freeBlockSize == newBlockSize) {
            bestBlockHead = freeBlockHead;
            break;
        }
        // if block is big enough and is closer to desired size
        else if (freeBlockSize > newBlockSize && freeBlockSize <= bestBlockSize) {
            bestBlockHead = freeBlockHead;
            bestBlockSize = freeBlockSize;
        }
        freeBlockHead = getHeadOfNextFreeBlock(freeBlockHead);
        freeBlockSize = getIntFromRegion(freeBlockHead);
    }
    return bestBlockHead;
}

/**
 * Function that allocates memory and gives you pointer to allocated block (NULL if memory was full)
 * @param newBlockSize  wanted size of memory block
 * @return  reference to that block
 */
void* memoryAlloc(int newBlockSize) {
    // if first free block pointer is 0 than memory is full
    if (getIntFromRegion(FIRST_FREE_BLOCK_LOCATION) == EMPTY) {
        //printf("Memory is full.");
        return NULL;
    }
    // rounding to even number to simulate small fragmentation
    int realNewBlockSize = (newBlockSize % 2 == 1 ? newBlockSize + 1: newBlockSize) + SIZE_OF_HEAD_WITH_TAIL;
    int freeBlockHead = bestFit(realNewBlockSize);
    if (freeBlockHead == EMPTY) {
        return NULL;
    }
    return allocateBlock(freeBlockHead, realNewBlockSize);
}

/**
 * Function that checks if allocated blockPointer is still valid
 * @param blockPointer  block you want to check
 * @return  true/false if operation was successful
 */
bool memoryCheck(void *blockPointer) {
    if (blockPointer == NULL) {
        return false;
    }
    const int memorySize = getIntFromRegion(0);
    const int blockLocation = getPointerLocation(blockPointer);
    const int blockHeadSize = getIntFromRegion(blockLocation);
    // if it is allocated
    if (blockHeadSize >= MIN_BLOCK_SIZE) {
        return false;
    }
    // if block would reach outside memory
    if ((blockLocation + abs(blockHeadSize)) > memorySize) {
        return false;
    }
    const int blockFootSize = getIntFromRegion(blockLocation + abs(blockHeadSize) - INT_SIZE);
    // if footer and header does not match
    if (blockHeadSize != blockFootSize) {
        return false;
    }
    return true;
}

/**
 * Function to release memory and push it to the list of free memory blocks
 * @param blockHeadPointer block pointer you want to free
 * @return true/false if operation was successful
 */
bool memoryFree(void *blockHeadPointer) {
    if (memoryCheck(blockHeadPointer) == false) {
        return false;
    }
    int memSize = getIntFromRegion(0);
    int sizeOfBlock =  abs(*(int *) blockHeadPointer);
    int blockHead = getPointerLocation(blockHeadPointer);
    // getting successor and predecessor pointers
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
    if ((blockHead + sizeOfBlock + INT_SIZE) <= memSize) {
        succBlockSize = getIntFromRegion(blockHead + sizeOfBlock);
        if (succBlockSize >= MIN_BLOCK_SIZE) {
            succBlockHead = blockHead + sizeOfBlock;
        }
    }
    // if I have free block in on both sides
    if (predBlockHead > 0 && succBlockHead > 0) {
        if (removeBlockFromMemoryList(succBlockHead) == false) {
            return false;
        }
        if (removeBlockFromMemoryList(predBlockHead) == false) {
            return false;
        }
        int currentFreeBlockSize = predBlockSize + sizeOfBlock + succBlockSize;
        createFreeBlock(predBlockHead, currentFreeBlockSize,0, 0);
    }
    // if successor block is free
    else if (predBlockHead <= 0 && succBlockHead > 0) {
        if (removeBlockFromMemoryList(succBlockHead) == false) {
            return false;
        }
        createFreeBlock(blockHead, succBlockSize + sizeOfBlock, 0, 0);
    }
    // if predecessor block is free
    else if (predBlockHead > 0 && succBlockHead <= 0) {
        if (removeBlockFromMemoryList(predBlockHead) == false) {
            return false;
        }
        createFreeBlock(predBlockHead, predBlockSize + sizeOfBlock, 0, 0);
    }
    // if nothing close is free
    else {
        createFreeBlock(blockHead, sizeOfBlock, 0, 0);
    }
    return true;
}

/**
 * Test of creating 5 random size blocks
 * @param SIZE memory size
 * @param MIN min block size
 * @param MAX max block size
 */
void testRandom(const int SIZE, const int MIN, const int MAX ) {
    char region[SIZE];
    const int NUMBER_OF_BLOCKS = 5;
    void* blockArray[NUMBER_OF_BLOCKS];
    for (int i = 0; i < NUMBER_OF_BLOCKS; ++i) {
        blockArray[i] = NULL;
    }

    memoryInnit(region, SIZE);

    // allocation of 5 blocks
    int allocatedBlocksCount = 0;
    int allocatedSize = 0;
    int realAllocatedSize = 0;
    int randomSize;
    for (int i = 0; i < NUMBER_OF_BLOCKS; ++i) {
        randomSize = MIN + (rand() % (MAX - MIN + 1));
        blockArray[i] = memoryAlloc(randomSize);
        if (blockArray[i] != NULL) {
            realAllocatedSize += -getIntFromRegion(getPointerLocation(blockArray[i]));
            allocatedSize += -getIntFromRegion(getPointerLocation(blockArray[i])) - SIZE_OF_HEAD_WITH_TAIL;
            allocatedBlocksCount++;
        }
    }

    float fragmentation = ((float) allocatedSize / (float) realAllocatedSize) * 100;

    printf("Tried to allocate %d blocks. Successfully allocated %d Blocks.\n", NUMBER_OF_BLOCKS, allocatedBlocksCount);
    printf("Original memory size was %d. Remaining memory size after allocation of blocks with random size was %d bytes, in ideal contitions it would be %d bytes.\n",
           SIZE,  SIZE - realAllocatedSize - 2 * INT_SIZE, SIZE - allocatedSize - 2 * INT_SIZE);
    printf("Fragmentation was %lf %%. \n\n", fragmentation);

    memset(head, 0, SIZE);
}

/**
 * Test of creating 5 same size blocks and freeing some of them (depends on which you comment out)
 * @param SIZE memory size
 */
void test(const int SIZE) {
    const int BLOCK_SIZE = 8;
    char region[SIZE];

    memoryInnit(region, SIZE);

    // list of all blocks
    void* blockArray[5] = {NULL};
    blockArray[0] = memoryAlloc(BLOCK_SIZE);
    blockArray[1] = memoryAlloc(BLOCK_SIZE);
    blockArray[2] = memoryAlloc(BLOCK_SIZE);
    blockArray[3] = memoryAlloc(BLOCK_SIZE);
    blockArray[4] = memoryAlloc(BLOCK_SIZE);

    int allocatedBlocksCount = 0;
    for (int i = 0; i < sizeof(blockArray) / sizeof(blockArray[0]); ++i) {
        if (blockArray[i] != NULL) {
            allocatedBlocksCount++;
        }
    }

    memoryFree(blockArray[0]);
//    memoryFree(blockArray[3]);
    memoryFree(blockArray[1]);
//    memoryFree(blockArray[2]);
    memoryFree(blockArray[4]);

    // number of all free bytes including headers and footers
    int remainingMemory = 0;
    // number of all free bytes excluding headers and footers
    int realRemainingMemory = 0;
    int freeBlocksCount = 0;

    int firstFreeBlock = getIntFromRegion(FIRST_FREE_BLOCK_LOCATION);
    // if there are remaining free blocks
    if (firstFreeBlock != EMPTY) {
        int freeBlockHead = firstFreeBlock;
        int freeBlockSize;
        // count all the memory
        while (freeBlockHead != EMPTY) {
            freeBlocksCount++;
            freeBlockSize = getIntFromRegion(freeBlockHead);
            remainingMemory+= freeBlockSize;
            realRemainingMemory+= freeBlockSize - SIZE_OF_HEAD_WITH_TAIL;
            freeBlockHead = getHeadOfNextFreeBlock(freeBlockHead);
        }
    }
    int expectedSize = SIZE - (allocatedBlocksCount - freeBlocksCount) * BLOCK_SIZE;
    float fragmentation = ((float) remainingMemory / (float) expectedSize) * 100;

    printf("Starting memory size was %d.\n", SIZE);
    printf("Remaining memory after allocating %d and freeing %d blocks of size %d was %d bytes.\n",
           allocatedBlocksCount, freeBlocksCount, BLOCK_SIZE, remainingMemory);
    printf("Expected free memory size would be %d bytes\n", expectedSize);
    printf("Fragmentation was %lf %%. \n\n", fragmentation);
}

int main(void) {
    // first scenario
    test(100);

    // second scenario
    testRandom(50, 8, 24);
    testRandom(100, 8, 24);
    testRandom(200, 8, 24);

    // third scenario
    testRandom(10000, 500, 5000);

    // fourth scenario
    testRandom(100000, 8, 50000);

    return 0;
}