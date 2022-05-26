#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void *head;

void memory_innit(void *ptr, unsigned int size) {
    head = ptr;
    memset(head, 0, size);
    size = size - 4 - 8;    // max pouzitelna velkost je minus root minus hlava s patou
    *(int*) (head) = 8;    //ukazovatel na najlbizssi volny blok
    *(int*) (head + 4) = (int) - size;   //zaciatok pamate - nebude sa menit, bude hovorit o velkosti pola (mensia kontrola)
    *(int*) (head + 8) = (int) size - 8;     //hlava prveho pouzitelneho bloku pamate
    *(int*) (head + size + 4) = (int) size - 8;    //pate prveho pouzitelneho bloku pamate
    *(int*) (head + size + 8) = (int) - size;   // zapise patu celej pamate
}

void* memory_alloc( unsigned int size ) {
    int mem_size = - *(int*) (head + 4);
    if (*(int*) head == 0) {
        //printf("Nie je volna pamat");
        return NULL;
    }
    else
    {


        if (size % 2 == 1) // zaokruhlovanie na nasobok dvojky
        {
            size++;
        }

        int head_with_tail = 8, size_of_free_block = *(int *) (head + *(int *) head), head_of_free_block;

        if (*(int *) (head + 4) == -mem_size)      // konrola hlavy pamate ak sa rovna velkosti celej pouzitelnej pamati
        {
            int pointer_to_free_block = *(int *) head, offset_to_next_block_of_next_block = 0; // ulozenie offsetu na prvy volny blok
            int best_fit = 0, best_size_fit =
                    mem_size; // nastavenie na najhorsi mozny fit ktory sa potom upravi na najlepsi

            while (1) // logika best fit
            {
                if (size_of_free_block >= size) // ak je blok dostatocne volny
                {
                    if (size_of_free_block <= best_size_fit) // ak je vhodnejsi
                    {
                        best_fit = pointer_to_free_block; // ulozenie offsetu na najvhodnejsi blok
                        best_size_fit = size_of_free_block; // ulozenie velkosti najvhodnejsieho bloku
                        if (best_size_fit == size) // ak sa nahodou najde presny blok tak sa algoritmus ukonci
                        {
                            break;
                        }
                    }
                }
                offset_to_next_block_of_next_block = *(int *) (head + pointer_to_free_block + sizeof(int));
                if (offset_to_next_block_of_next_block == 0) // ak dalsi blok nema ukzazovatel tak sa algoritmus ukonci
                {
                    break;
                }
                pointer_to_free_block = offset_to_next_block_of_next_block;
            }

            head_of_free_block = best_fit;

            // skontrolovat prepisovanie pointeru

            if (size_of_free_block >= size)     // kontrola ci je volny blok dostatocne velky
            {
                //ulozenie pointerov na dalsi a predosly blok
                int pointer_to_next = *((int *) (head + head_of_free_block + sizeof(int)));
                int pointer_to_prev = *((int *) (head + head_of_free_block + 2 * sizeof(int)));

                size_of_free_block = size_of_free_block - (int) size - head_with_tail;

                if (size_of_free_block < 8 && size_of_free_block != -8)       // podmienka na boundovanie bloku ak by bol volny blok prilis maly
                {
                    size += size_of_free_block + head_with_tail;
                    size_of_free_block = 0;
                }

                if (size_of_free_block >=  8)  // ak je blok vacssi alebo rovny ako minimalna velkost tak sa prepocita zvyskova velkost
                {
                    *(int *) (head + pointer_to_free_block + size + head_with_tail) = size_of_free_block; // nacitanie zmensenej velkosti po alokacii
                    *(int *) (head + pointer_to_free_block + (size + head_with_tail) + size_of_free_block + 4) = size_of_free_block; // nacitanie zvysnej hodnoty na koniec volneho bloku
                }
                *(int *) (head + pointer_to_free_block) = (int) -size;   // hlavicka
                *(int *) (head + pointer_to_free_block + size + 4) = (int) -size;    // pata
                memset(head + pointer_to_free_block + 4, 0, size); // precistenie vnutorneho miesta bloku


                // logika pointerov
                if (size_of_free_block >= 8) // ak zotalo dostatok miesta tak sa len posunie hlavicka a pointery
                {
                    void *head_of_remaining_block = (head + pointer_to_free_block + size + 2 * sizeof(int));
                    *(int *) head += (int) size + head_with_tail;
                    *(int *) (head_of_remaining_block + sizeof(int)) = pointer_to_next;
                    *(int *) (head_of_remaining_block + 2 * sizeof(int)) = pointer_to_prev;
                } else if (size_of_free_block < 8) // ak sa pouzil cely volny blok
                {
                    if (pointer_to_next != 0)  // ak nebude nic v pointeri tak sa nebude nic zapisovat
                    {
                        *(int *) (head + pointer_to_next + 2 *
                                                           sizeof(int)) = pointer_to_prev;   // ulozi sa offset na predosly blok do dalsieho bloku

                    }
                    if (pointer_to_prev != 0)  // ak nebude nic v pointeri tak sa nebude nic zapisovat
                    {
                        *(int *) (head + pointer_to_prev +
                                  sizeof(int)) = pointer_to_next;   // ulozi sa offset na dalsi blok do predosleho bloku
                    }
                    if (head_of_free_block == *(int *) head) {
                        *(int *) head = pointer_to_prev;
                    }
                }

                return (void *) (head + pointer_to_free_block + 4);     // vrati pointer na data alkokovaneho miesta
            } else {
                //printf("Nie je dostatok pamate.\n");
                return NULL;
            }

        } else {
            //printf("Globalny pointer je chybny\n");
            return NULL;
        }
    }
}




