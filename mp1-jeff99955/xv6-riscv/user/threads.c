#include "kernel/types.h"
#include "user/setjmp.h"
#include "user/threads.h"
#include "user/user.h"
#define NULL 0


static struct thread* current_thread = NULL;
static int id = 1;
static jmp_buf env_st;
// static jmp_buf env_tmp;

struct thread *thread_create(void (*f)(void *), void *arg){
    struct thread *t = (struct thread*) malloc(sizeof(struct thread));
    //unsigned long stack_p = 0;
    unsigned long new_stack_p;
    unsigned long new_stack;
    new_stack = (unsigned long) malloc(sizeof(unsigned long)*0x100);
    new_stack_p = new_stack +0x100*8-0x2*8;
    t->fp = f;
    t->arg = arg;
    t->ID  = id;
    t->buf_set = 0;
    t->stack = (void*) new_stack;
    t->stack_p = (void*) new_stack_p;
    id++;
    return t;
}
void thread_add_runqueue(struct thread *t){
    if(current_thread == NULL){
        // TODO
        current_thread = t;
        t->previous = t->next = t;
    }
    else{
        // TODO
        t->previous = current_thread->previous;
        t->next = current_thread;
        current_thread->previous->next = t;
        current_thread->previous = t;
    }
}
void thread_yield(void){
    // TODO
    if (setjmp(current_thread->env) == 0) {
        schedule();
        dispatch();
    }
}
void dispatch(void){
    // TODO
    if (current_thread->buf_set)
        longjmp(current_thread->env, 1);
    if (setjmp(current_thread->env) == 0) {
        current_thread->env->sp = (unsigned long)current_thread->stack_p;
        longjmp(current_thread->env, 1);
    }
    current_thread->buf_set = 1;
    current_thread->fp(current_thread->arg);
    thread_exit();
}
void schedule(void){
    current_thread = current_thread->next;
}
void thread_exit(void){
    if(current_thread->next != current_thread){
        // TODO
        current_thread->next->previous = current_thread->previous;
        current_thread->previous->next = current_thread->next;
        struct thread *tf = current_thread;
        current_thread = current_thread->next;
        free(tf->stack);
        free(tf->stack_p);
        free(tf);
    }
    else{
        // TODO
        // Hint: No more thread to execute
        free(current_thread->stack);
        free(current_thread->stack_p);
        free(current_thread);
        longjmp(env_st, 1);
    }
    dispatch();
}
void thread_start_threading(void){
    // TODO
    if (setjmp(env_st) != 0) {
        return;
    }
    schedule();
    dispatch();
}
