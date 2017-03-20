/*
tree -- by larkguo@gmail.com

1.Compile:
	gcc tree.c -o tree -lpthread

2.Run:
	[/]:
	/BBF=(null)
	/UPnP=(null)
	/UPnP/DM=(null)
	/UPnP/DM/Enable=1
	/UPnP/DM/Status=OK

	[/UPnP]:
	/UPnP/DM/Enable=1
	/UPnP/DM/Status=OK

	[/UPnP/DM/Status]:
	Status=OK

*/
#include "tree.h"

/************************** global **************************/
#define ROOT_PATH ("/")
#define PATH_BUFFER_SIZE (1024)

Node *g_tree=NULL;
pthread_mutex_t g_tree_mutex;

static char g_path[PATH_BUFFER_SIZE]={0};

/************************** node **************************/
Node*
node_new(const char *path,const char *name,const char *value)
{
	Node *node = (Node *) calloc(1,sizeof(Node));
	if( NULL == node ) return NULL;
	node->name = safe_strdup(name);	
	node->value = safe_strdup(value);
	node->path = safe_strdup(path);
	
	return node;
}

Node*
node_set(Node *node,const char *path,const char *name,const char *value)
{
	if( NULL == node ) return NULL;
	if(node->path){
		safe_free(node->path );
		node->path = safe_strdup(path);
	}
	if(node->name){
		safe_free(node->name );
		node->name = safe_strdup(name);
	}
	if(node->value){
		safe_free(node->value );
		node->value = safe_strdup(value);
	}
	
	return node;
}

/**
* @fn void node_recursive_free(Node * node)
* @brief Free all child of a node.
*
* The node itself is not free. This is a recursive function.
* @param node
*/
void 
node_recursive_free(Node * node)
{
	Node *child = NULL;
	Node *sibling = NULL;
	if(NULL == node) 	return;
	
	child = node->first_child;

	/* node's children && brother */
	while( NULL != child){
		/* until leaf */
		node_recursive_free(child);

		/* save brother */
		sibling = child->next_sibling;
		
		/* node free */
		safe_free(child->value);
		child->value = NULL;
		safe_free(child->name);
		child->name = NULL;
		safe_free(child->path);
		child->path = NULL;
		safe_free(child);
		child = NULL;

		/* begin free brother's children  */
		child = sibling;
	}
	node->first_child = NULL;
}

int 
node_insert_horizontal(Node *before,Node *node,Node *after)
{
	/* node */
	node->next_sibling = after;
	node->prev_sibling = before;

	/* before */
	if( NULL != before){
		before->next_sibling = node;
	}

	/* after */
	if( NULL != after){
		after->prev_sibling = node;
	}
	return 0;
}

int 
node_insert_vertical(Node *above,Node *node,Node *below,int append)
{
	/* node */
	node->parent= above;
	node->first_child= below;

	/* above */
	if( NULL != above){
		if(append){
			above->last_child = node;
			if( NULL == above->first_child){
				above->first_child = node;
			}
		}else{
			above->first_child = node;
			if( NULL == above->last_child){
				above->last_child = node;
			}
		}
	}

	/* below */
	if( NULL != below){
		below->parent = node;
	}
	return 0;
}

/*
insert node before sibling
*/
int
node_insert_before(Node *parent,Node *sibling,Node *node)
{
	Node *before = NULL;
	Node *after = NULL;
	Node *above = parent;
	Node *below = NULL;
	
	if (parent == NULL) return -1;
	if (node == NULL) return -1;

	if( NULL != sibling){
		before = sibling->prev_sibling;
		after = sibling;
	}else{
		before = NULL;
		after = parent->first_child;
	}

	node_insert_horizontal(before,node,after);
	node_insert_vertical(above, node, below,0);
	return 0;
}

/*
insert node after sibling
*/
int
node_insert_after(Node *parent,Node *sibling,Node *node)
{
	Node *before = NULL;
	Node *after = NULL;
	Node *above = parent;
	Node *below = NULL;
	
	if (parent == NULL) return -1;
	if (node == NULL) return -1;

	if( NULL != sibling){
		before = sibling;
		after = sibling->next_sibling;
	}else{
		before = parent->last_child;
		after = NULL;
	}

	node_insert_horizontal(before,node,after);
	node_insert_vertical(above, node, below,1);
	return 0;
}

int
node_prepend (Node *parent,Node *node)
{
	if (parent == NULL) return -1;
	
	return node_insert_before(parent, parent->first_child,node);
}

int
node_append(Node *parent, Node *node)
{
	if (parent == NULL) return -1;
	
	return node_insert_after(parent,parent->last_child,node);
}


void 
node_recursive_print(Node * node)
{
	Node *sibling;
	Node *child = node->first_child;
	
	while( NULL != child){
		if (strlen(child->path) == strlen(ROOT_PATH)){
			printf("%s%s=%s\n",ROOT_PATH,child->name,child->value );
		}else{
			printf("%s/%s=%s\n",child->path,child->name,child->value );
		}
		
		node_recursive_print(child);
		
		sibling = child->next_sibling;
		child = sibling;
	}
}

