
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

/**
 * Generic node structure used for tree in data model
 */
typedef struct Node
{
  char *name;			/**< Node name */
  char  *value;			/**< Data used by leaf node */
  char *path;			/**< Node path */
  struct Node *parent;	/**< Parent node */
  struct Node *prev_sibling;	/**< Prefix sibling node */
  struct Node *next_sibling;	/**< Next sibling node */
  struct Node *first_child;	/**< First children node */
  struct Node *last_child;	/**< Last children node */
}Node;

typedef void	(*NodeForeach)(Node *node);

void node_recursive_foreach(Node *node,NodeForeach func);
void node_recursive_free(Node * node);
void node_recursive_print(Node * node);
int node_insert_horizontal(Node *before,Node *node,Node *after);
int node_insert_vertical(Node *above,Node *node,Node *below,int append);
int node_insert_before(Node *parent,Node *sibling,Node *node);
int node_insert_after(Node *parent,Node *sibling,Node *node);
int node_prepend (Node *parent,Node *node);
int node_append(Node *parent, Node *node);

Node *node_new(const char *path,const char *name, const char *value);

void node_recursive_xml_write(FILE * fd,Node * node);
void node_recursive_json_write(FILE * fd,Node * node);
void node_recursive_ini_write(FILE * fd,Node * node);

#define safe_free(s)  if(s!=NULL) free(s)
#define safe_strdup(s) (s!=NULL ? strdup(s) : NULL)
#define safe_null(s) (((s)!=NULL&&(s)[0]!='\0')?(s):NULL)

#endif

