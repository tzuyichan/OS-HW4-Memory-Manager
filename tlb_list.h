typedef struct tlb_entry
{
    int vpn;
    int pfn;
    struct tlb_entry *next;
} TLBE;

typedef struct tlb_head
{
    TLBE *front;
    int count;
} TLB;

TLB *create_tlb();
void delete_tlb(TLB *tlb);
TLBE *init_tlb_entry(int vpn, int pfn);
int tlb_enqueue(TLB *tlb, TLBE *entry);
TLBE *tlb_dequeue(TLB *tlb);
int tlb_find_idx_entry(TLB *tlb, int idx);
TLBE *tlb_extract(TLB *tlb, int vpn);