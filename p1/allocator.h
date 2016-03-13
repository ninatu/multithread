#include <stdexcept>
#include <string>
#include <list>
#include <iostream>
#include <algorithm>
#include <cstring>

using namespace std;

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class AllocError: std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    AllocErrorType getType() const { return type; }
};

class Allocator;
class MemoryBlock;

class MemoryBlock {
    void *ptr_memory;
    size_t size;
    bool free;

public :
    MemoryBlock(void *_ptr_memory, size_t size_, bool free_=true):
        ptr_memory(_ptr_memory), size(size_), free(free_) {}
    void *get_ptr() const;
    friend Allocator;
};

class Pointer {
    MemoryBlock *ptr_block;
public:
    Pointer(MemoryBlock *ptr_block_=nullptr) :
        ptr_block(ptr_block_) {}
    void *get() const;
    friend Allocator;
};

class Allocator {
    bool ping_pong = false;
    void *ptr_memory;
    size_t size_memory;

    void *ptr_memory_part1;
    void *ptr_memory_part2;
    size_t size_memory_part;

    list< MemoryBlock *> list_blocks;
    size_t size_free_memory;

    void joinfree(list<MemoryBlock *>::iterator iter);
    void appendfollow(list<MemoryBlock *>::iterator iter);
    list<MemoryBlock *>::iterator findblock(size_t N);
    list<MemoryBlock *>::iterator shortSizeBlock(list<MemoryBlock *>::iterator iter_block, size_t N);
    void defragWithLast(list<MemoryBlock *>::iterator iter_block);
public:
    Allocator(void *base, size_t size);
    Pointer alloc(size_t N);
    void realloc(Pointer &p, size_t N);
    void free(Pointer &p);
    void defrag();
    void print() {
        for(auto iter = list_blocks.begin(); iter != list_blocks.end(); iter++) {
            cout << (*iter)->free << ' ' <<  (*iter)->size << ' ' << (*iter)->ptr_memory<<  endl;
        }
        cout << endl;
    }

    std::string dump() { return ""; }
};






