#include "MemManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define TLB_SIZE 32
#define N_MAX_FRAMES 1024
#define DISK_SIZE 1024
#define LINE_SIZE 64
#define TLB_WORD_SIZE 7
#define PAGE_WORD_SIZE 6
#define FRAME_WORD_SIZE 7
#define TLB_LOOKUP_TIME 20.0
#define MEM_CYCLE_TIME 100.0

#define FAIL_IF(EXP, MSG)              \
    {                                  \
        if (EXP)                       \
        {                              \
            fprintf(stderr, MSG "\n"); \
            exit(EXIT_FAILURE);        \
        }                              \
    }

char tlb_policy[TLB_WORD_SIZE];
char page_policy[PAGE_WORD_SIZE];
char frame_policy[FRAME_WORD_SIZE];
int n_procs, n_pages, n_frames;

PTE **page_table;
TLB *tlb;
Page_List *page_list;
bool *free_disk;
float *tlb_lookup_count;

int main(void)
{
    // load system config file
    load_sys_config();

    // initialize data structures
    // initialize page table
    FAIL_IF(!(page_table = malloc(n_procs * sizeof(PTE *))), "Malloc failure!");
    for (int i = 0; i < n_procs; ++i)
    {
        FAIL_IF(!(page_table[i] = malloc(n_pages * sizeof(PTE))), "Malloc failure!");
        for (int j = 0; j < n_pages; ++j)
        {
            page_table[i][j].pfn_dbi = -1;
            page_table[i][j].present = 0;
        }
    }
    // initialize TLB
    tlb = create_tlb(); // empty
    // record process ID
    int pid = -1, prev_pid = -1;
    // initialize page list
    page_list = create_page_list(frame_policy, n_procs);
    // initialize free disk array
    FAIL_IF(!(free_disk = malloc(DISK_SIZE * sizeof(bool))), "Malloc failure!");
    for (int i = 0; i < DISK_SIZE; ++i)
    {
        free_disk[i] = true;
    }
    // initialize analysis variables
    float reference_count[n_procs];
    for (int i = 0; i < n_procs; ++i)
    {
        reference_count[i] = 0;
    }
    float tlb_hit_count[n_procs];
    for (int i = 0; i < n_procs; ++i)
    {
        tlb_hit_count[i] = 0;
    }
    FAIL_IF(!(tlb_lookup_count = malloc(n_procs * sizeof(float))), "Malloc failure!");
    for (int i = 0; i < n_procs; ++i)
    {
        tlb_lookup_count[i] = 0;
    }
    float fault_count[n_procs];
    for (int i = 0; i < n_procs; ++i)
    {
        fault_count[i] = 0;
    }
    float fault_ratio[n_procs];
    for (int i = 0; i < n_procs; ++i)
    {
        fault_ratio[i] = 0;
    }
    float hit_ratio[n_procs];
    for (int i = 0; i < n_procs; ++i)
    {
        hit_ratio[i] = 0;
    }
    float eat[n_procs]; // effective access time
    for (int i = 0; i < n_procs; ++i)
    {
        eat[i] = 0;
    }

    FILE *file, *out, *analysis;

    // load input trace file
    char *line;
    file = fopen("trace.txt", "r");
    FAIL_IF(!file, "Failed to open file.");
    FAIL_IF(!(line = malloc(LINE_SIZE)), "Malloc failure!");
    memset(line, '\0', LINE_SIZE);

    // open trace output file
    out = fopen("trace_output.txt", "w");
    FAIL_IF(!out, "Failed to open output file.");

    // start simulation
    while (fgets(line, LINE_SIZE, file))
    {
        int vpn = -1, pfn = -1;
        prev_pid = pid;
        load_new_reference(&pid, &vpn, line);
        reference_count[pid]++;

        // check TLB
        if (prev_pid != pid)
            tlb_flush();

        if ((pfn = tlb_lookup(pid, vpn)) != -1)
        {
            fprintf(out, "Process %c, TLB Hit, %d=>%d\n", pid + 'A', vpn, pfn);
            tlb_hit_count[pid]++;
            continue;
        }
        else
        {
            fprintf(out, "Process %c, TLB Miss, ", pid + 'A');
        }

        // check page table
        if ((pfn = page_lookup(pid, vpn)) != -1)
        {
            fprintf(out, "Page Hit, %d=>%d\n", vpn, pfn);
            tlb_update(vpn, pfn);
        }
        else // page fault
        {
            fault_count[pid]++;

            Page *evicted;
            int page_in_dbi = page_table[pid][vpn].pfn_dbi;
            if ((evicted = page_update(pid, vpn))) // no free frames
            {
                int evicted_dbi = page_table[evicted->pid][evicted->vpn].pfn_dbi;
                fprintf(out, "Page Fault, %d, Evict %d of Process %c to %d, %d<<%d\n",
                        evicted->pfn, evicted->vpn, evicted->pid + 'A',
                        evicted_dbi, vpn, page_in_dbi);
            }
            else
            {
                fprintf(out, "Page Fault, %d, Evict %d of Process %c to %d, %d<<%d\n",
                        page_table[pid][vpn].pfn_dbi, -1, pid + 'A',
                        -1, vpn, -1);
            }
            tlb_update(vpn, page_table[pid][vpn].pfn_dbi);
        }

        if ((pfn = tlb_lookup(pid, vpn)) != -1)
        {
            fprintf(out, "Process %c, TLB Hit, %d=>%d\n", pid + 'A', vpn, pfn);
            tlb_hit_count[pid]++;
        }
    }

    // write analysis file
    analysis = fopen("analysis.txt", "w");
    FAIL_IF(!analysis, "Failed to open analysis file.");

    for (int i = 0; i < n_procs; ++i)
    {
        fault_ratio[i] = fault_count[i] / reference_count[i];
        hit_ratio[i] = tlb_hit_count[i] / tlb_lookup_count[i];
        eat[i] = hit_ratio[i] * (MEM_CYCLE_TIME + TLB_LOOKUP_TIME) + (1 - hit_ratio[i]) * (2 * MEM_CYCLE_TIME + TLB_LOOKUP_TIME);

        fprintf(analysis, "Process %c, Effective Access Time = %.3f\n"
                "Process %c, Page Fault Rate: %.3f\n",
                i + 'A', eat[i], i + 'A', fault_ratio[i]);
    }

    // clean up
    fclose(file);
    fclose(out);
    fclose(analysis);
    free(line);

    return 0;
}

