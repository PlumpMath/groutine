// Copyright (c) 2016, The groutine Authors.
// All rights reserved.
//
// Author: Zheng Gonglin <scaugrated@163.com>
// Created: 2017/02/06

#ifndef GROUTINE_GROUTINE_H_
#define GROUTINE_GROUTINE_H_

#include <ucontext.h>

namespace groutine {

struct CoroutineManager;
struct Coroutine;

typedef void* (*CoFunction) (void*);

enum CoroutineState {
    CO_STATE_READY,
    CO_STATE_RUNNING,
    CO_STATE_SUPPEND,
    CO_STATE_DEAD,
};

const size_t DEFAULT_CO_STACK_SIZE = 65535;

CoroutineManager* CreateCoroutineManager();
void FreeCoroutineManager(CoroutineManager* co_manager);

Coroutine* CoCreate(CoroutineManager* co_manager,
                    CoFunction co_function,
                    void* co_function_arg);
void CoResume(Coroutine* co);
void CoYield(Coroutine* co);

struct Coroutine {
    explicit Coroutine(CoroutineManager* _co_manage,
                       CoFunction _function,
                       void* _function_arg);
    ~Coroutine();

    CoroutineManager* co_manager;
    ucontext_t ctx;
    enum CoroutineState state;

    CoFunction function;
    void* function_arg;

    char* stack;
    size_t stack_size;

    Coroutine* next;
};

struct CoroutineManager {
    CoroutineManager();
    ~CoroutineManager();

    ucontext_t ctx;
    // list of Coroutine;
    Coroutine* alive_co;
    // list of Coroutine;
    Coroutine* dead_co;
    // Coroutine pointer
    Coroutine* running_co;
};

}  // namespace groutine

#endif  // GROUTINE_GROUTINE_H_
