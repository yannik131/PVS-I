#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

typedef struct NODE Node;

//=============================================================================
//Stack
//=============================================================================

//The stack is used to remember the unvisited nodes when traversing the tree iteratively
//A dynamic array would've also been possible, but the stack is easier to use
typedef struct STACK_ELEMENT StackElement;

struct STACK_ELEMENT {
    Node* value;
    StackElement *next;
};

typedef struct Stack {
    StackElement* top;
} Stack;

Stack stack_create() {
    Stack stack;
    stack.top = NULL;

    return stack;
}

StackElement* stack_create_node(Node* value) {
    StackElement* element = (StackElement*)malloc(sizeof(StackElement));
    element->value = value;
    element->next = NULL;

    return element;
}

void stack_push(Stack* stack, Node* value) {
    if(stack->top == NULL) {
        stack->top = stack_create_node(value);
        return;
    }

    StackElement* element = stack_create_node(value);
    element->next = stack->top;
    stack->top = element;
}

Node* stack_pop(Stack* stack) {
    Node* value = stack->top->value;
    StackElement* top = stack->top;
    stack->top = stack->top->next;
    free(top);

    return value;
}

bool stack_empty(Stack stack) {
    return stack.top == NULL;
}

void stack_delete(Stack* stack) {
    while(!stack_empty(*stack)) {
        stack_pop(stack);
    }
}

//=============================================================================
//Tree
//=============================================================================

struct NODE {
    int key;
    Node* smaller_keys; 
    Node* larger_keys;
};

typedef struct { 
    Node* root;
} Tree;

Tree tree_create() {
    Tree tree;
    tree.root = NULL;

    return tree;
}

Node* node_create(int key) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->key = key;
    node->smaller_keys = NULL;
    node->larger_keys = NULL;

    return node;
}

//Helper function to find a suitable node to attach a new node with a given key to
Node* tree_get_candidate_node(Node *node, int key) {
    if(node->smaller_keys && key < node->key) {
        return tree_get_candidate_node(node->smaller_keys, key);
    }
    else if(node->larger_keys && key > node->key) {
        return tree_get_candidate_node(node->larger_keys, key);
    }
    else {
        return node;
    }
}

bool tree_find_key_recursive(Tree tree, int key) {
    return tree_get_candidate_node(tree.root, key)->key == key;
}

Node* tree_insert_key(Tree* tree, int key) {
    if(!tree->root) {
        tree->root = node_create(key);
        return tree->root;
    }
    Node* insertion_point = tree_get_candidate_node(tree->root, key);
    if(insertion_point->key == key) {
        return NULL;
    }
    Node* new_node = node_create(key);
    if(key < insertion_point->key) {
        insertion_point->smaller_keys = new_node;
    }
    else {
        insertion_point->larger_keys = new_node;
    }

    return new_node;
}

typedef struct {
    Node* current;
    Stack left_turns;
    Stack right_turns;
} TreeTraverser;

TreeTraverser tree_traverser_create(Node* node) {
    TreeTraverser t = {
        .current = node, 
        .left_turns = stack_create(),
        .right_turns = stack_create()
    };

    return t;
}

void tree_traverser_delete(TreeTraverser* tree_traverser) {
    stack_delete(&tree_traverser->left_turns);
    stack_delete(&tree_traverser->right_turns);
}

void move_to_next_level(TreeTraverser* t) {
    if(t->current->smaller_keys) {
        stack_push(&t->left_turns, t->current->smaller_keys);
    }
    if(t->current->larger_keys) {
        stack_push(&t->right_turns, t->current->larger_keys);
    }
    if(!stack_empty(t->left_turns)) {
        t->current = stack_pop(&t->left_turns);
    }
    else if(!stack_empty(t->right_turns)) {
        t->current = stack_pop(&t->right_turns);
    }
    else {
        t->current = NULL;
    }
}

bool tree_traverser_end_reached(TreeTraverser t) {
    return t.current == NULL;
}

Node* tree_find_key_iterative(Tree tree, int key) {
    TreeTraverser t = tree_traverser_create(tree.root);
    while(!tree_traverser_end_reached(t)) {
        if(t.current->key == key) {
            tree_traverser_delete(&t);
            return t.current;
        }
        move_to_next_level(&t);
    };
    
    return NULL;
}

void _get_min_max(Node* node, int* min, int* max) {
    *min = INT_MAX;
    *max = INT_MIN;
    
    TreeTraverser t = tree_traverser_create(node);
    while(!tree_traverser_end_reached(t)) {
        if(t.current->key < *min) {
            *min = t.current->key;
        }
        if(t.current->key > *max) {
            *max = t.current->key;
        }
        move_to_next_level(&t);
    } 
}

bool _is_valid(Node node) {
    if(!node.smaller_keys && !node.larger_keys) {
        return true;
    }

    if(node.smaller_keys) {
        int left_min, left_max;
        _get_min_max(node.smaller_keys, &left_min, &left_max);
        if(left_max >= node.key) {
            return false;
        }
    }
    if(node.larger_keys) {
        int right_min, right_max;
        _get_min_max(node.larger_keys, &right_min, &right_max);
        if(right_min <= node.key) {
            return false;
        }
    }

    return true;
}

bool tree_is_valid(Tree tree) {
    TreeTraverser t = tree_traverser_create(tree.root);
    while(!tree_traverser_end_reached(t)) {
        if(!_is_valid(*t.current)) {
            tree_traverser_delete(&t);
            return false;
        }
        move_to_next_level(&t);
    }

    return true;
}

Tree tree_deep_copy(Tree tree) {
    Tree copy = tree_create();
    
    TreeTraverser t = tree_traverser_create(tree.root);
    while(!tree_traverser_end_reached(t)) {
        tree_insert_key(&copy, t.current->key);
        move_to_next_level(&t);
    }

    return copy;
}

void tree_delete(Tree* tree) {
    TreeTraverser t = tree_traverser_create(tree->root);
    while(!tree_traverser_end_reached(t)) {
        Node* to_delete = t.current;
        move_to_next_level(&t);
        free(to_delete);
    }
    tree->root = NULL;
}

int main() {
    Tree tree = tree_create();
    assert(tree_is_valid(tree));

    const int SIZE = 50;

    Node* nodes[SIZE];
    for(int i = 0; i < SIZE; ++i) {
        int value = rand() % SIZE*10;
        if(tree_find_key_iterative(tree, value)) {
            --i;
            continue;
        }
        nodes[i] = tree_insert_key(&tree, value);
    }
    tree_is_valid(tree);

    Tree copy = tree_deep_copy(tree);
    for(int i = 0; i < SIZE; ++i) {
        assert(tree_find_key_iterative(tree, nodes[i]->key));
        assert(tree_find_key_recursive(tree, nodes[i]->key));
        assert(tree_find_key_iterative(copy, nodes[i]->key));
    }

    tree_delete(&copy);
    for(int i = 0; i < SIZE; ++i) {
        assert(tree_find_key_iterative(tree, nodes[i]->key));
        assert(!tree_find_key_iterative(copy, nodes[i]->key));
    }
    assert(tree_is_valid(tree));

    printf("All tests passed!\n");

    return 0;
}