#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <string.h>
#include <direct.h>

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
    int access;                // number of accesses
    int read_hit;              // number of read hit
    int read_miss;             // number of read miss
    int write_hit;             // number of write hit
    int write_miss;            // number of write miss
} result_t;

typedef struct instruction_ {
    char command[5];           // command part of instruction
    long long address;         // address part of instruction
} instruction_t;

// calculate the logarithm with base
int logarithm_base(int base, int num) {
    return log(num) / log(base);
}

// parse a line of the instruction into the command part and the address part
void parsing_line(instruction_t *instruction, char *line) {
    int i, j;
    boolean flag_part;      // a flag to seperate the part of the instruction
    boolean flag_element;   // a flag to extract each part of the instruction   
    char *ptr;              // a char type pointer to use strtoll function
    char command_tmp[5];    // a temporary array for extracting command part of instruction
    char address_tmp[15];   // a temporary array for extracting address part of instruction
    
    flag_part    = TRUE;
    flag_element = FALSE;
    j            = 0;

    for (i = 0; i < strlen(line); i++) {
        if (flag_part == TRUE) {    // when the part is TRUE, extract command part from the instruction
            if (line[i] == ' ') {
                if (flag_element == TRUE) {
                    command_tmp[j] = '\0';
                    strncpy(instruction ->command, command_tmp, sizeof(command_tmp));
                    j = 0;
                    flag_part = FALSE;                    
                    flag_element = FALSE;
                }   
            } else {            
                command_tmp[j] = line[i];
                j++;
                flag_element = TRUE;
            }
        } else {    // when the part is FALSE, extract address part from the instruction
            if (line[i] == ' ' || line[i] == '\n') {
                if (flag_element == TRUE) {
                    address_tmp[j] = '\0';
                    instruction ->address = strtoll(address_tmp, &ptr, 16);  // fscanf
                }
            } else {
                address_tmp[j] = line[i];
                j++;
                flag_element = TRUE;
            }
        }
    }
}

// handle cache miss
int miss_handler(cache_t *cache_ptr, int index, long long tag) {
    // st dbg
    // printf("now in the miss handler\n");
    //ed dbg

    /*
     Description
       ...

     Arguments
       Entry *cache: ...
       ...

     Returns
       ...
     */

    int i, j, tmp;

    // try to find an empty cache
    for (i = 0; i < cache_ptr -> way_num; i++) {
        if (cache_ptr -> entries[index][i].valid_bit == 0) {    // if find an empty cache
            cache_ptr -> entries[index][i].valid_bit = 1;       // set the valid bit to 1
            cache_ptr -> entries[index][i].tag = tag;           // set the tag

            // if the relacement policy is LRU or FIFO, update the order of the entries
            if(!strcmp(cache_ptr -> replacement_policy, "LRU") || !strcmp(cache_ptr -> replacement_policy, "FIFO")) {
                for (j = 0; j < cache_ptr -> way_num - 1; j++) {
                    cache_ptr -> entries_order[index][cache_ptr -> way_num - 1 - j] = cache_ptr -> entries_order[index][cache_ptr -> way_num - 2 - j];                  
                }
                cache_ptr -> entries_order[index][0] = i;          
            }

            return 1;
        }
    }

    // when fail to find an empty cache
    // if the replacement policy is LRU or FIFO, change the order of the entries and replace the cache
    if (!strcmp(cache_ptr -> replacement_policy, "LRU") || !strcmp(cache_ptr -> replacement_policy, "FIFO")) {
        // change the order of the entries
        tmp = cache_ptr -> entries_order[index][cache_ptr -> way_num - 1];

        for (i = 0; i < cache_ptr -> way_num - 1; i++) {
            cache_ptr -> entries_order[index][cache_ptr -> way_num - 1 - i] = cache_ptr -> entries_order[index][cache_ptr -> way_num - 2 - i];
        }
        cache_ptr -> entries_order[index][0] = tmp;

        // replace the cache
        cache_ptr -> entries[index][tmp].valid_bit = 1;
        cache_ptr -> entries[index][tmp].tag       = tag;

    // if the replacement policy is RANDOM, only replace the cache
    } else {       
        cache_ptr -> entries[index][rand() % cache_ptr -> way_num].valid_bit = 1;
        cache_ptr -> entries[index][rand() % cache_ptr -> way_num].tag = tag;
    }

    return 1;
}

