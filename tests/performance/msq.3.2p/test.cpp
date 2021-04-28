// TEST: E | E | E | D | D, blocking D

#include <stdio.h>
#include <pthread.h>
#include <atomic>
#include <assert.h>

typedef int nodeptr32;

struct Node;

struct NodePtr {
public:
	nodeptr32 ptr;
	int count;

	bool operator==(NodePtr const& other) {
		return this->ptr == other.ptr
			&& this->count == other.count;
	}

	NodePtr(nodeptr32 const& ptr, int const& count) noexcept
		: ptr(ptr)
		, count(count)
		{
	}

	NodePtr(nodeptr32 volatile const& ptr, int const& count) noexcept
		: ptr(ptr)
		, count(count)
		{
	}

	NodePtr() noexcept
		: ptr(-1)
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
	int data;
	Node(): next(), data() {}
} __attribute__ ((aligned (16)));


int const MAX_NODES = 10;
Node globalMemory[MAX_NODES] __attribute__ ((aligned (16)));
bool inUse[MAX_NODES] __attribute__ ((aligned (16)));
	
	nodeptr32 newNode() {
		int i=0;
		bool F = false;
		bool T = true;
		do {
		//	i = 0;
			//while(i<MAX_NODES && inUse[i].load(std::memory_order_relaxed)) i++;
			i++;
			assert(i<MAX_NODES);
			F = false;
//			printf("[n] 1 __atomic_compare_exchange(%p, %p, %p)\n", &inUse[i], &F, &T);
		} while(!__atomic_compare_exchange(&inUse[i], &F, &T, true, std::memory_order_relaxed, std::memory_order_relaxed));
		//assert(globalMemory[i].data != 1000);
		return i;
	}
	
	void freeNode(nodeptr32 i) {
		inUse[i] = false;
	}
	
	Node* toPtr(nodeptr32 i) {
		return &globalMemory[i];
	}
	
	
struct MSQ {
public:
	volatile std::atomic<NodePtr> Head;
	volatile std::atomic<NodePtr> Tail;
	
public:
	MSQ() {
		Node* node = &globalMemory[1];
		assert(node);
		inUse[1] = true;
		node->next = NodePtr(-1, 0);
		Head = NodePtr(1, 0);
		Tail = NodePtr(1, 0);
		assert(Head.load(std::memory_order_relaxed)==Tail.load(std::memory_order_relaxed));
		assert(Head.load().ptr > 0);
//		printf("[i] Head.ptr: %p\n", Head.load().ptr);
	}
	
	void enqueue(int const& data, int nid) {
//		printf("[e] Head.ptr: %p\n", Head.load().ptr);
		assert(data);
		NodePtr tail;
		NodePtr next;
		NodePtr tail2;
		nodeptr32 nodeID = nid;//newNode();
		Node* node = toPtr(nodeID);
		node->data = data;
		assert(node->data);
		node->next.store(NodePtr(-1, 0), std::memory_order_relaxed);
//		printf("[e] Head.ptr: %p\n", Head.load().ptr);
//		printf("[e] created node with data: %u @ %p (node @ %p)\n", data, &node->data, node);
		int i=0;
		//llmc_barrier_release(); // Can find this one: E || D; E; E; D; D
		//assert(data!=3 || node==&globalMemory[1]);
		assert(node->next.load(std::memory_order_relaxed).ptr == -1);
		while(true) {

//			printf("====================\n");
			tail = Tail.load(std::memory_order_relaxed);
//			printf("tail  %p %p %zu\n", &tail, tail.ptr, tail.count);
// 			tail2.count = tail.count;
// 			tail2.ptr = tail.ptr;
//			Tail.load(tail, std::memory_order_relaxed);
			//llmc_barrier_acquire(); // not needed because of data dependency
			Node* tailptr = toPtr(tail.ptr);
			next = tailptr->next.load(std::memory_order_relaxed);
//			printf("next %p %p %zu\n", &next, next.ptr, next.count);
			//llmc_barrier_acquire(); // Can find this one
 			tail2 = Tail.load(std::memory_order_relaxed);
//			printf("tail2 %p %p %zu\n", &tail2, tail2.ptr, tail2.count);
//			tail.ptr->next.load(next, std::memory_order_relaxed);
//			Tail.load(tail2, std::memory_order_relaxed);
			assert(tail.ptr > 0);
			assert(tail2.ptr > 0);

//			printf("Tail %p %u\n", Tail.load().ptr, Tail.load().count);
			if(tail.ptr==tail2.ptr && tail.count==tail2.count)  {
//				printf("  they are equal\n");
				if(next.ptr <= 0) {
//					printf("  !next.ptr\n");
//					printf("[e] 2 __atomic_compare_exchange(%p)\n", tail.ptr);
					if(tailptr->next.compare_exchange_weak(next, NodePtr(nodeID, next.count+1), std::memory_order_relaxed)) {
//						printf("  cmpxchg success\n");
						break;
					}
//						printf("  cmpxchg failure\n");
				} else {
//					printf("  next %p %u\n", next.ptr, next.count);
//					printf("[e] 3 __atomic_compare_exchange(%p)\n", Tail.load().ptr);
					Tail.compare_exchange_strong(tail, NodePtr(next.ptr, tail.count+1), std::memory_order_relaxed);
//					printf("[e] 3 __atomic_compare_exchange(%p)\n", Tail.load().ptr);
				}
			} else {
//				printf("  they are not equal\n");
			}
		}
//		printf("[e] enqueued data: %u @ %p (node @ %p)\n", data, &node->data, node);
		//llmc_barrier_release(); // not needed because of data dependency
//		printf("[e] 4 __atomic_compare_exchange(%p) %p -> %p\n", Tail, Tail.load().ptr, tail.ptr);
		Tail.compare_exchange_strong(tail, NodePtr(nodeID, tail.count+1), std::memory_order_relaxed);
	}
	
