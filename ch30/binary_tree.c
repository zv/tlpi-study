/*
  Implement a set of thread-safe functions that update and search an unbalanced
  binary tree. This library should include functions (with the obvious purposes) of
  the following form:

    initialize(tree);
    add(tree, char *key, void *value);
    delete(tree, char *key)
    Boolean lookup(char *key, void **value)

  In the above prototypes, tree is a structure that points to the root of the tree (you
  will need to define a suitable structure for this purpose). Each element of the tree
  holds a key-value pair. You will also need to define the structure for each element
  to include a mutex that protects that element so that only one thread at a time can
  access it. The initialize(), add(), and lookup() functions are relatively simple to imple-
  ment. The delete() operation requires a little more effort.

  Removing the need to maintain a balanced tree greatly simplifies the locking
  requirements of the implementation, but carries the risk that certain patterns
  of input would result in a tree that performs poorly. Maintaining a balanced
  tree necessitates moving nodes between subtrees during the add() and delete()
  operations, which requires much more complex locking strategies.

*/

#include <stdint.h>
#include <pthread.h>
#include <search.h>
#include "../lib/tlpi_hdr.h"

// static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
// typedef __uint128_t Key;
typedef unsigned int Key;

typedef struct bt_node {
    Key key;
    int value;
    struct bt_node *left;
    struct bt_node *right;
    pthread_mutex_t mutex;
} bt_node;

typedef struct bt_root {
    bt_node *root;
    size_t node_count;
} bt_root;

// pthread_create arguments
typedef struct mt_node_arguments {
    Key key;
    int value;
    bt_root *root;
} mt_args;

int lock(bt_node *node) {
    int s = pthread_mutex_lock(&node->mutex);
    if (s != 0) { errExitEN(s, "pthread_mutex_lock" ); }
    return s;
}

int unlock(bt_node *node) {
    int s = pthread_mutex_unlock(&node->mutex);
    if (s != 0) { errExitEN(s, "pthread_mutex_unlock" ); }
    return s;
}

bt_root *initialize() {
    bt_root *tree = calloc(1, sizeof(bt_root));
    tree->node_count = 0;
    tree->root = NULL;
    return tree;
}

bt_node *create_node(Key key, int value) {
    bt_node *result = calloc(1, sizeof(bt_node));
    result->left  = NULL;
    result->right = NULL;
    result->key   = key;
    result->value = value;
    pthread_mutex_init(&result->mutex, NULL);
    return result;
}

bt_node *closest_leaf(bt_node *root, Key key) {
    bt_node *current = root;
    bt_node *prev = NULL;
    while (current != NULL) {
        prev = current;
        if      (key < current->key)  current = current->left;
        else if (key > current->key)  current = current->right;
        else                          return current;
    }

    return prev;
}

int add(bt_root *root_tree, Key key, int value) {
    bt_node *new_node = create_node(key, value);
    bt_node *current = root_tree->root;

    // we could have both threads think 'root' is NULL and replace both in sequence
    if (current == NULL) {
        root_tree->root = new_node;
        root_tree->node_count = 1;
        return 1;
    }

    lock(current);

      bt_node *leaf = closest_leaf(current, key);
      if      (key == leaf->key)  return 0;
      if      (key < leaf->key)   leaf->left = new_node;
      else if (key > leaf->key)   leaf->right = new_node;
      ++root_tree->node_count;

    unlock(current);

    return 1;
}

void replace_parent(bt_node *parent, bt_node *child, bt_node *new_child) {
    if (parent->left == child) {
        free(parent->left);
        parent->left = new_child;
    }

    if (parent->right == child) {
        free(parent->right);
        parent->right = new_child;
    }
}

