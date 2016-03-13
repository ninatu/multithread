#include "allocator.h"

void *MemoryBlock::get_ptr() const {
    return ptr_memory;
}

void *Pointer::get() const {
    if (ptr_block != nullptr)
        return ptr_block->get_ptr();
    else
        return nullptr;
}

Allocator::Allocator(void *base, size_t size)
{
    ptr_memory = base;
    size_memory = size;

    ping_pong = false;
    ptr_memory_part1 = base;
    size_memory_part = size / 2;
    ptr_memory_part2 = (char *)ptr_memory + size_memory_part;
    size_free_memory = size_memory_part;

    list_blocks.push_back(new MemoryBlock(ptr_memory_part1, size_memory_part, true));
}


list<MemoryBlock *>::iterator Allocator::findblock(size_t N) {
    auto iter = list_blocks.begin();
    for(;iter != list_blocks.end(); iter++) {
        if ((*iter)->free == true && (*iter)->size >= N)
            return iter;
    }
    return iter;
}

Pointer Allocator::alloc(size_t N) {
    if (N > size_free_memory)
        throw AllocError(AllocErrorType::NoMemory, string("ERROR in alloc, no memory!"));
    auto iter = findblock(N);
    if (iter == list_blocks.end()) {
        this->defrag();
        iter = findblock(N);
    }

    if ((*iter)->size == N) {
        (*iter)->free = false;
        size_free_memory -= N;
        return Pointer((*iter));
    } else {
        void *ptr_memory = (*iter)->ptr_memory;
        (*iter)->size -= N ;
        (*iter)->ptr_memory = (char *)((*iter)->ptr_memory) + N;
        MemoryBlock *new_block = new MemoryBlock(ptr_memory, N, false);
        list_blocks.insert(iter, new_block);
        size_free_memory -= N;
        return Pointer(new_block);
    }
}

void Allocator::appendfollow(list<MemoryBlock *>::iterator iter) {
    iter++;
    size_t size_block = (*iter)->size;
    delete (*iter);
    iter = list_blocks.erase(iter);
    iter--;
    (*iter)->size += size_block;
}

void Allocator::joinfree(list<MemoryBlock *>::iterator iter) {
    // check before element
    if (iter != list_blocks.begin()) {
        iter--;
        if ((*iter)->free == true) {
            appendfollow(iter);
        }
        else {
            iter++;
        }
    }
    // check follow element
    iter++;
    if (iter != list_blocks.end() && (*iter)->free) {
        iter--;
        appendfollow(iter);
    }
    return;
}

void Allocator::free(Pointer &p) {
    if (p.ptr_block == nullptr)
        throw AllocError(AllocErrorType::InvalidFree, string("ERROR in free, pointer is NULL"));
    MemoryBlock *ptr_block = p.ptr_block;
    size_t size_block = ptr_block->size;
    ptr_block->free = true;
    auto iter_block = find(list_blocks.begin(), list_blocks.end(), ptr_block);
    joinfree(iter_block);
    size_free_memory += size_block;
    p.ptr_block = nullptr;
}

list<MemoryBlock *>::iterator Allocator::shortSizeBlock(list<MemoryBlock *>::iterator iter_block, size_t N) {
    void *ptr_memory = (*iter_block)->ptr_memory;
    size_t size_memory = (*iter_block)->size;
    bool is_free = (*iter_block)->free;
    (*iter_block)->size = size_memory - N ;
    (*iter_block)->ptr_memory = (char *)((*iter_block)->ptr_memory) + N;
    (*iter_block)->free = true;
    MemoryBlock *new_block = new MemoryBlock(ptr_memory, N, is_free);
    list_blocks.insert(iter_block, new_block);
    iter_block--;
    return iter_block;
}

void Allocator::defrag() {
    list<MemoryBlock *> new_list_blocks;
    void *cur_ptr_data;
    if (ping_pong)
        cur_ptr_data = ptr_memory_part1;
    else
        cur_ptr_data = ptr_memory_part2;
    MemoryBlock *cur_block;
    for(auto iter = list_blocks.begin(); iter != list_blocks.end(); iter++) {
        cur_block = *iter;
        if (cur_block->free == false) {
            cur_block = *iter;
            memcpy(cur_ptr_data, cur_block->ptr_memory, cur_block->size);
            cur_block->ptr_memory = cur_ptr_data;
            cur_ptr_data = (char *)cur_ptr_data + cur_block->size;
            new_list_blocks.push_back(cur_block);
        } else {
            delete cur_block;
        }
    }
    new_list_blocks.push_back(new MemoryBlock(cur_ptr_data, size_free_memory, true));
    ping_pong = !ping_pong;
    list_blocks.clear();
    list_blocks = new_list_blocks;
}


