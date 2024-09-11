#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <direct.h>
#include <stdbool.h>

typedef struct _entries {
    char valid_bit;             // valid bit that represents cache is available
    long long tag;              // tag that represents tag bits as an integer
} entries_t;

typedef struct _cache {
    entries_t **entries;        // dynamically allocated 2D array for entries_t structure
    int **entries_order;        // dynamically allocated 2D array to treat replacement policy
    int cache_size;             // size of the cache
    int block_size;             // size of the block
    int block_num;              // number of the block
    int set_num;                // number of the sets
    int way_num;                // numbr of the ways
    char *replacement_policy;   // replacement policy
} cache_t;

typedef struct _result {
    int access;                 // number of accesses
    int read_hit;               // number of read hit
    int read_miss;              // number of read miss
    int write_hit;              // number of write hit
    int write_miss;             // number of write miss
} result_t;

typedef struct instruction_ {
    char command[5];            // command part of instruction
    long long address;          // address part of instruction
} instruction_t;


int logarithm_base(int base, int num) {
    /*
     Description
        This function calculate logarithm with the base.
        
     Arguments
        int base : This variable is the base of the loagarithm.
        int num : This variable is used for logarithm calculation.
     Returns
        This function returns the result of the logarithm caculation.
     */

    int result;
    result = log(num) / log(base);

    return result;
}


void parsing_line(instruction_t *instruction, char *line) {
    /*
     Description
        This function parses a line of the instruction into the command part and address part.
        
     Arguments
        instruction_t *instruction : This structure variable contains the command part and address part.
        char *line                 : This string variable is a line of the instruction.

     Returns
       This function does not return a value; instead, it stores the parsed data in the structure.
     */

    int i, j;
    bool flag_part;         // a flag to seperate the part of the instruction
    bool flag_element;      // a flag to extract each part of the instruction   
    char *ptr;              // a char type pointer to use strtoll function
    char command_tmp[5];    // a temporary array for extracting command part of instruction
    char address_tmp[15];   // a temporary array for extracting address part of instruction
    
    flag_part    = true;
    flag_element = false;
    j            = 0;

    for (i = 0; i < strlen(line); i++) {
        if (flag_part == true) {    // when the flag_part is true, extract the command part from the line of the instruction
            if (line[i] == ' ') {
                if (flag_element == true) {
                    command_tmp[j] = '\0';
                    strncpy(instruction ->command, command_tmp, sizeof(command_tmp));
                    j = 0;
                    flag_part = false;                    
                    flag_element = false;
                }   
            } else {            
                command_tmp[j] = line[i];
                j++;
                flag_element = true;
            }
        } else {    // when the flag_part is false, extract the address part from the instruction
            if (line[i] == ' ' || line[i] == '\n') {
                if (flag_element == true) {
                    address_tmp[j] = '\0';
                    instruction ->address = strtoll(address_tmp, &ptr, 16);  // fscanf
                }
            } else {
                address_tmp[j] = line[i];
                j++;
                flag_element = true;
            }
        }
    }
}

