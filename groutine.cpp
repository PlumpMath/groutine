// Copyright (c) 2017, The groutine Authors.
// All rights reserved.
//
// Author: Zheng Gonglin <scaugrated@163.com>
// Created: 2017/02/06

#include "groutine/groutine.h"

#include <assert.h>
#include <ucontext.h>

#include <cstdlib>

namespace groutine {

// private function.
void FreeCoroutineList(Coroutine* head);
void CoFunctionWrapper(Coroutine* co);
// TODO(Zheng Gonglin): update malloc and free coroutine stack
void MallocCoStack(Coroutine* co);
void FreeCoStack(Coroutine* co);

Coroutine::Coroutine(CoroutineManager* _co_manager,
                     CoFunction _function,
                     void* _function_arg):
        co_manager(_co_manager),
        state(CO_STATE_READY),
        function(_function),
        function_arg(_function_arg),
        next(NULL) {
    MallocCoStack(this);
}

Coroutine::~Coroutine() {
    if (stack) {
        FreeCoStack(this);
    }
}

CoroutineManager::CoroutineManager():
        alive_co(NULL), dead_co(NULL), running_co(NULL) {}

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
            co->ctx.uc_stack.ss_sp = co->stack;
            co->ctx.uc_stack.ss_size = co->stack_size;
            co->ctx.uc_link = &(co_manager->ctx);
            makecontext(&(co->ctx), (void (*)(void))CoFunctionWrapper, 1, co);
        case CO_STATE_SUPPEND:
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
            co_manager->running_co = NULL;
            co->state = CO_STATE_SUPPEND;
            swapcontext(&(co->ctx), &(co_manager->ctx));
            break;
        default:
            assert(0);
    }
}

void CoFunctionWrapper(Coroutine* co) {
    CoFunction function = co->function;
    void* function_arg = co->function_arg;
    CoroutineManager* co_manager = co->co_manager;

    function(function_arg);

    co_manager->running_co = NULL;
    co->state = CO_STATE_DEAD;

    // put co to manager's dead-coroutine list.
    if (!co_manager->dead_co) {
        co_manager->dead_co = co;
    } else {
        co->next = co_manager->dead_co;
        co_manager->dead_co = co;
    }
}

void MallocCoStack(Coroutine* co) {
    // TODO(Zheng Gonglin): update malloc memory algorithm.
    co->stack = new char[DEFAULT_CO_STACK_SIZE];
    co->stack_size = DEFAULT_CO_STACK_SIZE;
}

void FreeCoStack(Coroutine* co) {
    // TODO(Zheng Gonglin): update free memory algorithm.
    delete[] co->stack;
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