void Allocator::defragWithLast(list<MemoryBlock *>::iterator iter_block) {
    list<MemoryBlock *> new_list_blocks;
    void *cur_ptr_data;
    if (ping_pong)
        cur_ptr_data = ptr_memory_part1;
    else
        cur_ptr_data = ptr_memory_part2;
    MemoryBlock *cur_block;
    for(auto iter = list_blocks.begin(); iter != list_blocks.end(); iter++) {
        if(iter == iter_block)
            continue;
        cur_block = *iter;
        if (cur_block->free == false) {
            cur_block = *iter;
            memcpy(cur_ptr_data, cur_block->ptr_memory, cur_block->size);
            cur_block->ptr_memory = cur_ptr_data;
            cur_ptr_data = (char *)cur_ptr_data + cur_block->size;
            new_list_blocks.push_back(cur_block);
        } else {
            delete cur_block;
        }
    }
    cur_block = *iter_block;
    memcpy(cur_ptr_data, cur_block->ptr_memory, cur_block->size);
    cur_block->ptr_memory = cur_ptr_data;
    cur_ptr_data = (char *)cur_ptr_data + cur_block->size;
    new_list_blocks.push_back(cur_block);

    new_list_blocks.push_back(new MemoryBlock(cur_ptr_data, size_free_memory, true));
    ping_pong = !ping_pong;
    list_blocks.clear();
    list_blocks = new_list_blocks;
}

void Allocator::realloc(Pointer &p, size_t N) {
    if (p.ptr_block == nullptr) {
        p = this->alloc(N);
        return;
    }
    MemoryBlock *ptr_block = p.ptr_block;
    if (ptr_block->size == N)
        return;
    if (ptr_block->size > N) {
        auto iter_block = find(list_blocks.begin(), list_blocks.end(), ptr_block);
        void *new_ptr_data = ((char *)ptr_block->ptr_memory + N);
        size_t new_size = ptr_block->size - N;
        MemoryBlock *new_block = new MemoryBlock(new_ptr_data, new_size, true);
        ptr_block->size = N;
        if (iter_block != list_blocks.end()) {
            iter_block++;
            list_blocks.insert(iter_block, new_block);
            iter_block--;
        }
        else {
            list_blocks.push_back(new_block);
            iter_block++;
        }
        joinfree(iter_block);
        size_free_memory += new_size;
        return;
    } else {
        size_t delta_size = N - ptr_block->size;
        if (delta_size > size_free_memory)
            throw AllocError(AllocErrorType::NoMemory, string("ERROR in realloc, no memory!"));
        auto iter_block = find(list_blocks.begin(), list_blocks.end(), ptr_block);
        if (size_free_memory < N) {
            defragWithLast(iter_block);
            iter_block = find(list_blocks.begin(), list_blocks.end(), ptr_block);
        }
        iter_block++;
        if(iter_block != list_blocks.end() && (*iter_block)->free == true && (*iter_block)->size >= delta_size) {
            if ((*iter_block)->size != delta_size) {
                iter_block = shortSizeBlock(iter_block, delta_size);
            }
            iter_block--;
            appendfollow(iter_block);
        } else  {
            auto find_iter = findblock(N);
            if (find_iter == list_blocks.end()) {
                this->defrag();
                this->print();
                find_iter = findblock(N);
            }

            if ((*find_iter)->size != N)  {
                find_iter = shortSizeBlock(find_iter, N);
            }
            iter_block = find(list_blocks.begin(), list_blocks.end(), ptr_block);
            MemoryBlock *ptr_find_block = *find_iter;
            void *ptr_find_memory =  ptr_find_block->ptr_memory;
            delete ptr_find_block;
            size_t prev_size = ptr_block->size;
            void *prev_ptr_memory = ptr_block->ptr_memory;
            memcpy(ptr_find_memory, ptr_block->ptr_memory, ptr_block->size);
           // list_blocks.erase(iter_block);
            (*find_iter) = ptr_block;
            ptr_block->ptr_memory = ptr_find_memory;
            ptr_block->size = N;
            (*iter_block) = new MemoryBlock(prev_ptr_memory, prev_size, true);
            joinfree(iter_block);

        }
        size_free_memory -= delta_size;
    }
}
