#include <stddef.h>
#include <stdio.h>
#include <string.h>

// 定义内存池大小（字节）
#define MEMORY_POOL_SIZE 1024 * 10  // 10KB

// 内存块的最小尺寸
#define MIN_BLOCK_SIZE sizeof (MemoryBlockHeader)

// 内存块状态枚举
typedef enum
{
    FREE = 0,
    ALLOCATED = 1
} BlockStatus;

// 内存块头部结构体
typedef struct MemoryBlockHeader
{
    size_t size;                     // 数据区大小(不包括头部)
    BlockStatus status;              // 块状态：FREE 或 ALLOCATED
    struct MemoryBlockHeader* next;  // 指向下一个块的指针
    struct MemoryBlockHeader* prev;  // 指向上一个块的指针
} MemoryBlockHeader;

// 内存池和初始块
static unsigned char memory_pool[MEMORY_POOL_SIZE];
static MemoryBlockHeader* first_block = NULL;
static int is_initialized = 0;

// 内存管理初始化
void Memory_Init (void)
{
    if (is_initialized)
    {
        return;
    }

    // 将第一个块放在内存池开始处
    first_block = (MemoryBlockHeader*)memory_pool;

    // 设置第一个块的属性
    first_block->size = MEMORY_POOL_SIZE - sizeof (MemoryBlockHeader);
    first_block->status = FREE;
    first_block->next = NULL;
    first_block->prev = NULL;

    is_initialized = 1;
}

// 分配内存
void* My_Malloc (size_t size)
{
    MemoryBlockHeader* current;
    MemoryBlockHeader* new_block;

    // 确保内存管理已初始化
    if (!is_initialized)
    {
        Memory_Init ();
    }

    // 对齐到4字节边界（在某些系统上提高效率）
    size = (size + 3) & ~3;

    // 检查申请的大小是否有效
    if (size == 0)
    {
        return NULL;
    }

    // 遍历内存块链表寻找合适的空闲块
    current = first_block;

    while (current != NULL)
    {
        // 找到空闲且足够大的块
        if (current->status == FREE && current->size >= size)
        {
            // 检查是否需要分割该块
            if (current->size >=
                size + sizeof (MemoryBlockHeader) + MIN_BLOCK_SIZE)
            {
                // 计算新块位置
                new_block =
                    (MemoryBlockHeader*)((unsigned char*)current +
                                         sizeof (MemoryBlockHeader) + size);

                // 设置新块属性
                new_block->size =
                    current->size - size - sizeof (MemoryBlockHeader);
                new_block->status = FREE;
                new_block->next = current->next;
                new_block->prev = current;

                // 更新当前块属性
                current->size = size;
                current->next = new_block;

                // 更新下一个块的前向指针（如果存在）
                if (new_block->next != NULL)
                {
                    new_block->next->prev = new_block;
                }
            }

            // 标记当前块为已分配
            current->status = ALLOCATED;

            // 返回数据区指针（跳过头部）
            return (void*)((unsigned char*)current +
                           sizeof (MemoryBlockHeader));
        }

        current = current->next;
    }

    // 没找到合适的块
    return NULL;
}

// 释放内存
void My_Free (void* ptr)
{
    MemoryBlockHeader* block;
    MemoryBlockHeader* next_block;
    MemoryBlockHeader* prev_block;

    // 检查空指针
    if (ptr == NULL)
    {
        return;
    }

    // 获取块头指针（从数据区指针向前偏移）
    block =
        (MemoryBlockHeader*)((unsigned char*)ptr - sizeof (MemoryBlockHeader));

    // 检查指针是否在内存池范围内
    if ((unsigned char*)block < memory_pool ||
        (unsigned char*)block >= memory_pool + MEMORY_POOL_SIZE)
    {
        printf ("Error: Pointer outside memory pool\n");
        return;
    }

    // 标记为空闲
    block->status = FREE;

    // 尝试与下一个块合并（如果也是空闲的）
    next_block = block->next;
    if (next_block != NULL && next_block->status == FREE)
    {
        block->size += sizeof (MemoryBlockHeader) + next_block->size;
        block->next = next_block->next;

        if (next_block->next != NULL)
        {
            next_block->next->prev = block;
        }
    }

    // 尝试与前一个块合并（如果也是空闲的）
    prev_block = block->prev;
    if (prev_block != NULL && prev_block->status == FREE)
    {
        prev_block->size += sizeof (MemoryBlockHeader) + block->size;
        prev_block->next = block->next;

        if (block->next != NULL)
        {
            block->next->prev = prev_block;
        }
    }
}

// 获取内存使用情况的函数（用于调试）
void Memory_Stats (void)
{
    MemoryBlockHeader* current = first_block;
    int block_count = 0;
    int free_blocks = 0;
    size_t total_free = 0;

    printf ("Memory Pool Stats:\n");
    printf ("------------------\n");

    while (current != NULL)
    {
        printf ("Block %d: Address: %p, Size: %zu, Status: %s\n", block_count,
                (void*)((unsigned char*)current + sizeof (MemoryBlockHeader)),
                current->size, current->status == FREE ? "FREE" : "ALLOCATED");

        if (current->status == FREE)
        {
            free_blocks++;
            total_free += current->size;
        }

        current = current->next;
        block_count++;
    }

    printf ("\nSummary:\n");
    printf ("Total blocks: %d\n", block_count);
    printf ("Free blocks: %d\n", free_blocks);
    printf ("Total free space: %zu bytes\n", total_free);
    printf ("Total memory pool size: %d bytes\n", MEMORY_POOL_SIZE);
}

// 测试代码
int main (void)
{
    void* ptr1;
    void* ptr2;
    void* ptr3;

    // 初始化内存管理
    Memory_Init ();

    // 初始状态
    printf ("Initial memory state:\n");
    Memory_Stats ();

    // 分配内存测试
    ptr1 = My_Malloc (100);
    printf ("\nAfter allocating 100 bytes:\n");
    Memory_Stats ();

    ptr2 = My_Malloc (200);
    printf ("\nAfter allocating 200 bytes:\n");
    Memory_Stats ();

    ptr3 = My_Malloc (300);
    printf ("\nAfter allocating 300 bytes:\n");
    Memory_Stats ();

    // 释放内存测试
    My_Free (ptr2);
    printf ("\nAfter freeing the second allocation:\n");
    Memory_Stats ();

    My_Free (ptr1);
    printf ("\nAfter freeing the first allocation:\n");
    Memory_Stats ();

    My_Free (ptr3);
    printf ("\nAfter freeing all allocations:\n");
    Memory_Stats ();

    return 0;
}