void 
node_recursive_foreach(Node *node,NodeForeach func)
{
	Node *sibling = NULL;
	Node *child = NULL;
	
	if ( NULL == node ) return;
	if ( NULL == func )	return;
	
	child = node->first_child;
	while(child!=NULL)	{
		
		func(child);

		node_recursive_foreach(child,func);
		
		sibling = child->next_sibling;
		child = sibling;
	}
}

void 
node_recursive_xml_write(FILE * fd,Node * node)
{
	Node *sibling;
	Node *child = node->first_child;
  
	while( NULL != child){
		// <name> 
		if(NULL != child->name){
			fwrite("\n<", strlen("\n<"), 1, fd);
			fwrite(child->name,strlen(child->name),1,fd);
			fwrite(">", strlen(">"), 1, fd);
		}
		// value
		if(NULL != child->value){
			fwrite(child->value,strlen(child->value),1,fd);
		}
		
		node_recursive_xml_write(fd, child);

		// </name> 
		if(NULL != child->first_child){
			fwrite("\n", strlen("\n"), 1, fd);
		}
		if(NULL != child->name){
			fwrite("</", strlen("</"), 1, fd);
			fwrite(child->name,strlen(child->name),1,fd);
			fwrite(">", strlen(">"), 1, fd);
		}
		
		sibling = child->next_sibling;
		child = sibling;
	}
}

void 
node_recursive_json_write(FILE * fd,Node * node)
{
	Node *sibling;
	Node *child = node->first_child;

	while( NULL != child){
		
		if( NULL != child->first_child){
			if(NULL != child->name){
				fwrite("\n\"", strlen("\n\""), 1, fd);
				fwrite(child->name,strlen(child->name),1,fd);
				fwrite("\":", strlen("\":"), 1, fd);
			}
			fwrite("{", strlen("{"), 1, fd);	
			
		}else{
			if(NULL != child->name){
				fwrite("\n\"", strlen("\n\""), 1, fd);
				fwrite(child->name,strlen(child->name),1,fd);
				fwrite("\":", strlen("\":"), 1, fd);
			}

			if(NULL != child->value){
				fwrite("\"", strlen("\""), 1, fd);
				fwrite(child->value,strlen(child->value),1,fd);
				fwrite("\"", strlen("\""), 1, fd);
			}
			if(NULL != child->next_sibling){
				fwrite(",", strlen(","), 1, fd);
			}
		}
		
		node_recursive_json_write(fd, child);

		if( NULL != child->first_child){
			fwrite("\n}", strlen("\n}"), 1, fd);	
		}
		
		sibling = child->next_sibling;
		child = sibling;
	}
}

void 
node_recursive_ini_write(FILE *fd,Node *node)
{
	Node *sibling;
	Node *child = node->first_child;

	while( NULL != child){
		if(NULL==child->first_child && 0!=strcmp(ROOT_PATH,child->path)){
			fwrite("\n[", strlen("\n["), 1, fd);
			fwrite(child->path,strlen(child->path),1,fd);
			fwrite("]", strlen("]"), 1, fd);

			if(NULL != child->name){
				fwrite("\n", strlen("\n"), 1, fd);
				fwrite(child->name,strlen(child->name),1,fd);
				fwrite("=", strlen("="), 1, fd);
			}
			if(NULL != child->value){
				fwrite(child->value,strlen(child->value),1,fd);
			}
		}
				
		node_recursive_ini_write(fd, child);
		
		sibling = child->next_sibling;
		child = sibling;
	}
}

/************************** tree **************************/
void 
tree_foreach(Node *node,
			 NodeForeach  func)
{
	pthread_mutex_lock(&g_tree_mutex);
	if( NULL == node) 
		node_recursive_foreach(g_tree,func);
	else
		node_recursive_foreach(node,func);
	pthread_mutex_unlock(&g_tree_mutex);
}

/**
* @fn Node * tree_node_get(const char * path)
*
* @brief return return node from the given path.
*
* @param path to the node 
* @return node or null if path is bad
*
*/
Node *
tree_node_get(const char *path)
{  
	Node *ret = NULL;
	Node *current_parent;
	Node *current_node = NULL;
	char *current_node_name;
	
	if (path && (path[0]=='/') && (g_tree != NULL)) {
		if (path[1]=='\0') {
			ret = g_tree;
		} else {
			pthread_mutex_lock(&g_tree_mutex);
			
			strcpy(g_path,path);
			
			/* Set current parent node to root node */
			current_parent = g_tree;
			
			/* Get First node name to look for */
			current_node_name = strtok(g_path,"/");
			
			/* For each name in path */
			while(current_node_name!=NULL) {
				/* Search for node name in all node children */
				current_node = current_parent->first_child;
				while (current_node!=NULL) {
					if (strcmp(current_node->name, current_node_name)==0) {
						break;
					} else {
						current_node = current_node->next_sibling;
					}
				}
				
				/* Path provided does not exist */
				if (current_node==NULL) {
					printf("Path '%s' does not exist\n",path);
					break;
				}
				
				/* Get next name in path */
				current_parent = current_node;
				current_node_name = strtok(NULL,"/");
			}
			pthread_mutex_unlock(&g_tree_mutex);
			
			/* If end of path has been reached and match current node return node */
			if (current_node_name==NULL) {
				ret = current_node;
			}
		}
	}
	return ret;
}

