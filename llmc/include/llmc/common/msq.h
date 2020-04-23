#pragma once

#include <stdio.h>
#include <pthread.h>
#include <atomic>

namespace common {

struct Node;

struct NodePtr {
public:
    Node* ptr;
    intptr_t count;

    bool operator==(NodePtr const& other) {
        return this->ptr == other.ptr
               && this->count == other.count;
    }

    NodePtr(Node* volatile const& ptr, intptr_t const& count) noexcept
            : ptr(ptr)
            , count(count)
    {
    }

    NodePtr(Node* const& ptr, intptr_t const& count) noexcept
            : ptr(ptr)
            , count(count)
    {
    }

    NodePtr() noexcept
            : ptr(nullptr)
            , count(0)
    {
    }

//	volatile NodePtr& operator=(NodePtr const& d) volatile {
//		memcpy((void*)this,(void*)&d,sizeof(d));
//		return *this;
//	}

//	void store(NodePtr const& d, std::memory_order model = std::memory_order_seq_cst) volatile  {
//		if(model==std::memory_order_release || model==std::memory_order_seq_cst)
//			llmc_barrier_release();
//		memcpy((void*)this,(void*)&d,sizeof(d));
//		//ptr = d.ptr;
//		//count = d.count;
//	}
//	NodePtr load(std::memory_order model = std::memory_order_seq_cst) volatile {
//		NodePtr d;
//		load(d,model);
//		return d;
//	}
//	void load(NodePtr& d, std::memory_order model = std::memory_order_seq_cst) volatile {
//		memcpy((void*)&d,(void*)this,sizeof(d));
//		//d.ptr = ptr;
//		//d.count = count;
//		if(model==std::memory_order_acquire || model==std::memory_order_seq_cst)
//			llmc_barrier_acquire();
//	}
//	bool compare_exchange_strong(NodePtr volatile& expected, NodePtr const& desired, std::memory_order const& mo = std::memory_order_seq_cst) volatile {
//		return __atomic_compare_exchange_llmc(sizeof(NodePtr), (void*)this, (void*)&expected, (void*)&desired, mo, mo);
////		return __atomic_compare_exchange_llmc(sizeof(NodePtr), this, &expected, &desired, std::memory_order_relaxed, std::memory_order_relaxed);
//	}
//	bool compare_exchange_weak(NodePtr volatile& expected, NodePtr const& desired, std::memory_order const& mo = std::memory_order_seq_cst) volatile {
//		return __atomic_compare_exchange_llmc(sizeof(NodePtr), (void*)this, (void*)&expected, (void*)&desired, mo, mo);
////		return __atomic_compare_exchange_llmc(sizeof(NodePtr), this, &expected, &desired, std::memory_order_relaxed, std::memory_order_relaxed);
//	}

};

struct Node {
public:
    std::atomic<NodePtr> next __attribute__ ((aligned (16)));
    size_t data;

    Node() : next(), data() {}
} __attribute__ ((aligned (16)));


//int const MAX_NODES = 200;
//Node globalMemory[MAX_NODES] __attribute__ ((aligned (16)));
//bool inUse[MAX_NODES] __attribute__ ((aligned (16)));
//
//Node* newNode() {
//    int i = 0;
//    bool F = false;
//    bool T = true;
//    do {
//        //	i = 0;
//        //while(i<MAX_NODES && inUse[i].load(std::memory_order_relaxed)) i++;
//        i++;
//        assert(i < MAX_NODES);
//        F = false;
////			printf("[n] 1 __atomic_compare_exchange(%p, %p, %p)\n", &inUse[i], &F, &T);
//    } while(!__atomic_compare_exchange(&inUse[i], &F, &T, true, std::memory_order_relaxed, std::memory_order_relaxed));
//    //assert(globalMemory[i].data != 1000);
//    return &globalMemory[i];
//}
//
//void freeNode(Node* node) {
//    inUse[node - globalMemory] = false;
//}

Node* newNode() {
    return new Node;
}

void freeNode(Node* node) {
//    delete node;
}

struct MSQ {
public:
    volatile std::atomic<NodePtr> Head;
    volatile std::atomic<NodePtr> Tail;

public:
    MSQ() {
//        Node* node = &globalMemory[1];
//        inUse[1] = true;
        Node* node = newNode();
        node->next = NodePtr(nullptr, 0);
        Head = NodePtr(node, 0);
        Tail = NodePtr(node, 0);
    }

    void enqueue(size_t const& data) {
        NodePtr tail;
        NodePtr next;
        NodePtr tail2;
        Node* node = newNode();
        node->data = data;
        node->next.store(NodePtr(nullptr, 0), std::memory_order_relaxed);
        int i = 0;
        while(true) {
            tail = Tail.load(std::memory_order_relaxed);
            next = tail.ptr->next.load(std::memory_order_relaxed);
            tail2 = Tail.load(std::memory_order_relaxed);
            if(tail.ptr == tail2.ptr && tail.count == tail2.count) {
                if(!next.ptr) {
                    if(tail.ptr->next.compare_exchange_weak(next, NodePtr(node, next.count + 1),
                                                            std::memory_order_relaxed)) {
                        break;
                    }
                } else {
                    Tail.compare_exchange_strong(tail, NodePtr(next.ptr, tail.count + 1), std::memory_order_relaxed);
                }
            } else {
            }
        }
        Tail.compare_exchange_strong(tail, NodePtr(node, tail.count + 1), std::memory_order_relaxed);
    }

    bool dequeue(size_t& data) {
        NodePtr head;
        NodePtr head2;
        NodePtr tail;
        NodePtr next;
        int i = 0;
        while(true) {
            head = Head.load(std::memory_order_relaxed);
            tail = Tail.load(std::memory_order_relaxed);
            next = head.ptr->next.load(std::memory_order_relaxed);
            head2 = Head.load(std::memory_order_relaxed);
            if(head.ptr == head2.ptr && head.count == head2.count) {  // Count is needed to fix: E; E; D || (D; E); D
                if(head.ptr == tail.ptr) {
                    if(!next.ptr) return false;
                    Tail.compare_exchange_strong(tail, NodePtr(next.ptr, tail.count + 1), std::memory_order_relaxed);
                } else {
                    data = next.ptr->data;
                    if(Head.compare_exchange_weak(head, NodePtr(next.ptr, head.count + 1), std::memory_order_relaxed)) {
                        break;
                    }
                }
            }
        }
        //head.ptr->data = 1000;
        freeNode(head.ptr);
        return true;
    }

};

}