
cfs


1: conception
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

n process, each of them run 1/n second.

公平体现在
a) 运行时间长是公平的，都是1/n
b) 运行顺序是公平的， fifo
c)尽可能少切换，一次竟可能运行完成自己的所有时间片

实现算法
rb tree
理想运行时间 = 1/n
实际已经运行时间 = x
等待时间 = 1/n - x

等待时间越长，越在rb tree 左边。

 
2：实际实现
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

a) vruntime is per-thread; it is a member nested within the task_struct.
b) Essentially, vruntime is a measure of the "runtime" of the thread - the amount of time it has spent on the processor. 
这个线程已经运行了多长时间, vtime的意义。
c) cfs算法核心，the task with the lowest vruntime is the task that most deserves to run, hence select it as 'next'.


Quick summary - vruntime calculation: (based on the book)
 - Most of the work is done in kernel/sched_fair.c:__update_curr() 
 - Called on timer tick
 - Updates the physical and virtual time 'current' has just spent on the processor
 - For tasks that run at default priority, i.e., nice value 0, the physical and virtual time spent is identical
 - Not so for tasks at other priority (nice) levels; thus the calculation of vruntime is affected by the priority of current using a load weight factor
 
    delta_exec = (unsigned long)(now – curr->exec_start); 
    delta_exec_weighted = calc_delta_fair(delta_exec, curr); 
    curr->vruntime += delta_exec_weighted;
    
    Neglecting some rounding and overflow checking, what calc_delta_fair does is to compute the value given by the following formula:
    delta_exec_weighed = delta_exec * (NICE_0_LOAD / curr->load.weight)


Linux Scheduler – CFS and Virtual Run Time (vruntime)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


In Linux Scheduler, Work In Progress on July 3, 2012 at 5:20 pm
This article explains concept of virtual run time as used in Linux CFS scheduler

What is Virtual run time?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Virtual run time is the weighted time a task has run on the CPU

Essentially, vruntime is a measure of the "runtime" of the thread - the amount of time it has spent on the processor. The whole point of CFS is to be fair to all; hence, the algo kind of boils down to a simple thing: (among the tasks on a given runqueue) the task with the lowest vruntime is the task that most deserves to run, hence select it as 'next'. (The actual implementation is done using an rbtree for efficiency).

Where is it stored?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Virtual run time is stored in vruntime member of struct sched_entity
Note that sched_entity contains scheduling related information of task and it is member of task_struct.

struct task_struct
{
….
struct sched_entity se;
…..
};

struct sched_entity
{
…
u64 vruntime;
….
};

What is the unit of vruntime?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
vruntime is measured in nano seconds

Where is vruntime updated?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
vruntime of current task is updated in __update_curr function defined in sched_fair.c.
__update_curr() is called from update_curr() function, again defined in sched_fair.c
update_curr() is called from many places, including periodic timer interrupt.

periodic timer interrupt
... and others
    update_curr()
        __update_curr --> update vruntime

How is vruntime updated?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
First, time elapsed since last call to update_curr is determined
Then, this time is weighted and added to vruntime of current task.

delta = current - (last update_curr)
real_delta = weighted(delta)
add the real_delta to current task


How is weighting factor calculated?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Factor depends on nice value of task.
weighting factor = weight of process with nice value 0 / weight of current task;
where ‘weight’ is roughly equivalent to 1024 * (1.25)^(-nice)

Some examples values of weight
weight is 820 for nice value 1
weight is 1024 for nice value 0
weight is 1277 for nice value -1

In other words, the weighting factor = 1.25^nice
How does this impact tasks with different nice values?

For nice value equal to 0, factor is 1; vruntime is same as real time spent by task on CPU.

For nice value less than 0, factor is < 1; vruntime is less than real time spent. vruntime grows slower than real time.

For nice value more than 0, factor is > 1; vruntime is more than real time spent. vruntime grows faster than real time.

Which functions are involved in updating vruntime?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Four functions are involved
(a) update_curr()
(b) __update_curr()
(c) calc_delta_fair()
(d) calc_delta_mine()

Brief description of what each of these functions do?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
update_curr determines time elapsed since last call to update_curr, then calls __update_curr with that time
__update_curr determines weighted time using calc_delta_fair (which in turn uses calc_delta_mine) and updates vruntime of current task.

Detailed description of what each of these functions do?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
calc_delta_mine()
Prototype
static unsigned long calc_delta_mine(unsigned long delta_exec, unsigned long weight, struct load_weight *lw)
Description
Returns (delta_exec * weight)/lw->weight;
    -- delta_exec_weighed = delta_exec * (NICE_0_LOAD / curr->load.weight)

calc_delta_fair
Prototype
static inline unsigned long calc_delta_fair(unsigned long delta, struct sched_entity *se)
Description:
Returns (delta * 1024)/se->load->weight;
Calls calc_delta_mine to do the processing.

