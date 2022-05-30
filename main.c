#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INT_SIZE (int)sizeof(int)
#define MIN_MEMORY_SIZE 24
#define MIN_BLOCK_SIZE 16
#define SIZE_OF_HEAD_WITH_TAIL 8
#define FIRST_FREE_BLOCK_LOCATION 4

void *head;
// x/50db head

int setIntIntoRegion(int byteLocation, int value) {
    return *(int*) (head + byteLocation) = value;
}

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
        if (prevFreeBlockHead == 0 && nextFreeBlockHead == 0) {
            setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, 0);
        }
            // if it is last block in chain
        else if (prevFreeBlockHead != 0 && nextFreeBlockHead == 0) {
            setIntIntoRegion(FIRST_FREE_BLOCK_LOCATION, nextFreeBlockHead);
            setPointerToPrevBlock(nextFreeBlockHead, 0);
        }
            // if it is first block in chain
        else if (prevFreeBlockHead == 0 && nextFreeBlockHead != 0) {
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
    int freeBlockHead = getIntFromRegion(4);
    if (freeBlockHead == 0) {
        return NULL;
    }
    int freeBlockSize = getIntFromRegion(freeBlockHead);

    int bestBlockHead = 0;
    int bestBlockSize = INT_MAX;
    // if free block does not have pointer (it is 0) to next block it is last block
    while (freeBlockHead != 0) {
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
    if (getIntFromRegion(FIRST_FREE_BLOCK_LOCATION) == 0) {
        //printf("Memory is full.");
        return NULL;
    }
    // rounding to even number
    int realNewBlockSize = (newBlockSize % 2 == 1 ? newBlockSize + 1: newBlockSize) + SIZE_OF_HEAD_WITH_TAIL;
    int freeBlockHead = bestFit(realNewBlockSize);

    allocateBlock(freeBlockHead, realNewBlockSize);
    return (void *) (head + freeBlockHead + 4);
}

int memory_free(void *valid_pointer)    // uvolnovanie hotove uz len logika pointerov
{
    int mem_size = *(int*) (head + 4);
    void *head_of_block = (valid_pointer - 4); //head_of_block => adresa hlavicky
    int head_with_tail = 8;
    int size_of_block = -(*(int *) head_of_block);
    int size_of_new_block = 0;
    int offset_to_next = 0, first_free_block = *(int *) head;


    if (*(int *) (head + 4) == mem_size) // kontrola hlavy pamate
    {
        if (*(int *) (head_of_block - 4) > 0)        //ak je pata predosleho bloku nealokovana - cize viac ako nula
        {
            //int index = *(int*) (head_of_block - *(int*) ());
            int size_of_prev = *(int *) (head_of_block - 4);    // ulozi sa velkost predosleho volneho bloku
            void *head_of_prev = (head_of_block - size_of_prev - head_with_tail);      // ukazovatel na hlavu predosleho volneho bloku

            //ulozenie offsetov dalsieho bloku + first_free_block
            int offset_to_next_of_next_block = *(int *) (head_of_prev + INT_SIZE);
            int offset_to_prev_of_next_block = *(int *) (head_of_prev + 2 * INT_SIZE);
            first_free_block = *(int *) head;


            *(int *) ((head_of_block - size_of_prev - head_with_tail)) = size_of_prev + size_of_block + head_with_tail; // na miesto predoslej hlavy sa napise velkost prveho bloku + velkost druheho bloku + jedna hlava s patou ktore zaniknu
            size_of_new_block = *(int *) head_of_prev; // spocita velkost noveho bloku
            memset((head_of_prev + 4), 0, size_of_new_block + 4); // prepise cely blok na nuly
            (*(int *) (head_of_prev + size_of_new_block + 4)) = size_of_new_block; // zapise patu


            head_of_block = head_of_block - size_of_prev - head_with_tail; // nova adresa zaciatku bloku
            valid_pointer = head_of_block + 4; // nova adresa valid pointera
            size_of_block = *(int *) head_of_block;
            size_of_new_block = 0;


            //presmerovanie pointerov predosleho bloku

            if (*(int *) head < -mem_size) // kontrola hlavneho offsetu ci neukazuje mimo
            {
                if (offset_to_next_of_next_block != 0 && offset_to_prev_of_next_block != 0) {
                    *(int *) (head + offset_to_prev_of_next_block + INT_SIZE) = offset_to_next_of_next_block;
                    *(int *) (head + offset_to_next_of_next_block + 2 * INT_SIZE) = offset_to_prev_of_next_block;
                    *(int *) (head + offset_to_prev_of_next_block + 2 * INT_SIZE) = (int) (head_of_block - head);
                }

                // nastavenie dalsieho a predosleho offetu + nastavenie hlavicky na uvolnovany blok
                offset_to_next = first_free_block; // ulozenie offsetu na prvy bolny blok
                *(int *) head = (int) (head_of_block - head);  // ulozenie do hlavneho offsetu cislo uvolneneho bloku
                *(int *) valid_pointer = offset_to_next;    //ulozenie offsetu na hlavu dalsieho volneho bloku do uvolneneho bloku
            }
        }
        if (*(int *) (head_of_block + size_of_block + head_with_tail) > 0)      // ak je hlava dalsieho bloku nealokovana - viac ako nula
        {
            int size_of_next = *(int *) (head_of_block + size_of_block + head_with_tail);    //zisti velkost dalsieho volneho bloku
            void *head_of_next = head_of_block + size_of_block + head_with_tail;      // ukazovatel na hlavu predosleho volneho bloku


            //ulozenie offsetov predosleho bloku + first_free_block
            int offset_to_next_of_prev_block = *(int *) (head_of_next + INT_SIZE);
            int offset_to_prev_of_prev_block = *(int *) (head_of_next + 2 * INT_SIZE);
            first_free_block = *(int *) head;


            size_of_new_block = size_of_block + size_of_next + head_with_tail;      // vypocita velkost novo vzniknuteho bloku
            *(int *) head_of_block = size_of_new_block;      // zapise do hlavy novu velkost
            memset(valid_pointer, 0, size_of_new_block);    //premaze pamat volneho bloku
            *(int *) (head_of_block + size_of_new_block + 4) = size_of_new_block;   // zapise velkost paty novej pamate



            //presmerovanie pointerov predosleho bloku
            if (*(int *) head < -mem_size) // kontrola hlavneho offsetu ci neukazuje mimo
            {
                if (offset_to_next_of_prev_block != 0 && offset_to_prev_of_prev_block != 0) {
                    *(int *) (head + offset_to_prev_of_prev_block + INT_SIZE) = offset_to_next_of_prev_block;

                    *(int *) (head + offset_to_next_of_prev_block + 2 * INT_SIZE) = offset_to_prev_of_prev_block;
                }
                if (offset_to_prev_of_prev_block != 0) {
                    *(int *) (head + offset_to_prev_of_prev_block + 2 * INT_SIZE) = (int) (head_of_block - head);
                }


                // nastavenie dalsieho a predosleho offetu + nastavenie hlavicky na uvolnovany blok
                offset_to_next = first_free_block; // ulozenie offsetu na prvy bolny blok
                *(int *) head = (int) (head_of_block - head);  // ulozenie do hlavneho offsetu cislo uvolneneho bloku
                *(int *) valid_pointer = offset_to_next;    //ulozenie offsetu na hlavu dalsieho volneho bloku do uvolneneho bloku

            }
        } else if (*(int *) head_of_block <
                   0)        // ak ostatne podmienky neboli splnene tak sa neuvolnil blok a tym padom prejde tato podmienka
        {
            *(int *) head_of_block = size_of_block;
            memset(valid_pointer, 0, size_of_block);
            *(int *) (head_of_block + size_of_block + 4) = size_of_block;

            if (*(int *) head < -mem_size)  // ak pointer neukazuje mimo
            {
                offset_to_next = *(int *) head;     //offset na dalsi volny blok
                *(int *) head = (int) (head_of_block - head);  // ulozenie do hlavneho offsetu cislo uvolneneho bloku

                if ((int) (offset_to_next) != 0) {
                    *(int *) (head + offset_to_next + 8) = (int) (head_of_block -
                                                                  head);   // ulozenie do pointer_to_prev to
                    *(int *) valid_pointer = offset_to_next;    //ulozenie offsetu na hlavu dalsieho volneho bloku do uvolneneho bloku
                }
            }
            return 0;
        }
    }
    return 1;
}




int memeory_check(void *ptr) {
    int size = *(int *) (ptr - INT_SIZE);
    if (size > 0)
    {
        if (*(int *) (ptr + size) == size) // pozrie patu, ak sa rovna, pointer je platny
        {
            return 1;
        } else
        {
            return 0;
        }
    } else
    {
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

    void *all_block = memoryAlloc(8);
    void *all_block_2 = memoryAlloc(8);
    void *all_block_3 = memoryAlloc(8);
    void *all_block_4 = memoryAlloc(8);
    void *all_block_5 = memoryAlloc(8);

    if ( all_block != NULL) { allocated_blocks ++;}
    if ( all_block_2 != NULL) { allocated_blocks ++;}
    if ( all_block_3 != NULL) { allocated_blocks ++;}
    if ( all_block_4 != NULL) { allocated_blocks ++;}
    if ( all_block_5 != NULL) { allocated_blocks ++;}

    if (memory_free(all_block_2) == 0)
    {
        allocated_blocks--;
    }


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
    test(50);


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