int memory_free(void *valid_pointer)    // uvolnovanie hotove uz len logika pointerov
{
    int mem_size = *(int*) (head + 4);
    void *head_of_block = (valid_pointer - 4); //head_of_block => adresa hlavicky
    int head_with_tail = 8, size_of_block = -(*(int *) head_of_block), size_of_new_block = 0;
    int offset_to_next = 0, first_free_block = *(int *) head;


    if (*(int *) (head + 4) == mem_size) // kontrola hlavy pamate
    {
        if (*(int *) (head_of_block - 4) > 0)        //ak je pata predosleho bloku nealokovana - cize viac ako nula
        {
            //int index = *(int*) (head_of_block - *(int*) ());
            int size_of_prev = *(int *) (head_of_block - 4);    // ulozi sa velkost predosleho volneho bloku
            void *head_of_prev = (head_of_block - size_of_prev - head_with_tail);      // ukazovatel na hlavu predosleho volneho bloku

            //ulozenie offsetov dalsieho bloku + first_free_block
            int offset_to_next_of_next_block = *(int *) (head_of_prev + sizeof(int));
            int offset_to_prev_of_next_block = *(int *) (head_of_prev + 2 * sizeof(int));
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
                    *(int *) (head + offset_to_prev_of_next_block + sizeof(int)) = offset_to_next_of_next_block;
                    *(int *) (head + offset_to_next_of_next_block + 2 * sizeof(int)) = offset_to_prev_of_next_block;
                    *(int *) (head + offset_to_prev_of_next_block + 2 * sizeof(int)) = (int) (head_of_block - head);
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
            int offset_to_next_of_prev_block = *(int *) (head_of_next + sizeof(int));
            int offset_to_prev_of_prev_block = *(int *) (head_of_next + 2 * sizeof(int));
            first_free_block = *(int *) head;


            size_of_new_block = size_of_block + size_of_next + head_with_tail;      // vypocita velkost novo vzniknuteho bloku
            *(int *) head_of_block = size_of_new_block;      // zapise do hlavy novu velkost
            memset(valid_pointer, 0, size_of_new_block);    //premaze pamat volneho bloku
            *(int *) (head_of_block + size_of_new_block + 4) = size_of_new_block;   // zapise velkost paty novej pamate



            //presmerovanie pointerov predosleho bloku
            if (*(int *) head < -mem_size) // kontrola hlavneho offsetu ci neukazuje mimo
            {
                if (offset_to_next_of_prev_block != 0 && offset_to_prev_of_prev_block != 0) {
                    *(int *) (head + offset_to_prev_of_prev_block + sizeof(int)) = offset_to_next_of_prev_block;

                    *(int *) (head + offset_to_next_of_prev_block + 2 * sizeof(int)) = offset_to_prev_of_prev_block;
                }
                if (offset_to_prev_of_prev_block != 0) {
                    *(int *) (head + offset_to_prev_of_prev_block + 2 * sizeof(int)) = (int) (head_of_block - head);
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
    int size = *(int *) (ptr - sizeof(int));
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





void test_random (unsigned  int size, int min, int max )
{
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
        block_array[i] = memory_alloc(random);
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

    void *all_block = memory_alloc(8);
    void *all_block_2 = memory_alloc(8);
    void *all_block_3 = memory_alloc(8);
    void *all_block_4 = memory_alloc(8);
    void *all_block_5 = memory_alloc(8);

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
        remaining_memory = size - 3 * sizeof(int) - allocated_blocks * (8 + 2 * sizeof(int));
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
    //test(150);


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