static inline unsigned long calc_delta_fair(unsigned long delta_exec, struct load_weight *lw) 
{
    /* delta_exec_weighed = delta_exec * (NICE_0_LOAD / curr->load.weight) */
    return calc_delta_mine(delta_exec, NICE_0_LOAD, lw);
}

__update_curr()
Prototype
static inline void __update_curr(struct cfs_rq *cfs_rq, struct sched_entity *curr, unsigned long delta_exec)
Description
Determines (delta_exec * 1024)/curr->load->weight using calc_delta_fair
Adds that value to curr->vruntime


static inline void __update_curr(struct cfs_rq *cfs_rq, struct sched_entity *curr, unsigned long delta_exec)
{
    unsigned long delta_exec_weighted;
    u64 vruntime;

    curr->sum_exec_runtime += delta_exec;
    delta_exec_weighted = delta_exec;
    if (unlikely(curr->load.weight != NICE_0_LOAD)) {
        delta_exec_weighted = calc_delta_fair(delta_exec_weighted,
                            &curr->load);
    }
    curr->vruntime += delta_exec_weighted;

    /*
     * maintain cfs_rq->min_vruntime to be a monotonic increasing
     * value tracking the leftmost vruntime in the tree.
     */
    if (first_fair(cfs_rq)) {
        vruntime = min_vruntime(curr->vruntime, __pick_next_entity(cfs_rq)->vruntime);
    } else
        vruntime = curr->vruntime;

    cfs_rq->min_vruntime = max_vruntime(cfs_rq->min_vruntime, vruntime);
}


update_curr()
Prototype
static void update_curr(struct cfs_rq *cfs_rq)
Description
Determines time spent since last call to update_curr
Calls __update_curr to add weighted runtime to vruntime

static void update_curr(struct cfs_rq *cfs_rq)
{
    struct sched_entity *curr = cfs_rq->curr;
    u64 now = rq_of(cfs_rq)->clock;
    unsigned long delta_exec;

    if (unlikely(!curr))
        return;

    delta_exec = (unsigned long)(now - curr->exec_start);

    __update_curr(cfs_rq, curr, delta_exec);
    curr->exec_start = now;
}

static void place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial)
{
    u64 vruntime = cfs_rq->min_vruntime;

    if (initial) vruntime += sched_vslice_add(cfs_rq, se);  /* fork */

    if (!initial) {                                         /* wake up */
        vruntime -= sysctl_sched_latency;
        vruntime = max_vruntime(se->vruntime, vruntime);
    }

    se->vruntime = vruntime;
}

================================================
Minimum virtual run time of CFS RunQueue (min_vruntime)
================================================
What is min_vruntime?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Represents minimum run time of any task in the CFS run queue. -- runqueue中最小的vruntime, 但是实际上有的时候不是最小的。

Where is stored?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
It is part of CFS run queue structure (struct cfs_rq)
struct cfs_rq
{
…..
u64 min_vruntime;
…..
};

What is its initial value?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
min_vruntime is set to -1 << 20 in init_cfs_rq() function defined in sched.c

Where is min_vruntime updated?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
It is updated in update_min_vruntime() function defined in sched_fair.c
update_min_vruntime() is called from many places including
(a) __update_curr(), which is in turn called from update_curr()
(b) dequeue_entity(), which removes an entity from RB tree
    
__update_curr()
    队列的vruntime只有被tree上的某个节点的vruntime超出的时候，才更新
dequeue_entity()
    update_min_vruntime()

How is min_vruntime used?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline s64 entity_key(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    return se->vruntime - cfs_rq->min_vruntime;
}

static void __enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    ...
    s64 key = entity_key(cfs_rq, se);
    ...
}


How is min_vruntime updated?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Find the minimum vruntime of current task and leftmost task in run queue
Set this runtime as min_vruntime if it is greater than current value of min_vruntime
In other words

min_vruntime = MAX(min_vruntime, MIN(current_running_task.vruntime, cfs_rq.left_most_task.vruntime))

a) no task in queue
b) no xx

MIN(cur, tree_left) ==> cur is from tree, and used to be tree_left, after run a few time, maybe its vruntime is greater than current_tree_left.
we need the MIN value of the two, which means the real minimal vruntime value.

MAX(self, MIN) ==> self min_vruntime is ascendant, in some particular cases, MIN may be little than self.
For an instance: only a task in run queue, and this task is waked up just now.



================================================
Time related members in sched_entity
================================================
What are various time related members in sched_entity?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
There are following members, related to time
struct sched_entity {
…..
u64 exec_start;
u64 sum_exec_runtime;
u64 vruntime;
u64 prev_sum_exec_runtime;
…..
}

What are these members?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
exec_start
– Time when task started running on CPU
– *Used in update_curr() to find time duration run by current process on CPU*
– Reset in update_curr(), after using previous value to determine time current process ran on CPU

sum_exec_runtime
– Total time process ran on CPU
– In real time
– Nano second units
– Updated in __update_curr(), called from update_curr()

prev_sum_exec_runtime
– When a process is taken to the CPU, its current sum_exec_runtime value is preserved in prev_exec_runtime.
    
    