	bool dequeue(int& data) {
		NodePtr head;
		NodePtr head2;
		NodePtr tail;
		NodePtr next;
		int i=0;
		while(true) {

			// Obtaining head and tail is NOT done atomically!
			// Atomically would avoid the need of a fence
			
//			Head.load(head, std::memory_order_relaxed); // 1
			head = Head.load(std::memory_order_relaxed);

//			printf("[d] head.ptr: %p\n", head.ptr);

			//llmc_barrier_acquire(); // Verified. Can find this one: E; D || (E; D; D; E)   or   E; D || (E; D; D) (can block!) needs a buffer size of 5

//			Tail.load(tail, std::memory_order_relaxed); // 2
			tail = Tail.load(std::memory_order_relaxed);
			
			//llmc_barrier_acquire(); // Verified. Can find this one: E || D; E; E; D; D needs a buffer size of 5

//			printf("[d] tail.ptr: %p\n", tail.ptr);
			Node* headptr = toPtr(head.ptr);
			next = headptr->next.load(std::memory_order_relaxed);
//			head.ptr->next.load(next, std::memory_order_relaxed); // 3

			//llmc_barrier_acquire(); // Verified. Can find this one: E; E; D || (D; E); D

			head2 = Head.load(std::memory_order_relaxed);
//			Head.load(head2, std::memory_order_relaxed);

//			assert( !(head.ptr==head2.ptr && head.count==head2.count && head.ptr != tail.ptr) || next.ptr);
			
//			printf("[d] head2.ptr: %p\n", head2.ptr);
			if(head.ptr==head2.ptr && head.count==head2.count) {  // Count is needed to fix: E; E; D || (D; E); D
				if(head.ptr == tail.ptr) {
					if(next.ptr <= 0) return false;
//					printf("[d] 5 __atomic_compare_exchange(%p)\n", Tail);
					Tail.compare_exchange_strong(tail, NodePtr(next.ptr, tail.count+1), std::memory_order_relaxed);
				} else {
					assert(next.ptr > 0);
					Node* nextptr = toPtr(next.ptr);
					data = nextptr->data;
//					printf("data: %u @ %p\n", data, &next.ptr->data);
					assert(data);
					assert(data<1000);
//					printf("[d] 6 __atomic_compare_exchange(%p)\n", Head);
					if(Head.compare_exchange_weak(head, NodePtr(next.ptr, head.count+1), std::memory_order_relaxed)) {
						assert(data && data<1000);
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

MSQ msq;

void* E(intptr_t data) {
	msq.enqueue(data, data+1);
	return nullptr;
}

void* E1(void*) {
	msq.enqueue(1, 2);
	return nullptr;
}

void* E2(void*) {
	msq.enqueue(2, 3);
	return nullptr;
}

void* E3(void*) {
	msq.enqueue(3, 4);
	return nullptr;
}

void* D(void*) {
	int data;
	while(!msq.dequeue(data));
	//bool b = msq.dequeue(data);
	//llmc_assume( b );
//	std::atomic_thread_fence(std::memory_order_seq_cst);
	return (void*)(intptr_t)data;
}

int main(int argc, char** argv) {
// 	for(Node& n: globalMemory) {
// 		assert(n.next.ptr==nullptr);
// 		assert(n.next.count==0);
// 	}
// 	assert(msq.Tail.load().ptr!=nullptr);
	pthread_t e1, d1;
	pthread_t e2, d2;
	pthread_t e3, d3;
	intptr_t data[3];
	
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	
// 	assert(msq.Head.ptr!=nullptr);
	//assert(msq.Head.ptr!=msq.Tail.ptr);

//	llmc_barrier_seq_cst();
	
//	llmc_atomic_begin();
 	pthread_create(&e1, 0, &E1, 0);
 	pthread_create(&e2, 0, &E2, 0);
 	pthread_create(&e3, 0, &E3, 0);

	
 	pthread_create(&d1, 0, &D, 0);
// 	pthread_create(&d2, 0, &D, 0);
//	data[0] = (intptr_t)D(nullptr);
	
	data[0] = (intptr_t)D(nullptr);
	pthread_join(e1, nullptr);
	pthread_join(e2, nullptr);
	pthread_join(e3, nullptr);
	pthread_join(d1, (void**)&data[1]);
//	pthread_join(d2, (void**)&data[2]);
//	assert(0);
// 	pthread_join(e2, nullptr);
//	data[0] = (intptr_t)D(nullptr);
//	data[1] = (intptr_t)D(nullptr);
//	assert(data[0] == 3);

//	pthread_create(&e1, 0, &E1, nullptr);
// 	pthread_join(e1, nullptr);
//	E(2);
//	E(2);
	//pthread_create(&d1, 0, &D, 0);
//	data[1] = (intptr_t)D(nullptr);
	//data[2] = (intptr_t)D(nullptr);
//	data[2] = (intptr_t)D(nullptr);

//	assert(0);
	
	//E(2);
	//pthread_join(d1, (void**)&data[1]);
//	pthread_create(&e3, 0, &E3, 0);
//	pthread_create(&d2, 0, &D, 0);
//	pthread_join(e1, nullptr);
//	pthread_join(e3, nullptr);
//	pthread_join(d2, (void**)&data[2]);
//	assert(data[0]+data[1]+data[2]==6);
//	assert(data[2]);
//	assert(data[2] != data[1]);
//	assert(data[2] != data[0]);
//	data[1] = (intptr_t)D(nullptr);

//	data[1] = (intptr_t)D(nullptr);
//	llmc_atomic_end();
//	pthread_join(e2, nullptr);

//	assert(data[0]==1 || data[0]==2);
//	data[1] = (intptr_t)D(nullptr);
	
//	printf("0:%zu 1:%zu\n", data[0], data[1]);
// 	assert( (data[0]==1 && data[1]==3) 
// 	     || (data[0]==3 && data[1]==1) 
// 	);
 	assert( (data[0]==1 && data[1]==2)
 	     || (data[0]==1 && data[1]==3)
 	     || (data[0]==2 && data[1]==1)
 	     || (data[0]==2 && data[1]==3)
 	     || (data[0]==3 && data[1]==1)
 	     || (data[0]==3 && data[1]==2)
 	);

}
extern "C" {

void llmc_memcpy8(uint8_t* dst, uint8_t* src, int bytes) {
	uint8_t* end = dst+bytes;
	while(dst<end) *dst++ = *src++;
}

void llmc_memcpy64(uint64_t* dst, uint64_t* src, int bytes) {
	uint64_t* end = dst+bytes/sizeof(uint64_t);
	while(dst<end) *dst++ = *src++;
}

//bool llmc_atomic_compare_exchange64(uint64_t* dst, uint64_t* src, int bytes

}