int tlb_lookup(int pid, int vpn)
{
    tlb_lookup_count[pid]++;
    TLBE *entry;

    if ((entry = tlb_extract(tlb, vpn)))
    {
        tlb_enqueue(tlb, entry);
        if (strcmp(page_policy, "CLOCK") == 0)
            reference_page(page_list, pid, vpn);
        return entry->pfn;
    }
    return -1;
}

void tlb_update(int vpn, int pfn)
{
    TLBE *entry = init_tlb_entry(vpn, pfn);

    if (tlb->count == TLB_SIZE) // TLB is full
    {
        if (strcmp(tlb_policy, "RANDOM") == 0)
        {
            int random_vpn = tlb_find_idx_entry(tlb, rand() % TLB_SIZE);
            tlb_extract(tlb, random_vpn);
        }
        if (strcmp(tlb_policy, "LRU") == 0)
        {
            tlb_dequeue(tlb);
        }
    }
    tlb_enqueue(tlb, entry);
}

void tlb_flush()
{
    delete_tlb(tlb);
    tlb = create_tlb();
}

int page_lookup(int pid, int vpn)
{
    if (page_table[pid][vpn].present)
    {
        if (strcmp(page_policy, "CLOCK") == 0)
        {
            reference_page(page_list, pid, vpn);
        }
        return page_table[pid][vpn].pfn_dbi;
    }
    else
        return -1;
}

Page *page_update(int pid, int vpn)
{
    Page *page = NULL, *evicted = NULL;

    if (page_list->frame_count < n_frames)
    {
        page = init_page(pid, vpn, -1);

        if (strcmp(page_policy, "FIFO") == 0)
        {
            page_FIFO_enqueue(page_list, page);
        }
        if (strcmp(page_policy, "CLOCK") == 0)
        {
            page_clock_enqueue(page_list, page);
        }
    }
    else // have to evict a victim page
    {
        if (strcmp(page_policy, "FIFO") == 0)
        {
            // printf("\nbefore deq: %d\n", page_list->frame_count);
            evicted = page_FIFO_dequeue(page_list, pid);
            page = init_page(pid, vpn, evicted->pfn);
            page_FIFO_enqueue(page_list, page);
            // printf("\nafter enq: %d\n", page_list->frame_count);
        }
        if (strcmp(page_policy, "CLOCK") == 0)
        {
            page = init_page(pid, vpn, -1);
            evicted = swap_page(page_list, page);
        }

        page_table[evicted->pid][evicted->vpn].pfn_dbi = next_free_disk();
        page_table[evicted->pid][evicted->vpn].present = 0;
        if (evicted->pid == pid)
            tlb_extract(tlb, evicted->vpn);
        if (page_table[pid][vpn].pfn_dbi != -1)
            free_disk[page_table[pid][vpn].pfn_dbi] = true; // mark dbi free
    }

    page_table[pid][vpn].pfn_dbi = page->pfn;
    page_table[pid][vpn].present = 1;

    return evicted;
}

int next_free_disk()
{
    int i = 0;
    while (!free_disk[i++])
        ;
    free_disk[i - 1] = false;
    return i - 1;
}

void load_new_reference(int *pid, int *vpn, char *line)
{
    char *pch;

    pch = strtok(line, "(");
    *pid = strtok(NULL, ",")[0] - 'A';
    pch = strtok(NULL, ")");
    *vpn = atoi(pch + 1);
    // printf("%d,%d\n", *pid, *vpn);

    memset(line, '\0', LINE_SIZE);
}

void load_sys_config(void)
{
    FILE *file;
    char *line, *pch;

    file = fopen("sys_config.txt", "r");
    FAIL_IF(!file, "Failed to open file.");
    FAIL_IF(!(line = malloc(LINE_SIZE)), "Malloc failure!");

    for (int i = 0; i < 6; ++i)
    {
        memset(line, '\0', LINE_SIZE);
        line = fgets(line, LINE_SIZE, file);
        pch = strtok(line, ":");
        pch = strtok(NULL, ":");

        switch (i)
        {
        case 0:
            pch[strlen(pch) - 1] = '\0'; // strip \n
            strcpy(tlb_policy, pch + 1); // avoid ' ' at position 0
            break;
        case 1:
            pch[strlen(pch) - 1] = '\0';
            strcpy(page_policy, pch + 1);
            break;
        case 2:
            pch[strlen(pch) - 1] = '\0';
            strcpy(frame_policy, pch + 1);
            break;
        case 3:
            pch[strlen(pch) - 1] = '\0';
            n_procs = atoi(pch + 1);
            break;
        case 4:
            pch[strlen(pch) - 1] = '\0';
            n_pages = atoi(pch + 1);
            break;
        case 5:
            n_frames = atoi(pch + 1);
            break;
        default:
            ;
        }
    }

    fclose(file);
    free(line);
}
