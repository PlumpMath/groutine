// Copyright (c) 2017, The groutine Authors.
// All rights reserved.
//
// Author: Zheng Gonglin <scaugrated@163.com>
// Created: 2017/02/06

#include "groutine/groutine.h"

#include <assert.h>
#include <ucontext.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace groutine {

// private function.
void FreeCoroutineList(Coroutine* head);
void CoFunctionWrapper(Coroutine* co);
void SaveCoStack(Coroutine* co);
inline void MallocCoStack(Coroutine* co, const size_t stack_size);
inline void FreeCoStack(Coroutine* co);

const size_t ShareStack::DefaultStackSize = 1024 * 1024 * 8;
const unsigned int CoroutineManager::DefaultMaxDeadCoroutineNum = 1024;

ShareStack::ShareStack():
    stack(new char[DefaultStackSize]),
    stack_size(DefaultStackSize) {}

ShareStack::~ShareStack() {
    if (stack) {
        delete[] stack;
    }
}

void ShareStack::Debug(Coroutine* co) {
    for (int i = 0; i < 16; i++) {
        printf("%d|", stack[stack_size-1-i]);
    }
    printf(" coroutine used stack size: %lu\n", co->used_size);
}

Coroutine::Coroutine(CoroutineManager* _co_manager,
                     CoFunction _function,
                     void* _function_arg):
        co_manager(_co_manager),
        state(CO_STATE_READY),
        function(_function),
        function_arg(_function_arg),
        stack(NULL),
        stack_size(0),
        used_size(0),
        next(NULL) {}

Coroutine::~Coroutine() {
    if (stack) {
        FreeCoStack(this);
    }
}

CoroutineManager::CoroutineManager():
        alive_co(NULL),
        dead_co(NULL),
        dead_co_num(0),
        running_co(NULL) {}

CoroutineManager::~CoroutineManager() {
    if (alive_co) {
        FreeCoroutineList(alive_co);
    }
    if (dead_co) {
        FreeCoroutineList(dead_co);
    }
}

CoroutineManager* CreateCoroutineManager() {
    return new CoroutineManager();
}

void FreeCoroutineManager(CoroutineManager* co_manager) {
    assert(co_manager);
    delete co_manager;
}

Coroutine* CoCreate(CoroutineManager* co_manager,
                    CoFunction co_function,
                    void* co_function_arg) {
    assert(co_manager);

    Coroutine* co = NULL;

    if (co_manager->dead_co != NULL) {
        // get coroutine from dead-coroutine list.
        co = co_manager->dead_co;
        co_manager->dead_co = co_manager->dead_co->next;
        --co_manager->dead_co_num;
        co->state = CO_STATE_READY;
        co->function = co_function;
        co->function_arg = co_function_arg;
    } else {
        co = new Coroutine(co_manager, co_function, co_function_arg);
    }

    // add coroutine to alive-coroutine list.
    if (!co_manager->alive_co) {
        co->next = NULL;
        co_manager->alive_co = co;
    } else {
        co->next = co_manager->alive_co;
        co_manager->alive_co = co;
    }

    return co;
}

void CoResume(Coroutine* co) {
    CoroutineManager* co_manager = co->co_manager;
    CoroutineState state = co->state;

    // can't CoResume in Coroutine function.
    assert(!co_manager->running_co);

    switch (state) {
        case CO_STATE_READY:
            getcontext(&(co->ctx));
            // use ShareStack as running stack for running coroutine.
            co->ctx.uc_stack.ss_sp = co_manager->running_stack.stack;
            co->ctx.uc_stack.ss_size = co_manager->running_stack.stack_size;
            co->ctx.uc_link = &(co_manager->ctx);
            makecontext(&(co->ctx), (void (*)(void))CoFunctionWrapper, 1, co);
            co_manager->running_co = co;
            co->state = CO_STATE_RUNNING;
            swapcontext(&(co_manager->ctx), &(co->ctx));
            break;
        case CO_STATE_SUPPEND:
            memcpy(co_manager->running_stack.stack + \
                   co_manager->running_stack.stack_size - \
                   co->used_size,
                   co->stack,
                   co->used_size);
            co_manager->running_co = co;
            co->state = CO_STATE_RUNNING;
            swapcontext(&(co_manager->ctx), &(co->ctx));
            break;
        default:
            assert(0);
    }
}

void CoYield(Coroutine* co) {
    CoroutineManager* co_manager = co->co_manager;
    CoroutineState state = co->state;

    switch (state) {
        case CO_STATE_RUNNING:
            // copy stack data from ShareStack to coroutine's stack.
            SaveCoStack(co);
            co_manager->running_co = NULL;
            co->state = CO_STATE_SUPPEND;
            swapcontext(&(co->ctx), &(co_manager->ctx));
            break;
        default:
            assert(0);
    }
}

void SaveCoStack(Coroutine* co) {
    CoroutineManager* co_manager = co->co_manager;
    // co_manager->running_stack.Debug(co);

    // only for x86 CPU(Little endian)
    char* bottom = co_manager->running_stack.stack +
                   co_manager->running_stack.stack_size;

    char dummy_var;
    size_t used_stack_size = bottom - &dummy_var;
    assert(used_stack_size >= 0);
    assert(used_stack_size <= ShareStack::DefaultStackSize);

    if (!co->stack) {
        MallocCoStack(co, used_stack_size);
    } else if (co->stack_size < used_stack_size) {
        FreeCoStack(co);
        MallocCoStack(co, used_stack_size);
    }

    memcpy(co->stack, &dummy_var, used_stack_size);
    co->used_size = used_stack_size;
}

void CoFunctionWrapper(Coroutine* co) {
    CoFunction function = co->function;
    void* function_arg = co->function_arg;
    CoroutineManager* co_manager = co->co_manager;

    function(function_arg);

    co_manager->running_co = NULL;
    if (co->stack) {
        FreeCoStack(co);
    }
    co->state = CO_STATE_DEAD;

    if (co_manager->dead_co_num >=
        CoroutineManager::DefaultMaxDeadCoroutineNum) {
        delete co;
        return;
    }

    // put co to manager's dead-coroutine list.
    if (!co_manager->dead_co) {
        co_manager->dead_co = co;
    } else {
        co->next = co_manager->dead_co;
        co_manager->dead_co = co;
    }
    ++co_manager->dead_co_num;
}

inline void MallocCoStack(Coroutine* co, const size_t stack_size) {
    co->stack = new char[stack_size];
    co->stack_size = stack_size;
    co->used_size = 0;
}

inline void FreeCoStack(Coroutine* co) {
    delete[] co->stack;
    co->stack = NULL;
    co->stack_size = 0;
    co->used_size = 0;
}

void FreeCoroutineList(Coroutine* head) {
    Coroutine* next_co = head;
    while (head) {
        next_co = head->next;
        delete head;
        head = next_co;
    }
}

}  // namespace groutine