// access the cache
int cache_access(cache_t *cache_ptr, const instruction_t *instruction, result_t *result, int index_bit_num) {
    int i, j, k;
    int tmp;
    int mask;       // bitmask that extracts tag and index field from the address part
    int index;      // index as an integer
    long long tag;  // tag as ann integer
    char offset;    // offset variable that includes both byte offset and bit offset

    offset = logarithm_base(2, cache_ptr -> block_size) + 3;     // calculatate the offet bits
    mask = (1 << index_bit_num) - 1;
    index = (instruction -> address >> offset) & mask;           // extract the index bits using bitmask and make it an integer
    tag = (instruction -> address >> offset) >> index_bit_num;   // extract the tag bits and make it an integer

    result -> access++;                                          // whenever access the cache, increases access number by 1

    // try to access the cache with it's address
    for (i = 0; i < cache_ptr -> way_num; i++) {
        if (
            cache_ptr -> entries[index][cache_ptr -> entries_order[index][i]].valid_bit == 1 &&
            cache_ptr -> entries[index][cache_ptr -> entries_order[index][i]].tag == tag
        ) {
            if (!strcmp(instruction -> command , "R")) {         // if the command is read
                result -> read_hit++;                            // increases read hit by 1
            } else {                                             // if the command is write                 
                result -> write_hit++;                           // increases write hit by 1
            }               
             
            // if the replacement cache is LRU, update the order of the entries
            tmp = cache_ptr -> entries_order[index][i];
            for (j = 0; j < i; j++) {
                cache_ptr -> entries_order[index][i - j] = cache_ptr -> entries_order[index][i - 1 - j];
            }
            cache_ptr -> entries_order[index][0] = tmp;

            return 1;                                            // when hit the cache, return 1
        }
    }

    // when fail to access the cache
    if (!strcmp(instruction -> command, "R")) {                  // if the command is read
        result -> read_miss++;                                   // increases read miss by 1
    } else {                                                     // if the command is write
        result -> write_miss++;                                  // increases write miss by 1
    }
    
    miss_handler(cache_ptr, index, tag);                         // call the miss_handler function to treat the cache miss

    return 0;                                                    // when miss the cache, return 0;
}

int cache_simulator(FILE *instruction_file, int cache_size, int block_size, int associativity, char *replacement_policy) {

    // st dbg
    // printf("in the cache simulator\n");
    // ed dbg

    int i, j;
    int line_flag;                                          // a line flag for debuging          
    int block_num;                                          // number of the blocks
    int index_bit_num;                                      // number of the bits to represent indices                          
    char line[30];                                          // a line of instruction
    char file_path[200];                                    // path of the file
    cache_t cache;                                          // create a cache structure to store the information related to the cache
    result_t result = {0, 0, 0, 0, 0};                      // create a result structure to store the result of the simulator
    instruction_t instruction;
    FILE *result_file;                                      // a file to record the result of the simulator

    block_num                = cache_size / block_size;     // calculate the number of the blocks
    cache.set_num            = block_num / associativity;   // calculate the numver of sets
    cache.way_num            = associativity;               // calculate the number of ways
    cache.replacement_policy = replacement_policy;          // initialize replacement policy
    cache.block_size         = block_size;

    cache.entries = (entries_t **)malloc(sizeof(entries_t*) * cache.set_num);       // dynamically allocate an 1D array of pointer of entreis_t
    cache.entries_order = (int **)malloc(sizeof(int *) * cache.set_num);              // dynamically allocate an 1D array of pointer of integer

    for (i = 0; i < cache.set_num; i++) {                                           
        cache.entries[i] = (entries_t *)malloc(sizeof(entries_t) * cache.way_num);  // dynamically allocate 1D arraies of entreis_t
        cache.entries_order[i] = (int *)malloc(sizeof(int) * cache.way_num);          // dynamically allocate 1D arraies of integer
    }

    // initialize some variables
    for (i = 0; i < cache.set_num; i++) {
        for (j = 0; j < cache.way_num; j++) {
            cache.entries[i][j].valid_bit = 0;  // initialize the valid bits to 0
            cache.entries[i][j].tag = 0;        // initialize the tag to 0
            cache.entries_order[i][j] = 0;        // initialize the order of the entries to 0
        }
    }

    // calculate number of bits for index
    if (associativity == block_num) {                                                                                  
        index_bit_num = 0;
    } else {
        index_bit_num = logarithm_base(2, cache.set_num);    
    }

    // st dbg
    // printf("current spec\n");
    // printf("cache size : %d\n", cache_size);
    // printf("block size : %d\n", block_size);
    // printf("number of the block : %d\n", block_num);
    // printf("number of the sets : %d\n", cache.set_num);
    // printf("number of the ways : %d\n\n", cache.way_num);
    // ed sbg

    // line_flag = 1; // a flag to count the lines

    // access the cache 
    while (fgets(line, sizeof(line), instruction_file) != NULL) {
        // st dbg
        // printf("instruction %d start\n", line_flag);
        // ed dbg

        parsing_line(&instruction, line);
        cache_access(&cache, &instruction, &result, index_bit_num);
        
        // st dbg
        // printf("order of the entries for %d line of the instruction\n", line_flag);

        // for (i = 0; i < cache.set_num; i++) {
        //     for (j = 0; j < cache.way_num; j++) {
        //         printf("%d ", cache.entries_order[i][j]);
        //     }
        //     printf("\n");
        // }

        // printf("cache entries for %d line of the instruction\n", line_flag);

        // for (i = 0; i < cache.set_num; i++) {
        //     for (j = 0; j < cache.way_num; j++) {
        //         printf("%d-%lld ",cache.entries[i][j].valid_bit, cache.entries[i][j].tag);
        //     }
        //     printf("\n");
        // }

        // printf("instruction %d end\n\n", line_flag++);
        // en dbg
    }

    // st dbg
    // printf("after processing all instruction\n");
    // ed dbg

    fseek(instruction_file, 0, 0);  // rewind the file pointer of the instruction file to the beginning

    // file name format
    sprintf(file_path, "C:\\Users\\angJongMokddi\\OneDrive\\cache_simulator\\result\\cs_%d_bs_%d_as_%d_re_%s.txt", cache_size, block_size, associativity, replacement_policy);
    result_file = fopen(file_path, "wt");

    //st dbg
    // printf("create the file\n");
    // ed dbg

    // record the result of the simulation
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

    // st dbg
    // printf("write all things on the file and close it\n");
    // ed dbg

    // free all the memories allocated earlier
    for (i = 0; i < cache.set_num; i++) {
        free(cache.entries[i]);   
        free(cache.entries_order[i]);                                                          
    }
    free(cache.entries);
    free(cache.entries_order);
    
    // st dbg
    // printf("free all cache entries\n");
    // ed dbg

    return 1;
}