void 
tree_print(Node *node)
{
	pthread_mutex_lock(&g_tree_mutex);
	
	if( NULL == node) {
		node_recursive_print(g_tree);
	}else{
		node_recursive_print(node);
	}
	
	pthread_mutex_unlock(&g_tree_mutex);
}

Node *
tree_child_add(Node *parent,const char *path,const char *name,const char *value)
{ 
	Node * node;
	Node * last_node = NULL;
	Node * child = NULL;

	pthread_mutex_lock(&g_tree_mutex);

	node = parent->first_child;
	
	/* Get last child */
	while(node!=NULL) {
		if (node->name!=NULL && name!=NULL) {
			/* Test if a child with the same name exist */
			if (strcmp(node->name,name)==0) {
				child = node;
				break;
			}
		}
		last_node = node;
		node = node->next_sibling;
	}

	/* If child not found, create a new one */
	if (child==NULL) {
		child = node_new(path,name,value);
		if( NULL== child) 	return NULL;
		node_append(parent,child);
		//node_prepend(parent,child);
	}
	else{
		node_set(child,path,name,value);
	}
	
	pthread_mutex_unlock(&g_tree_mutex);
	
	return child;
}

Node * 
tree_node_create(const char *path,const char *name,const char *value)
{
	Node * parent = NULL;
	Node * node = NULL;

	/* get parent node */
	parent = tree_node_get(path);
	if (parent != NULL) {
		node = tree_child_add(parent,path,name,value);
	}
	
	return node;
}

void 
tree_init()
{
	g_tree = (Node *) calloc(1,sizeof(Node));
	g_tree->name = safe_strdup(ROOT_PATH);
}

void 
tree_exit()
{
	node_recursive_free(g_tree);
	
	if (g_tree->name!=NULL);
	{
		safe_free(g_tree->name);
		g_tree->name = NULL;
	}
	
	safe_free(g_tree);
	g_tree = NULL;
}

void 
tree_xml_write(const char *file)
{  
	FILE * fd;
	
	fd = fopen(file,"w");
	if (fd==NULL) {
		printf("fopen error\n");
		return ;
	}

	pthread_mutex_lock(&g_tree_mutex);
	node_recursive_xml_write(fd,g_tree);
	fwrite("\n", strlen("\n"), 1, fd);
	pthread_mutex_unlock(&g_tree_mutex);

	if (fclose(fd)!=0) {
		printf("fclose error\n");
	}
	return;
}

void 
tree_json_write(const char *file)
{  
	FILE * fd;
	
	fd = fopen(file,"w");
	if (fd==NULL) {
		printf("fopen error\n");
		return ;
	}

	pthread_mutex_lock(&g_tree_mutex);
	fwrite("{", strlen("{"), 1, fd);
	node_recursive_json_write(fd,g_tree);
	fwrite("\n}\n", strlen("\n}\n"), 1, fd);
	pthread_mutex_unlock(&g_tree_mutex);

	if (fclose(fd)!=0) {
		printf("fclose error\n");
	}
	return;
}

void 
tree_ini_write(const char *file)
{  
	FILE * fd;
	
	fd = fopen(file,"w");
	if (fd==NULL) {
		printf("fopen error\n");
		return ;
	}
	pthread_mutex_lock(&g_tree_mutex);
	node_recursive_ini_write(fd,g_tree);
	fwrite("\n", strlen("\n"), 1, fd);
	pthread_mutex_unlock(&g_tree_mutex);

	if (fclose(fd)!=0) {
		printf("fclose error\n");
	}
	return;
}

/************************** main **************************/
#define UPNP_PATH "/UPnP"
#define UPNP_DM_PATH "/UPnP/DM"

void 
node_print (Node    *node)
{
	if (strlen(node->path) == strlen(ROOT_PATH)){
		printf("%s%s=%s\n",ROOT_PATH,node->name,node->value );
	}else{
		printf("%s/%s=%s\n", node->path, node->name,node->value );
	}
}

int main(int argc,char *argv[])
{
	Node *node=NULL;
	char path[]="/UPnP/DM/Status";
	
	tree_init();

	tree_node_create(ROOT_PATH, "BBF", NULL);
	tree_node_create(ROOT_PATH, "UPnP", NULL);
	node = tree_node_create(UPNP_PATH, "DM", NULL);
	tree_node_create(UPNP_DM_PATH,"Enable", "1");
	tree_node_create(UPNP_DM_PATH,"Status", "OK");

	printf("[%s]:\n",ROOT_PATH);
	tree_foreach(NULL, node_print);

	printf("\n[%s]:\n",node->path);
	tree_print(node);
	
	node = tree_node_get(path);
	printf("\n[%s]:\n%s=%s\n", path,node->name,node->value );
	
	tree_xml_write("config.xml");
	tree_json_write("config.json");
	tree_ini_write("config.ini");
	
	tree_exit();

	return 0;
}

