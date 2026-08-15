#include <iostream>
#include <vector>

// Definition for singly-linked list.
struct ListNode {
    int val;
    ListNode *next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}
};

class Solution {
public:
    ListNode* mergeTwoLists(ListNode* list1, ListNode* list2) {
        ListNode * t1= list1;
        while(list1 != NULL){
            if (list1->val <= list2->val && list1->next->val >= list2->val){

                list2->next = list1->next;
                list1->next = list2;
                
            }
            list1= list1->next;
        }
        return t1;
    }
};

// Helper function to create a linked list from a vector
ListNode* createList(const std::vector<int>& values) {
    ListNode dummy(0);
    ListNode* curr = &dummy;
    for (int val : values) {
        curr->next = new ListNode(val);
        curr = curr->next;
    }
    return dummy.next;
}

// Helper function to print a linked list
void printList(ListNode* head) {
    while (head != nullptr) {
        std::cout << head->val << (head->next != nullptr ? " -> " : "");
        head = head->next;
    }
    std::cout << " -> NULL\n";
}

// Helper function to free list memory
void freeList(ListNode* head) {
    while (head != nullptr) {
        ListNode* temp = head;
        head = head->next;
        delete temp;
    }
}

int main() {
    // 1. Create two sorted lists
    ListNode* list1 = createList({1, 2, 4});
    ListNode* list2 = createList({1, 3, 4});

    std::cout << "List 1: ";
    printList(list1);

    std::cout << "List 2: ";
    printList(list2);

    // 2. Merge lists
    Solution solver;
    ListNode* merged = solver.mergeTwoLists(list1, list2);

    std::cout << "Merged: ";
    printList(merged);

    // 3. Cleanup memory
    freeList(merged);

    return 0;
}