int main(int argc, char *argv[]) {
    char *mode;
    char *instruction_file_path;
    int cache_size;
    int block_size;
    int associativity;
    char *replacement_policy;
    FILE *instruction_file;
    int i;

    mode                  = argv[1];
    instruction_file_path = argv[2];
    instruction_file      = fopen(instruction_file_path, "rt");

    if (instruction_file == NULL) {
        printf("fail to open the file");
    }
    
    if (!strcmp(mode, "cache_size")) {
        // st dbg
        printf("cache size mode start\n");
        // ed dbg

        int cache_size_min;
        int cache_size_max;

        cache_size_min     = atoi(argv[3]);
        cache_size_max     = atoi(argv[4]);
        block_size         = atoi(argv[5]);
        associativity      = atoi(argv[6]);
        replacement_policy = argv[7];

        // st dbg
        printf("current spec\n");
        printf("cache size min : %d\n", cache_size_min);
        printf("cache_size_max : %d\n", cache_size_max);
        printf("block_size : %d\n", block_size);
        printf("assoc : %d\n", associativity);
        printf("repl : %s\n", replacement_policy);
        // ed dbg

        for (i = cache_size_min; i <= cache_size_max; i *= 2) {
            // st dbg
            // printf("cache size %d start\n", i);
            // ed dbg

            cache_simulator(instruction_file, i, block_size, associativity, replacement_policy);

            // st dbg
            // printf("cache size %d end\n", i);
            // ed dbg
        }

        // st dbg
        // printf("cache size mode end\n");
        // ed dbg

    } else if (!strcmp(mode, "block_size")) {
        int block_size_min;
        int block_size_max;

        cache_size         = atoi(argv[3]);
        block_size_min     = atoi(argv[4]);
        block_size_max     = atoi(argv[5]);
        associativity      = atoi(argv[6]);
        replacement_policy = argv[7];

        for (i = block_size_min; i <= block_size_max; i *= 2) {
            cache_simulator(instruction_file, cache_size, i, associativity, replacement_policy);
        }
    } else if (!strcmp(mode, "associativity")) {
        int associativity_min;
        int associativity_max;

        cache_size         = atoi(argv[3]);
        block_size         = atoi(argv[4]);
        associativity_min  = atoi(argv[5]);
        associativity_max  = atoi(argv[6]);
        replacement_policy = argv[7];

        for (i = associativity_min; i <= associativity_max; i *= 2) {
            cache_simulator(instruction_file, cache_size, block_size, i, replacement_policy);
        }
    }

    // st dbg
    // printf("program name : %s\n", argv[0]);
    // printf("file path : %s\n", argv[1]);
    // printf("cache size : %d\n", atoi(argv[2]));
    // printf("block size : %d\n", atoi(argv[3]));
    // printf("assoc : %d\n", atoi(argv[4]));
    // printf("repl : %s\n", argv[3]);
    // ed dbg

    return 0;
}