void splice(bt_node *node, bt_node *parent) {
    // if it's a leaf, destroy it.
    if (!node->left && !node->right) {
             replace_parent(parent, node, NULL);

    // if it has one child, have them be adopted by their grandparent
    } else if (node->left && !node->right) {
            replace_parent(parent, node, node->left);
    } else if (node->right && !node->left) {
            replace_parent(parent, node, node->right);

    // if it has two children, take the smallest value of the right tree and replace the node with that one.
    } else if (node->right && node->left) {
        // find the smallest element of the right tree
        bt_node *smallest_parent = node;
        bt_node *smallest = node->right;
        while (smallest->left) {
            smallest_parent = smallest;
            // left is the smaller side
            smallest = smallest->left;
        }

        // "swap" the smallest.
        node->key = smallest->key;
        node->value = smallest->value;

        // unsure if this is correct -- seems to be only sensible thing to do
        smallest_parent->left = smallest->right;
        free(smallest);

    }
}

int delete_dfs(bt_node *node, bt_node *parent, Key key) {
    if (node == NULL) {
        return 0;
    }

    Key node_key = node->key;
    if (node_key == key) {
        if (parent) {
            splice(node, parent);
            return 1;
        } else {
            // delete the whole tree some other time
            return 0;
        }
    }

    if (key < node_key) {
        return delete_dfs(node->left, node, key);
    } else if (key > node_key) {
        return delete_dfs(node->right, node, key);
    }

    return -1;
}

void delete(bt_root *root_tree, Key key) {
    if (delete_dfs(root_tree->root, NULL, key)) root_tree->node_count--;
}

void breadth_first(bt_root *root_tree, void (*action)(const bt_node*)) {
    bt_node *current = root_tree->root;
    int count = root_tree->node_count;

    // stack of elements
    bt_node **stack = calloc(count, sizeof(bt_node*));

    int ith = 0, nth = 0;
    stack[0] = current;
    while(current != NULL) {
        if (current->left)  stack[++nth] = current->left;
        if (current->right) stack[++nth] = current->right;

        current = stack[++ith];
    }

    int i = 0;
    while (stack[i] != NULL)
        action(stack[i++]);
}

void depth_first_driver(bt_node *current, size_t depth, void (*action)(const bt_node*, int)) {
    if (!current) return;
    action(current, depth);
    if (!current->right && !current->left) return;
    depth_first_driver(current->right, depth + 1, action);
    depth_first_driver(current->left, depth + 1, action);
}

void depth_first(bt_root *root, void (*action)(const bt_node*, int)) {
    depth_first_driver(root->root, 0, action);
}

void print_node_depth(const bt_node *node, int depth) {
    printf("%c ", node->key);
    return;
    if (node != NULL) {
        printf("%*c \n", 3*depth, node->key);
    } else {
        printf("%*c \n", 3*depth, '%');
    }
}

void print_node(const bt_node *node) {
    if (node != NULL) {
        printf("node: %c\n", node->key);
    }
}

void *mt_add(void *arg) {
    mt_args *parameters = (mt_args*)arg;
    add(parameters->root, parameters->key, parameters->value);
    return NULL;
}

int main() {
    pthread_t threads[26];
    bt_root *root = initialize();
    add(root, 'M', 1);

    char letters[25] = {'G', 'A', 'Q', 'Z', 'J', 'C', 'L', 'U', 'P', 'V', 'T', 'O', 'E',
                        'X', 'N', 'S', 'R', 'W', 'B', 'F', 'H', 'I', 'D', 'K', 'Y'};

    for(int i = 0; i < 25; i++) {
        mt_args *args = alloca(1);
        args->key = letters[i]; args->value = i; args->root = root;
        int s = pthread_create(&threads[i], NULL, mt_add, args);
        if (s != 0) errExitEN(s, "pthread_create");
    }

    for(int i = 0; i < 25; i++) {
        int s = pthread_join(threads[i], NULL);
        if (s != 0) errExitEN(s, "pthread_join");
    }

    depth_first(root, &print_node_depth);
    printf("\nnumber: %zd\n", root->node_count);
    return 0;
}
