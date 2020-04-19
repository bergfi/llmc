#pragma once

template<typename EXPLORATION_STRATEGY, typename Model, typename STORAGE, template<typename,typename> typename LISTENER>
class mc {
public:
    using Storage = STORAGE;
    using StateSlot = typename Storage::StateSlot;
    using StateTypeID = typename Storage::StateTypeID;
    using StateID = typename Storage::StateID;
    using Delta = typename Storage::Delta;
    using MultiDelta = typename Storage::MultiDelta;
    using FullState = typename Storage::FullState;
    using InsertedState = typename Storage::InsertedState;
    using Listener = LISTENER<MultiCoreModelChecker, Model>;

    void go() {
        Context ctx(this, this->_m, 0);

        _storage.init();
        _m->init(&ctx);
        StateID init = this->_m->getInitial(&ctx).getData();
        System::Timer timer;

        stateQueueNew.push_back(init);
        _centralStateEnqueues.fetch_add(1, std::memory_order_relaxed);
        _states++;

        printf("first phase...\n");
        nonWaiters = 1;
        quit = false;

        workerThread<16>((void*)&ctx);

        double threadElapsed = 0;

        bool fireThreads = !ctx.stateQueueNew.empty();

        if(fireThreads) {

            printf("first phase resulted in %zu states and %zu transitions\n", ctx.localStates, ctx.localTransitions);

            _states.fetch_add(ctx.localStates, std::memory_order_relaxed);
            _transitions.fetch_add(ctx.localTransitions, std::memory_order_relaxed);

            ctx.localStates = 0;
            ctx.localTransitions = 0;

            if(_threads == 0) {
                _threads = System::getNumberOfAvailableCores();
            }
            _workers.reserve(_threads);
            _workerContexts.reserve(_threads);
            printf("going (%zu)...\n", _threads);
            nonWaiters = _threads;
            quit = false;
            stateQueueNew.swap(ctx.stateQueueNew);

            // Fire up all threads
            for(size_t tid = 0; tid < _threads; ++tid) {
                cpuset set = cpuset::createForCPU(tid);
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setaffinity_np(&attr, set._size, set._mask);
                _workerContexts.emplace_back(this, this->_m, tid);
                pthread_create(&_workers[tid], &attr, (void* (*)(void*)) workerThread<0>,
                               (void*) &_workerContexts[tid]);
            }

            // Wait for all threads to finish
            for(size_t tid = 0; tid < _threads; ++tid) {
                pthread_join(_workers[tid], nullptr);
            }
        }
        auto elapsed = timer.getElapsedSeconds();

        size_t stateBytesInsertedIntoStorage = 0;
        if(fireThreads) {
            for(size_t tid=_threads; tid--;) {
                if(!_workerContexts[tid].stateQueueNew.empty()) {
                    printf("State Queue %zu still contains states: %u\n", tid, _workerContexts[tid].stateQueueNew.size());
                }
                if(_workerContexts[tid].localStates != 0) {
                    printf("State Queue %zu still contains states count: %u\n", tid, _workerContexts[tid].localStates);
                }
                if(_workerContexts[tid].localTransitions != 0) {
                    printf("State Queue %zu still contains transitions count: %u\n", tid, _workerContexts[tid].localTransitions);
                }
                threadElapsed += _workerContexts[tid].elapsedSeconds;
                stateBytesInsertedIntoStorage += _workerContexts[tid].stateBytesInsertedIntoStorage;
            }
        } else {
            stateBytesInsertedIntoStorage = ctx.stateBytesInsertedIntoStorage;
            _threads = 1;
            threadElapsed = elapsed;
        }

        printf("Stopped after %.2lfs\n", elapsed); fflush(stdout);
        printf("Found %zu states, explored %zu transitions\n", _states.load(std::memory_order_acquire), _transitions.load(std::memory_order_acquire));
        printf("States/s: %lf\n", ((double)_states.load(std::memory_order_acquire))/elapsed);
        printf("Transitions/s: %lf\n", ((double)_transitions.load(std::memory_order_acquire))/elapsed);
        printf("Thread efficiency: %3.3lf%%\n", 100 * threadElapsed / (_threads * elapsed));
        printf("Central enqueues: %zu (%3.3lf%%)\n", _centralStateEnqueues.load(std::memory_order_relaxed), (double)100 * _centralStateEnqueues.load(std::memory_order_relaxed) / _states);
        printf("Central dequeues: %zu (%3.3lf%%)\n", _centralStateDequeues.load(std::memory_order_relaxed), (double)100 * _centralStateDequeues.load(std::memory_order_relaxed) / _states);
        printf("Storage Stats:\n");
        //Stats
        auto stats = _storage.getStatistics();
        _storage.printStats();
        printf("Inserted vector-data in bytes: %zu\n", stateBytesInsertedIntoStorage);
        printf("Compression ratio (%zu used bytes): %lf\n", stats._bytesInUse, (double)stateBytesInsertedIntoStorage / (stats._bytesInUse));
        printf("Compression ratio (%zu reserved bytes): %lf\n", stats._bytesReserved, (double)stateBytesInsertedIntoStorage / (stats._bytesReserved));
    }
};