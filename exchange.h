//
// Created by fangzhuhe on 2020/10/16.
//
#pragma once
#ifndef EXCHANGE_EXCHANGE_H
#define EXCHANGE_EXCHANGE_H
#define LIMIT 1000000000
#define MOD_LIMIT 10000
#define CHUNK_NUM 10240
#define THREAD_NUM 4
#define MSG_SIZE 40*1024*1024
#define PER_MSG_SIZE 10*1024*1024
#define MIN_MSG_SIZE 1024*1024

#include <string>
#include <random>
#include <grpcpp/grpcpp.h>
#include <thread>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include "exchange.grpc.pb.h"
using exchange::ReqChunk;
struct ServerAddr{
    std::string ip;
    std::string port;
}addr[100];

constexpr int RAND_NUMS_TO_GENERATE = 10;

int randNum()
{
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<int> distr(MIN_MSG_SIZE, MSG_SIZE);
    return distr(eng);
}


ReqChunk* GenChunk(uint64_t size) {
    ReqChunk* chk = new ReqChunk;
    char* dataChunk = new char[PER_MSG_SIZE];
    chk->set_data(dataChunk,PER_MSG_SIZE);
    chk->set_size(PER_MSG_SIZE);
    return chk;
}

ReqChunk** GenChunkList(uint64_t size) {
  ReqChunk **chk_list = new ReqChunk*[size];
  for (auto i = 0; i < size; ++i) {
    ReqChunk *chk = new ReqChunk;
    auto chunk_size = randNum();
    char *dataChunk = (char*)malloc(chunk_size);
    // set data content
    memset(dataChunk,1,chunk_size);
    *(int*)dataChunk = chunk_size;
    dataChunk[chunk_size-1] = '\0';
    chk->set_allocated_data(reinterpret_cast<std::string *>(dataChunk));
    chk->set_size(chunk_size);
    chk_list[i] = chk;
  }
  return chk_list;
}
#endif //EXCHANGE_EXCHANGE_H
