#pragma once

template <typename T>
class ObjectPool {
private:
    struct Node {
        T data;
        Node* next;
    };
    
    Node* freeList;
    int totalAllocated;
    int maxPreallocated;
    
public:
    ObjectPool() : freeList(nullptr), totalAllocated(0), maxPreallocated(0) {}
    
    ~ObjectPool() {
        Node* current = freeList;
        while (current) {
            Node* temp = current;
            current = current->next;
            delete temp;
        }
    }
    
    T* allocate() {
        if (freeList) {
            Node* node = freeList;
            freeList = freeList->next;
            T* obj = &node->data;
            new (obj) T();
            return obj;
        }
        totalAllocated++;
        if (totalAllocated > maxPreallocated) maxPreallocated = totalAllocated;
        Node* newNode = new Node();
        newNode->next = nullptr;
        T* obj = &newNode->data;
        new (obj) T();
        return obj;
    }
    
    void deallocate(T* obj) {
        if (!obj) return;
        obj->~T();
        Node* node = reinterpret_cast<Node*>(obj);
        node->next = freeList;
        freeList = node;
    }
    
    int free_count() const {
        int count = 0;
        Node* current = freeList;
        while (current) {
            count++;
            current = current->next;
        }
        return count;
    }
    
    int total_count() const {
        return totalAllocated;
    }
    
    int max_count() const {
        return maxPreallocated;
    }
};
