#include <stdio.h>
#include <pthread.h>
#define BUFFER_SIZE 16 // 缓冲区数量
struct prodcons
{
        // 缓冲区相关数据结构
        int buffer[BUFFER_SIZE]; /* 实际数据存放的数组*/
        pthread_mutex_t lock; /* 互斥体lock 用于对缓冲区的互斥操作 */
        int readpos, writepos; /* 读写指针*/
        pthread_cond_t notempty; /* 缓冲区非空的条件变量 */
        pthread_cond_t notfull; /* 缓冲区未满的条件变量 */
};
/* 初始化缓冲区结构 */
void init(struct prodcons *b)
{
        pthread_mutex_init(&b->lock, NULL);
        pthread_cond_init(&b->notempty, NULL);
        pthread_cond_init(&b->notfull, NULL);
        b->readpos = 0;
        b->writepos = 0;
}
/* 将产品放入缓冲区,这里是存入一个整数*/
void put(struct prodcons *b, int data)
{
        pthread_mutex_lock(&b->lock);
        /* 等待缓冲区未满*/
        if ((b->writepos + 1) % BUFFER_SIZE == b->readpos)
        {
                pthread_cond_wait(&b->notfull, &b->lock);
        }
        /* 写数据,并移动指针 */
        b->buffer[b->writepos] = data;
        b->writepos++;
        if (b->writepos >= BUFFER_SIZE)
        {
                b->writepos = 0;
        }
        /* 设置缓冲区非空的条件变量*/
        pthread_cond_signal(&b->notempty);
        pthread_mutex_unlock(&b->lock);
}
/* 从缓冲区中取出整数*/
int get(struct prodcons *b)
{
        int data;
        pthread_mutex_lock(&b->lock);
        /* 等待缓冲区非空*/
        if (b->writepos == b->readpos)
        {
                pthread_cond_wait(&b->notempty, &b->lock);
        }
        /* 读数据,移动读指针*/
        data = b->buffer[b->readpos];
        b->readpos++;
        if (b->readpos >= BUFFER_SIZE)
        {
                b->readpos = 0;
        }
        /* 设置缓冲区未满的条件变量*/
        pthread_cond_signal(&b->notfull);
        pthread_mutex_unlock(&b->lock);
        return data;
}

/* 测试:生产者线程将1 到10000 的整数送入缓冲区,消费者线
   程从缓冲区中获取整数,两者都打印信息*/
#define OVER (-1)
struct prodcons buffer;
void *producer(void *data)
{
        int n;
        for (n = 0; n < 10000; n++)
        {
                printf("%d --->\n", n);
                put(&buffer, n);
        }
        put(&buffer, OVER);
        return NULL;
}

void *consumer(void *data)
{
        int d;
        while (1)
        {
                d = get(&buffer);
                if (d == OVER)
                {
                        break;
                }
                printf("--->%d \n", d);
        }
        return NULL;
}

int main(void)
{
        pthread_t th_a, th_b;
        void *retval;
        init(&buffer);
        /* 创建生产者和消费者线程*/
        pthread_create(&th_a, NULL, producer, 0);
        pthread_create(&th_b, NULL, consumer, 0);
        /* 等待两个线程结束*/
        pthread_join(th_a, &retval);
        pthread_join(th_b, &retval);
        return 0;
}

/*
gcc -o threadcommunication.o -c threadcommunication.c 
gcc -o threadcommunication.elf threadcommunication.o -lpthread
*/


