#ifndef puzzle
#define puzzle

typedef enum {
    CMD
} puzzle_task_action_t;

// FreeRTOS task priorities
#define PUZZLE_TASK_PRIO                7  // Puzzle_Task
#define GENERIC_TASK_PRIO               1  // Any unspecified task

void puzzle_init(void);
void puzzle_task(void *arg);

#endif