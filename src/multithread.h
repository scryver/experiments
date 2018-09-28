struct PlatformWorkQueue;

#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(PlatformWorkQueue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(PlatformWorkQueueCallback);

struct PlatformWorkQueueEntry
{
    PlatformWorkQueueCallback *callback;
    void               *data;
};

#define MAX_WORK_QUEUE_ENTRIES 256
#define THREAD_STACK_SIZE      megabytes(8)

#if 0
typedef void PlatformAddEntry(PlatformWorkQueue *queue, PlatformWorkQueueCallback *callback, void *data);
typedef void PlatformCompleteAllWork(PlatformWorkQueue *queue);
#endif