int miss_handler(cache_t *cache_ptr, int index, long long tag) {
    /*
     Description
        This function handles cache misses when they occur.
        The operation of this function is as follows.
            1. trys to find an empty cache space.
            2. If there is an empty cache space, pushes the data into it and update the order of the cache.
            3. Otherwise, replaces the data in the cache and updates the order of the cache.

     Arguments
        cache_t *cache_ptr : This structure variable serves as the cache.
        int index          : This variable is used to locate a specific set in the cache. 
        long long tag      : This variable is used to compate the data.

     Returns
       return 1 : If finds an empty cache space, returns 1.
       return 0 : Otherwise, returns 0.
     */

    int i, j, tmp;

    // tries to find an empty cache
    for (i = 0; i < cache_ptr -> way_num; i++) {
        if (cache_ptr -> entries[index][i].valid_bit == 0) {    // if finds an empty cache
            cache_ptr -> entries[index][i].valid_bit = 1;       // then, sets the valid bit to 1
            cache_ptr -> entries[index][i].tag = tag;           // and sets the tag

            // if the relacement policy is LRU or FIFO, updates the order of the entries
            if(!strcmp(cache_ptr -> replacement_policy, "LRU") || !strcmp(cache_ptr -> replacement_policy, "FIFO")) {
                for (j = 0; j < cache_ptr -> way_num - 1; j++) {
                    cache_ptr -> entries_order[index][cache_ptr -> way_num - 1 - j] = cache_ptr -> entries_order[index][cache_ptr -> way_num - 2 - j];                  
                }
                cache_ptr -> entries_order[index][0] = i;          
            }

            return 1;
        }
    }

    // Exiting the for loop above indicates a failure to find an empty cache space
    // if the replacement policy is LRU or FIFO, changes the order of the entries and replaces the cache
    if (!strcmp(cache_ptr -> replacement_policy, "LRU") || !strcmp(cache_ptr -> replacement_policy, "FIFO")) {
        // changes the order of the entries
        tmp = cache_ptr -> entries_order[index][cache_ptr -> way_num - 1];

        for (i = 0; i < cache_ptr -> way_num - 1; i++) {
            cache_ptr -> entries_order[index][cache_ptr -> way_num - 1 - i] = cache_ptr -> entries_order[index][cache_ptr -> way_num - 2 - i];
        }
        cache_ptr -> entries_order[index][0] = tmp;

        // replaces the cache
        cache_ptr -> entries[index][tmp].valid_bit = 1;
        cache_ptr -> entries[index][tmp].tag       = tag;

    // if the replacement policy is RANDOM, only replaces the cache
    } else {       
        cache_ptr -> entries[index][rand() % cache_ptr -> way_num].valid_bit = 1;
        cache_ptr -> entries[index][rand() % cache_ptr -> way_num].tag = tag;
    }

    return 0;
}


int cache_access(cache_t *cache_ptr, const instruction_t *instruction, result_t *result, int index_bit_num) {
    /*
     Description
        This function accesses the cache.
        The operation of this function is as follows.
            1. attempts to locate data with the valid bits and tag.
            2. If the corresponding data is found, the hit rate increases, and the order of the entries is updated.
            3. Otherwise, the miss rate increases, and calls the miss_hanlder function.

     Arguments
        cache_t *cache_ptr               : This structure variable serves as the cache.
        const instruction_t *instruction : This structure contains the command and the address. 
        result_t *result                 : This structure variable contains some variables that are used to record the result of this simulator.
        int index_bit_num                : This variable is used to extract the index and tag.

     Returns
       return 1 : If hits the cache, return 1.
       return 0 : If misses the cache, return 0.
     */

    int i, j, k;
    int tmp;
    int mask;       // bitmask that extracts the index and tag field from the address part
    int index;      // index as an integer
    long long tag;  // tag as ann integer
    char offset;    // offset variable that includes both byte offset and bit offset

    offset = logarithm_base(2, cache_ptr -> block_size) + 3;     // calculatates the offet bits
    mask = (1 << index_bit_num) - 1;
    index = (instruction -> address >> offset) & mask;           // extracts the index bits using bitmask and makes it an integer
    tag = (instruction -> address >> offset) >> index_bit_num;   // extracts the tag bits and makes it an integer

    result -> access++;                                          // whenever accesses the cache, increases the access number by 1

    // tries to access the cache with the address
    for (i = 0; i < cache_ptr -> way_num; i++) {
        if (
            cache_ptr -> entries[index][cache_ptr -> entries_order[index][i]].valid_bit == 1 &&
            cache_ptr -> entries[index][cache_ptr -> entries_order[index][i]].tag == tag
        ) {
            if (!strcmp(instruction -> command , "R")) {         // if the command is reading
                result -> read_hit++;                            // increases the read hit by 1
            } else {                                             // if the command is writing               
                result -> write_hit++;                           // increases the write hit by 1
            }               
             
            // if the replacement cache is LRU, update the order of the entries
            tmp = cache_ptr -> entries_order[index][i];
            for (j = 0; j < i; j++) {
                cache_ptr -> entries_order[index][i - j] = cache_ptr -> entries_order[index][i - 1 - j];
            }
            cache_ptr -> entries_order[index][0] = tmp;

            return 1;
        }
    }

    // Exiting the for loop above indicates a failure to find the corresponding cache space, which means cache miss
    if (!strcmp(instruction -> command, "R")) {                  // if the command is reading
        result -> read_miss++;                                   // increases read miss by 1
    } else {                                                     // if the command is writing
        result -> write_miss++;                                  // increases write miss by 1
    }
    
    miss_handler(cache_ptr, index, tag);                         // calls the miss_handler function to treat the cache miss

    return 0;
}

