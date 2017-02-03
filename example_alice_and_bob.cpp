// Copyright (c) 2017, The groutine Authors.
// All rights reserved.
//
// Author: Zheng Gonglin <scaugrated@163.com>
// Created: 2017/02/08

#include <cstdio>

#include "groutine/groutine.h"

using groutine::CoroutineManager;
using groutine::Coroutine;
using groutine::CreateCoroutineManager;
using groutine::FreeCoroutineManager;
using groutine::CoCreate;
using groutine::CoResume;
using groutine::CoYield;

Coroutine* co_alice = NULL;
Coroutine* co_bob = NULL;

void* Alice(void* arg) {
    printf("Alice: Hello, I am Alice.\n");
    CoYield(co_alice);
    printf("Alice: How old are you?\n");
    CoYield(co_alice);
    printf("Alice: I am 18 years old.\n");
    CoYield(co_alice);
    printf("Alice: Nice to meet you too, bye bye.\n");
    CoYield(co_alice);
}

void* Bob(void* arg) {
    printf("  Bob: I am Bob.\n");
    CoYield(co_bob);
    printf("  Bob: I am 16 years old, and you?\n");
    CoYield(co_bob);
    printf("  Bob: Nice to meet you.\n");
    CoYield(co_bob);
    printf("  Bob: Bye bye.\n");
    CoYield(co_bob);
}

int main() {
    CoroutineManager* co_manager = CreateCoroutineManager();
    co_alice = CoCreate(co_manager, Alice, NULL);
    co_bob = CoCreate(co_manager, Bob, NULL);
    for (int i = 0; i < 4; i++) {
        CoResume(co_alice);
        CoResume(co_bob);
    }
    FreeCoroutineManager(co_manager);
    return 0;
}
