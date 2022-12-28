#include <stdbool.h>

typedef enum
{
    LOCAL,
    GLOBAL
} Frame_Policy;

typedef struct page
{
    int pid;
    int vpn;
    int pfn;
    bool referenced;
    struct page *next;
} Page;

typedef struct replacement_list
{
    Page *front;
    Page *clk_ptr;
} Repl_List;

typedef struct page_list
{
    Repl_List *l;
    Frame_Policy frame_policy;
    int n_procs;     // number of processes
    int frame_count; // frame count
} Page_List;

Page_List *create_page_list(char *frame_policy, int n_procs);
Page *init_page(int pid, int vpn, int pfn);
int page_FIFO_enqueue(Page_List *list, Page *page);
Page *page_FIFO_dequeue(Page_List *list, int pid);
int page_clock_enqueue(Page_List *list, Page *page);
Page *swap_page(Page_List *list, Page *page);
int reference_page(Page_List *list, int pid, int vpn);
int get_list_index(Frame_Policy frame_policy, int pid);