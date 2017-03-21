
#ifndef	__TREE_H__
#define __TREE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <pthread.h>
#else
#define g_tree_mutex
#define pthread_mutex_init(mutex,attr)
#define pthread_mutex_destroy(mutex) 
#define pthread_mutex_lock(mutex)
#define pthread_mutex_unlock(mutex) 
#define pthread_mutex_trylock(mutex) 
#define pthread_cond_init(cond, attr) 
#define pthread_cond_destroy(cond) 
#define pthread_cond_signal(cond)
#define pthread_cond_broadcast(cond) 
#define pthread_cond_wait(cond, mutex)
#define pthread_cond_timedwait(cond, mutex, time) 
#endif

#define safe_free(s)  if(s!=NULL) free(s)
#define safe_strdup(s) (s!=NULL ? strdup(s) : NULL)
#define safe_null(s) (((s)!=NULL&&(s)[0]!='\0')?(s):NULL)

/**
 * Generic node structure used for tree

[horizontal]: 
prev - node - next

[vertical]:
 parent 
  |
 node
 |   |
first last 
 */
typedef struct Node
{
  char *name;			/**< Node name */
  char *value;			/**< Data used by leaf node */
  char *path;			/**< Node path */
  struct Node *parent;	/**< Parent node */
  struct Node *prev_sibling;	/**< Prefix sibling node */
  struct Node *next_sibling;	/**< Next sibling node */
  struct Node *first_child;	/**< First children node */
  struct Node *last_child;	/**< Last children node */
}Node;

typedef void	(*NodeForeach)(Node *node);

Node *node_new(const char *path,const char *name,const char *value);
Node *node_set(Node *node,const char *path,const char *name,const char *value);
void node_free_recursive(Node *parent);
void node_free(Node *parent);
void node_print_recursive(Node *parent);
void node_foreach_recursive(Node *parent, NodeForeach func);
void node_xml_write_recursive(FILE * fd,Node *parent);
void node_json_write_recursive(FILE * fd,Node *parent);
void node_ini_write_recursive(FILE * fd,Node *parent);
int node_insert_horizontal(Node *prev,Node *node,Node *next);
int node_insert_vertical(Node *parent,Node *node,Node *child,int last);
int node_insert_before(Node *parent,Node *sibling,Node *node);
int node_insert_after(Node *parent,Node *sibling,Node *node);
int node_prepend (Node *parent,Node *node);
int node_append(Node *parent, Node *node);

Node *tree_node_get(const char *path);
Node *tree_child_add(Node *parent,const char *path,const char *name,const char *value);
Node *tree_node_create(const char *path,const char *name,const char *value);
void tree_foreach(Node *parent, NodeForeach func);
void tree_print(Node *parent);
void tree_init();
void tree_exit();
void tree_xml_write(const char *file);
void tree_json_write(const char *file);
void tree_ini_write(const char *file);

#endif

