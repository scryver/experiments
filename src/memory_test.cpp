enum MemoryAllocFlags
{
    Memory_NoClear   = 0x01,
    Memory_Debug     = 0x0F,
};

struct MemoryAllocInfo
{
    u32 flags;
    u32 alignSize;
};

internal MemoryAllocInfo
default_memory_alloc(void)
{
    MemoryAllocInfo result = {};
    result.alignSize = 4;
}

internal MemoryAllocInfo
no_clear_memory_alloc(void)
{
    MemoryAllocInfo result = default_memory_alloc();
    result.flags |= Memory_NoClear;
}

internal MemoryAllocInfo
debug_memory_alloc(void)
{
    MemoryAllocInfo result = default_memory_alloc();
    result.flags |= Memory_Debug;
}

internal MemoryAllocInfo
align_memory_alloc(u32 alignment)
{
    i_expect(is_pow2(alignment));
    MemoryAllocInfo result = default_memory_alloc();
    result.alignSize = alignment;
}

#define ALLOCATE_MEMORY_SIZE(name)    void *name(void *allocator, umm size, MemoryAllocInfo allocInfo)
typedef ALLOCATE_MEMORY_SIZE(AllocateMemorySize);

#define REALLOCATE_MEMORY_SIZE(name)  void *name(void *allocator, void *memory, umm size, MemoryAllocInfo allocInfo)
typedef REALLOCATE_MEMORY_SIZE(ReallocateMemorySize);

#define DEALLOCATE_MEMORY_SIZE(name)  void *name(void *allocator, void *memory, umm size)
typedef DEALLOCATE_MEMORY_SIZE(DeallocateMemorySize);

struct MemoryContext
{
    AllocateMemorySize   *allocate;
    ReallocateMemorySize *reallocate;
    DeallocateMemorySize *deallocate;
    void *allocator;
};

internal MemoryContext
initialize_memory(AllocateMemorySize *allocate, ReallocateMemorySize *reallocate = 0, DeallocateMemorySize *deallocate = 0, void *allocator = 0)
{
    MemoryContext result = {};
    result.allocate = allocate;
    result.reallocate = reallocate;
    result.deallocate = deallocate;
    result.allocator = allocator;
    return result;
}

#define allocate_array(context, type, count, ...) (type *)allocate_size(context, sizeof(type)*count, ## __VA_ARGS__)
#define allocate_struct(context, type, ...)       (type *)allocate_size(context, sizeof(type), ## __VA_ARGS__)
internal void *
allocate_size(MemoryContext *context, umm size, MemoryAllocInfo allocInfo = default_memory_alloc())
{
    return context->allocate(context->allocator, size, allocInfo);
}

internal String
allocate_string(MemoryContext *context, u32 strLen)
{
    String result = {};
    result.size = strLen;
    result.data = allocate_size(context, strLen + 1);
    return result;
}

internal String
copy_string(MemoryContext *context, String source)
{
    String result = {};
    result.size = source.size;
    result.data = allocate_size(context, source.size + 1, no_clear_memory_alloc());
    copy(source.size, source.data, result.data);
    result.data[result.size] = 0;
    return result;
}

#define reallocate_array(context, memory, type, count, ...) (type *)reallocate_size(context, memory, sizeof(type)*count, ## __VA_ARGS__)
internal void *
reallocate_size(MemoryContext *context, void *memory, umm size, MemoryAllocInfo allocInfo = default_memory_alloc())
{
    i_expect(context->reallocate);
    context->reallocate(context->allocator, memory, size, allocInfo);
}

internal void *
deallocate(MemoryContext *context, void *memory, umm size)
{
    void *result = 0;
    if (context->deallocate)
    {
        result = context->deallocate(context->allocator, memory, size);
    }
    return result;
}

//
// NOTE(michiel): Standard lib alloc
//

internal ALLOCATE_MEMORY_SIZE(std_alloc)
{
    // TODO(michiel): Debug print
    void *result = 0;
    b32 clear = !(allocInfo.flags & Memory_NoClear);
    i_expect(allocInfo.alignment <= 8);

    if (clear)
    {
        result = calloc(size, 1);
    }
    else
    {
        result = malloc(size);
    }

    return result;
}

internal REALLOCATE_MEMORY_SIZE(std_realloc)
{
    void *result = realloc(memory, size);
    return result;
}

internal DEALLOCATE_MEMORY_SIZE(std_dealloc)
{
    free(memory);
    return 0;
}

internal MemoryContext
std_memory_context(void)
{
    MemoryContext result = initialize_memory(std_alloc, std_realloc, std_dealloc);
    return result;
}

//
// NOTE(michiel): Linux Arena blocks
//

struct LinuxArenaBlock
{
    struct LinuxArenaBlock *next;
    umm size;
    u8 mem[1];
};

struct LinuxArena
{
    umm blockSize;
    u32 alignment;
    u8 *at;
    u8 *end;
    ArenaBlock sentinel;
};

internal void
arena_grow(LinuxArena *arena, umm size)
{
    // TODO(michiel): Handle multithread
    umm blockSizeof = offset_of(LinuxArenaBlock, mem);
    umm newSize = align_up(maximum(size + blockSizeof, arena->blockSize), arena->alignment);
    LinuxArenaBlock *newBlock = mmap(0, newSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    newBlock->size = newSize - blockSizeof;
    arena->at = newBlock->mem;

    i_expect(arena->at == align_ptr_down(arena->at, arena->alignment));
    arena->end = arena->at + newBlock->size;

    newBlock->next = arena->sentinel.next;
    arena->sentinel.next = newBlock;
}

internal ALLOCATE_MEMORY_SIZE(linux_arena_alloc)
{
    LinuxArena *arena = (LinuxArena *)allocator;
    i_expect(allocInfo.alignment <= arena->alignment);
    b32 clear = !(allocInfo.flags & Memory_NoClear);

    if (size > (umm)(arena->end - arena->at)) {
        arena_grow(arena, size);
        clear = false;
    }
    i_expect(size <= (umm)(arena->end - arena->at));

    void *at = arena->at;
    i_expect(at == align_ptr_down(at, arena->alignment));

    arena->at = (u8 *)align_ptr_up(arena->at + size, arena->alignment);
    i_expect(arena->at <= arena->end);

    if (clear)
    {
        copy_single(size, 0, at);
    }

    return at;
}

internal void
deallocate_block(LinuxArenaBlock *block)
{
    umm blockSizeof = offset_of(LinuxArenaBlock, mem);
    umm realSize = blockSizeof + block->size;
    munmap(block, realSize);
}

internal void
deallocate_arena(LinuxArena *arena)
{
    i_expect(arena->sentinel.size == 0);

    for (ArenaBlock *it = arena->sentinel.next;
         it;
         )
    {
        ArenaBlock *next = it->next;
        deallocate_block(it);
        it = next;
    }
    arena->at = 0;
    arena->end = 0;
    arena->sentinel.next = 0;
}

internal DEALLOCATE_MEMORY_SIZE(linux_arena_dealloc)
{
    if (
        LinuxArena *arena = (LinuxArena *)allocator;
        i_expect(arena->sentinel.size == 0);

        for (ArenaBlock *it = arena->sentinel.next;
             it;
             )
        {
        ArenaBlock *next = it->next;
        deallocate_block(it);
        it = next;
        }
        arena->at = 0;
        arena->end = 0;
        arena->sentinel.next = 0;
        }

        internal MemoryContext
        linux_arena_memory_context(umm blockSize, u32 alignment)
        {

        MemoryContext result = initialize_memory(std_alloc, std_realloc, std_dealloc);
        return result;
        }

