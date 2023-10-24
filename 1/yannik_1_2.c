#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef struct NODE Node;

struct NODE {
    int key;
    Node* smaller_keys; 
    Node* larger_keys;
};

typedef struct { 
    Node* root;
} Tree;

Tree* tree_create() {

}

void tree_insert_key(Tree* tree, int key) {

}

bool tree_find_key_recursive(Tree* tree, int key) {

}

bool tree_find_key_iterative(Tree* tree, int key) {

}

bool tree_is_valid(Tree* tree) {

}

Tree* tree_deep_copy(Tree* tree) {

}

void tree_delete(Tree* tree) {

}

Node** tree_get_nodes(Tree* tree, int size) {

}

int main() {
    Tree* tree = tree_create();
    assert(tree_is_valid(tree));

    const int SIZE = 10;
    int values[SIZE];
    for(int i = 0; i < SIZE; ++i) {
        int value = rand() % 100;
        tree_insert_key(tree, value);
        values[i] = value;
    }
    assert(tree_is_valid(tree));

    Tree* copy = tree_deep_copy(tree);
    for(int i = 0; i < SIZE; ++i) {
        assert(tree_find_key_iterative(tree, values[i]));
        assert(tree_find_key_recursive(tree, values[i]));
        assert(tree_find_key_iterative(copy, values[i]));
    }

    Node** nodes_copy = tree_get_nodes(copy, SIZE);
    Node** nodes_tree = tree_get_nodes(tree, SIZE);
    tree_delete(copy);
    for(int i = 0; i < SIZE; ++i) {
        assert(nodes_copy[i] == NULL);
        assert(nodes_tree[i] != NULL);
    }
    assert(tree_is_valid(tree));
}
