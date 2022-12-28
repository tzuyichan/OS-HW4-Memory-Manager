#include "tlb_list.h"
#include <stdio.h>
#include <stdlib.h>

#define FAIL_IF(EXP, MSG)                        \
    {                                            \
        if (EXP)                                 \
        {                                        \
            fprintf(stderr, "Error! " MSG "\n"); \
            exit(EXIT_FAILURE);                  \
        }                                        \
    }

TLB *create_tlb()
{
    TLB *tlb;
    FAIL_IF(!(tlb = malloc(sizeof(TLB))), "TLB head malloc failure!");
    tlb->front = NULL;
    tlb->count = 0;

    return tlb;
}

void delete_tlb(TLB *tlb)
{
    TLBE *entry;
    while ((entry = tlb_dequeue(tlb)))
    {
        free(entry);
    }
    free(tlb);
}

TLBE *init_tlb_entry(int vpn, int pfn)
{
    TLBE *entry;
    FAIL_IF(!(entry = malloc(sizeof(TLBE))), "TLB entry malloc failure!");
    entry->vpn = vpn;
    entry->pfn = pfn;
    entry->next = NULL;

    return entry;
}

int tlb_enqueue(TLB *tlb, TLBE *entry)
{
    TLBE *p;

    for (p = tlb->front; p != NULL; p = p->next)
    {
        if (!p->next)
            break;
    } // p = last node in the linked list

    if (!p)
        tlb->front = entry;
    else
        p->next = entry;
    entry->next = NULL;

    (tlb->count)++;

    return 0;
}

TLBE *tlb_dequeue(TLB *tlb)
{
    TLBE *p;

    if ((p = tlb->front))
    {
        tlb->front = p->next;
        p->next = NULL;
        (tlb->count)--;
        return p;
    }
    return NULL;
}

int tlb_find_idx_entry(TLB *tlb, int idx)
{
    TLBE *p = tlb->front;
    int i = 0;

    while (i < idx)
    {
        p = p->next;
        i++;
    }

    return p->vpn;
}

TLBE *tlb_extract(TLB *tlb, int vpn)
{
    TLBE *p, *prev;

    if ((p = tlb->front))
    {
        if (p->vpn == vpn)
        {
            tlb->front = p->next;
            p->next = NULL;
            (tlb->count)--;
            return p;
        }
    }
    else
        return NULL;

    prev = p;
    p = p->next;

    for (; p != NULL; p = p->next, prev = prev->next)
    {
        if (p->vpn == vpn)
        {
            prev->next = p->next;
            p->next = NULL;
            (tlb->count)--;
            return p;
        }
    }
    return NULL;
}