#include "tlb_list.h"
#include "frame_list.h"

typedef enum
{
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T
} PID;

typedef struct page_table_entry
{
    int pfn_dbi;
    int present;
} PTE;

int tlb_lookup(int pid, int vpn);
void tlb_update(int vpn, int pfn);
int page_lookup(int pid, int vpn);
Page *page_update(int pid, int vpn);
void tlb_flush();
int next_free_disk();
void load_sys_config(void);
void load_new_reference(int *pid, int *vpn, char *line);