int cache_simulator(FILE *instruction_file, char *result_file_path, int cache_size, int block_size, int associativity, char *replacement_policy) {
    /*
     Description
        This function simulates the cache.
        The operation of this function is as follows.
            1. creates some structures including the cache.
            2. Calculates certain elements required for cache access.
            3. accesses the cache while reading the instruction line by line.
            4. records the result of the simulation.

     Arguments
        FILE *instruction_file   : This file pointer points the instruction file.
        char *result_file_path   : This variable represents the path of the result file
        int cache_size           : This variable represents the size of the cache.
        int block_size           : This variable represents the size of the block.
        int associativity        : This variable repsrents the associativity of the cache.
        char *replacement_policy : This variable represents the replacement policy of the cache.
     Returns
       return 1 : When this function exits successfully, returns 1.
     */

    int i, j;
    int line_flag;                                          // a line flag for debuging          
    int block_num;                                          // number of the blocks
    int index_bit_num;                                      // number of the bits to represent indices                          
    char line[30];                                          // a line of instruction
    char file_path[200];                                    // path of the file
    cache_t cache;                                          // creates a cache structure to store the information related to the cache
    result_t result = {0, 0, 0, 0, 0};                      // creates a result structure to store the result of the simulator
    instruction_t instruction;
    FILE *result_file;                                      // a file to record the result of the simulator

    block_num                = cache_size / block_size;     // calculates the number of the blocks
    cache.set_num            = block_num / associativity;   // calculates the numver of sets
    cache.way_num            = associativity;               // calculates the number of ways
    cache.replacement_policy = replacement_policy;          // initializes replacement policy
    cache.block_size         = block_size;

    cache.entries = (entries_t **)malloc(sizeof(entries_t*) * cache.set_num);       // dynamically allocate an 1D array of pointer of entreis_t
    cache.entries_order = (int **)malloc(sizeof(int *) * cache.set_num);            // dynamically allocate an 1D array of pointer of integer

    for (i = 0; i < cache.set_num; i++) {                                           
        cache.entries[i] = (entries_t *)malloc(sizeof(entries_t) * cache.way_num);  // dynamically allocate 1D arraies of entreis_t
        cache.entries_order[i] = (int *)malloc(sizeof(int) * cache.way_num);        // dynamically allocate 1D arraies of integer
    }

    // initializes some variables
    for (i = 0; i < cache.set_num; i++) {
        for (j = 0; j < cache.way_num; j++) {
            cache.entries[i][j].valid_bit = 0;  // initializes the valid bits to 0
            cache.entries[i][j].tag = 0;        // initializes the tag to 0
            cache.entries_order[i][j] = 0;      // initializes the order of the entries to 0
        }
    }

    // calculates number of bits for index
    if (associativity == block_num) {
        index_bit_num = 0;
    } else {
        index_bit_num = logarithm_base(2, cache.set_num);    
    }

    // accesses the cache 
    while (fgets(line, sizeof(line), instruction_file) != NULL) {
        parsing_line(&instruction, line);
        cache_access(&cache, &instruction, &result, index_bit_num);
    }

    fseek(instruction_file, 0, 0);  // rewind the file pointer of the instruction file to the beginning

    // file name format
    sprintf(file_path, "%s\\cs_%d_bs_%d_as_%d_re_%s.txt", result_file_path ,cache_size, block_size, associativity, replacement_policy);
    result_file = fopen(file_path, "wt");

    // records the result of the simulation
    fprintf(result_file, "%s,%d\n", "cache size", cache_size);
    fprintf(result_file, "%s,%d\n", "block size", block_size);
    fprintf(result_file, "%s,%d\n", "associativity", associativity);
    fprintf(result_file, "%s,%s\n", "replacement policy", replacement_policy);
    fprintf(result_file, "%s,%d\n", "number of access", result.access);
    fprintf(result_file, "%s,%d\n", "number of read hits", result.read_hit);
    fprintf(result_file, "%s,%d\n", "number of write hit", result.write_hit);
    fprintf(result_file, "%s,%d\n", "number of total hits", (result.read_hit + result.write_hit));
    fprintf(result_file, "%s,%d\n", "number of read misses", result.read_miss);
    fprintf(result_file, "%s,%d\n", "number of write misses", result.write_miss);
    fprintf(result_file, "%s,%d\n", "number of total misses", (result.read_miss + result.write_miss));

    fclose(result_file);


    // free all the memories allocated earlier
    for (i = 0; i < cache.set_num; i++) {
        free(cache.entries[i]);   
        free(cache.entries_order[i]);                                                          
    }
    free(cache.entries);
    free(cache.entries_order);

    return 1;
}



