#include "frame_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAIL_IF(EXP, MSG)                        \
    {                                            \
        if (EXP)                                 \
        {                                        \
            fprintf(stderr, "Error! " MSG "\n"); \
            exit(EXIT_FAILURE);                  \
        }                                        \
    }

Page_List *create_page_list(char *frame_policy, int n_procs)
{
    Page_List *P;
    FAIL_IF(!(P = malloc(sizeof(Page_List))), "Page list malloc failure!");

    if (strcmp(frame_policy, "LOCAL") == 0)
        P->frame_policy = LOCAL;
    if (strcmp(frame_policy, "GLOBAL") == 0)
        P->frame_policy = GLOBAL;

    P->n_procs = n_procs;
    P->frame_count = 0;

    int list_length = (P->frame_policy == LOCAL ? n_procs : 1);
    FAIL_IF(!(P->l = malloc(sizeof(Repl_List) * list_length)), "Replacement list malloc failure!");
    for (int i = 0; i < list_length; ++i)
    {
        P->l[i].front = NULL;
        P->l[i].clk_ptr = NULL;
    }

    return P;
}

Page *init_page(int pid, int vpn, int pfn)
{
    Page *page;
    FAIL_IF(!(page = malloc(sizeof(Page))), "Page entry malloc failure!");
    page->pid = pid;
    page->vpn = vpn;
    page->pfn = pfn;
    page->referenced = true;
    page->next = NULL;

    return page;
}

int page_FIFO_enqueue(Page_List *list, Page *page)
{
    Page *p;
    int index = get_list_index(list->frame_policy, page->pid);

    for (p = list->l[index].front; p != NULL; p = p->next)
    {
        if (!p->next)
            break;
    } // p = last node in the linked list

    if (!p)
        list->l[index].front = page;
    else
        p->next = page;

    if (page->pfn == -1)
        page->pfn = list->frame_count;
    (list->frame_count)++;
    page->next = NULL;

    return 0;
}

int page_clock_enqueue(Page_List *list, Page *page)
{
    Page *p;
    int index = get_list_index(list->frame_policy, page->pid);

    for (p = list->l[index].clk_ptr; p != NULL; p = p->next)
    {
        if (p->next == list->l[index].clk_ptr)
            break;
    } // p = last node in the linked list a.k.a just before clk_ptr

    if (!p)
    {
        list->l[index].clk_ptr = page;
        page->next = page;
    }
    else
    {
        page->next = p->next;
        p->next = page;
    }

    if (page->pfn == -1)
        page->pfn = list->frame_count;
    (list->frame_count)++;

    return 0;
}

Page *page_FIFO_dequeue(Page_List *list, int pid)
{
    Page *p;
    int index = get_list_index(list->frame_policy, pid);

    if ((p = list->l[index].front))
    {
        list->l[index].front = p->next;
        p->next = NULL;
        (list->frame_count)--;
        return p;
    }
    return NULL;
}

Page *swap_page(Page_List *list, Page *page)
{
    Page *p;
    int index = get_list_index(list->frame_policy, page->pid);

    for (p = list->l[index].clk_ptr; p->referenced; p = p->next)
    {
        p->referenced = false;
        list->l[index].clk_ptr = p->next;
    } // p = first page that hasn't been referenced since the last lookup

    // save to-be evicted data
    int pid_temp = p->pid;
    int vpn_temp = p->vpn;
    // swap in new page
    p->pid = page->pid;
    p->vpn = page->vpn;
    p->referenced = true;
    // copy evicted page and return
    page->pid = pid_temp;
    page->vpn = vpn_temp;
    page->pfn = p->pfn;
    page->next = NULL;

    list->l[index].clk_ptr = p->next;
    return page; // return evicted page
}

int reference_page(Page_List *list, int pid, int vpn)
{
    Page *p, *clk;
    int index = get_list_index(list->frame_policy, pid);
    clk = list->l[index].clk_ptr;

    for (p = clk; p->next != clk; p = p->next)
    {
        if (p->pid == pid && p->vpn == vpn)
        {
            p->referenced = true;
            return p->pfn;
        }
    }
    return -1;
}

int get_list_index(Frame_Policy frame_policy, int pid)
{
    if (frame_policy == GLOBAL)
        return 0;
    if (frame_policy == LOCAL)
        return pid;
    else
        return -1;
}