int main(int argc, char *argv[]) {
    int i;

    char *independent_element;      // a variable that can be controlled independently                
    int cache_size;                 // cache size
    int block_size;                 // block size
    int associativity;              // associativity of the cache
    char *replacement_policy;       // replacement policy of the cache
    
    FILE *instruction_file;         // file pointer that points instruction file
    char *instruction_file_path;    // path of the instruction file
    char *result_file_path;         // path of the result file
    

    independent_element   = argv[1];
    instruction_file_path = argv[2];
    result_file_path      = argv[3];
    instruction_file      = fopen(instruction_file_path, "rt");

    if (instruction_file == NULL) {
        printf("fail to open the file");
    }
    
    // checks what the independent variable is
    if (!strcmp(independent_element, "cache_size")) {
        // when independent variable is cache size
        int cache_size_min; // minimum size of the cache
        int cache_size_max; // mazimum size of the cache

        cache_size_min     = atoi(argv[4]);
        cache_size_max     = atoi(argv[5]);
        block_size         = atoi(argv[6]);
        associativity      = atoi(argv[7]);
        replacement_policy = argv[8];

        for (i = cache_size_min; i <= cache_size_max; i *= 2) {
            cache_simulator(instruction_file, result_file_path ,i, block_size, associativity, replacement_policy);
        }

    } else if (!strcmp(independent_element, "block_size")) {
        // when independent variable is block size
        int block_size_min; // minimum size of the block
        int block_size_max; // maximum size of the block

        cache_size         = atoi(argv[4]);
        block_size_min     = atoi(argv[5]);
        block_size_max     = atoi(argv[6]);
        associativity      = atoi(argv[7]);
        replacement_policy = argv[8];

        for (i = block_size_min; i <= block_size_max; i *= 2) {
            cache_simulator(instruction_file, result_file_path ,cache_size, i, associativity, replacement_policy);
        }
    } else if (!strcmp(independent_element, "associativity")) {
        // when independent variable is associativity of the cache
        int associativity_min;  // minimum associativity of the cache
        int associativity_max;  // maximum associativity of the cache

        cache_size         = atoi(argv[4]);
        block_size         = atoi(argv[5]);
        associativity_min  = atoi(argv[6]);
        associativity_max  = atoi(argv[7]);
        replacement_policy = argv[8];

        for (i = associativity_min; i <= associativity_max; i *= 2) {
            cache_simulator(instruction_file, result_file_path ,cache_size, block_size, i, replacement_policy);
        }
    }

    return 0;
}