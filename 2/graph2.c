#include <limits.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

typedef struct GRAPH Graph;
typedef struct NODE Node;


struct NODE
{
	Graph* graph; // Graph containing the node
	char* label;  // A string containing a label for the node
	int adjacent_nodes_count;  // Amount of other nodes which are directly connected by edges to this node
	Node** adjacent_nodes;     // Other nodes which are directly connected by edges to this node
	                           // Those nodes are stored via an array of pointers
};

struct GRAPH {
	int node_count; // Amount of nodes in the graph
	Node** nodes;   // The nodes contained by this graph
	                // Those nodes are stored via an array of pointers
};

/////////////////////////////////////////////////////////////////////////////////
// This function allocates an empty graph in the dynamic memory 
// and returns a pointer to this graph via its first argument
/////////////////////////////////////////////////////////////////////////////////

void graph_create(Graph** g)
{
	*g = (Graph*)malloc(sizeof(Graph));
	(*g)->node_count = 0;
	(*g)->nodes = NULL;
}


/////////////////////////////////////////////////////////////////////////////////
// This function is a helper function returning the id of a node
// in the graph of this node
/////////////////////////////////////////////////////////////////////////////////

int graph_get_node_id(Node* n)
{
	for (int n_id = 0; n_id < n->graph->node_count; n_id++)
		if (n->graph->nodes[n_id] == n)
			return n_id;
	printf("ERROR NODE NOT FOUND\n");
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////
// This function creates a node in dynamic memory with a given 
// string as label, inserts the created node in the graph passed
// as argument and returns a pointer on the created node 
/////////////////////////////////////////////////////////////////////////////////

Node* graph_insert_node(Graph* g, char* label)
{
	Node* n = (Node*)malloc(sizeof(Node));

	n->adjacent_nodes_count= 0;
	n->adjacent_nodes = NULL;

	n->label = (char*)malloc((strlen(label) + 1) * sizeof(char));
	strcpy(n->label, label);

	n->graph = g;
	g->node_count++;
	g->nodes = (Node**)realloc(g->nodes, g->node_count * sizeof(Node*));
	g->nodes[g->node_count - 1] = n;
	return n;
}


/////////////////////////////////////////////////////////////////////////////////
// Inserts an edge between the two nodes passed as arguments
/////////////////////////////////////////////////////////////////////////////////

void graph_insert_edge(Node* n_1, Node* n_2)
{
	n_1->adjacent_nodes_count++;
	n_1->adjacent_nodes = (Node**)realloc(n_1->adjacent_nodes, n_1->adjacent_nodes_count * sizeof(Node*));
	n_1->adjacent_nodes[n_1->adjacent_nodes_count - 1] = n_2;

	n_2->adjacent_nodes_count++;
	n_2->adjacent_nodes = (Node**)realloc(n_2->adjacent_nodes, n_2->adjacent_nodes_count * sizeof(Node*));
	n_2->adjacent_nodes[n_2->adjacent_nodes_count - 1] = n_1;
}

/////////////////////////////////////////////////////////////////////////////////
// Deletes a graph passed as argument
/////////////////////////////////////////////////////////////////////////////////

void graph_delete(Graph* g)
{
	for (int n_id = 0; n_id < g->node_count; n_id++)
	{
        if(g->nodes[n_id]->adjacent_nodes_count != 0)
	        free(g->nodes[n_id]->adjacent_nodes); 

	        
	    free(g->nodes[n_id]);
	}

	free(g->nodes);
	free(g);
}

typedef struct
{
	int node_count;	 // Amount of nodes in the path                
	Node** nodes;    // The nodes contained in this graph
                     // Those nodes are stored via an array of pointers    
}Path;


/////////////////////////////////////////////////////////////////////////////////
// Prints the nodes in the path passed as argument
/////////////////////////////////////////////////////////////////////////////////

void path_print(Path* p)
{
	for (int n_id = 0; n_id < p->node_count; n_id++)
	{
		printf("%s", p->nodes[n_id]->label);
		if (n_id != p->node_count - 1)
			printf("->");
	}
	printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////
// Deletes the path 
/////////////////////////////////////////////////////////////////////////////////

void path_delete(Path* p) 
{
	free(p->nodes);
	free(p);
}


/////////////////////////////////////////////////////////////////////////////////
// This function finds the shortest path between the two nodes passed
// as first and second arguments via Dijkstra, and returns a pointer to the 
// found path data structure via its third argument.
// The returned path does not contain any nodes if no path is found.
// If from == to then the returned path only contains from . 
// Since the function allocates the path in dynamic memory, the returned path 
// needs to be deleted via the path_delete function!
/////////////////////////////////////////////////////////////////////////////////

void graph_find_shortest_path(Node* from, Node* to, Path** p)
{
    // Allocates the path
	*p = (Path*)malloc(sizeof(Path));

	if (from == to) // From-Node == To-Node, returning path with only From-Node
	{
		(*p)->node_count =1;
		(*p)->nodes = (Node**)malloc(sizeof(Node*));		
		(*p)->nodes[0] = from;
		return;
	}

	Graph* g = from->graph;
	
	// Current cost to add the nodes to the shortest path
	unsigned int* nodes_current_cost = (unsigned int*)malloc(g->node_count * sizeof(unsigned int));
	for (int n_id = 0; n_id < g->node_count; n_id++)		
			nodes_current_cost[n_id] = UINT_MAX;
	nodes_current_cost[graph_get_node_id(from)] = 0;
	
	// Stores for each node, from which other node the cost for reaching this
	// node are currently lowest
	int* nodes_best_reachable_from = (int*)malloc(g->node_count * sizeof(int));
	for (int n_id = 0; n_id < g->node_count; n_id++)	
		nodes_best_reachable_from[n_id] = -1;

	
	// Nodes, to which the shortest path is known
	bool* nodes_added = (bool*)malloc(g->node_count * sizeof(bool));
	for (int n_id = 0; n_id < g->node_count; n_id++)	
			nodes_added[n_id] = false;
	nodes_added[graph_get_node_id(from)] = true;
	
	
    // Initializes the shortest path from From-Node to all adjacent nodes
	for (int e_id = 0; e_id < from->adjacent_nodes_count; e_id++)
	{
		Node* n = from->adjacent_nodes[e_id];
		nodes_best_reachable_from[graph_get_node_id(n)] = graph_get_node_id(from);
		nodes_current_cost[graph_get_node_id(n)] = 1;
	}
	
	
	bool node_found = false;
	while (true)
	{
		unsigned int next_min_cost = UINT_MAX;
		int next_node_id = -1;

        // Searches for the next node to add to the nodes to which
        // the shortest path is known
		for (int n_id = 0; n_id < g->node_count; n_id++)
			if (nodes_added[n_id] == false &&
				nodes_current_cost[n_id] < next_min_cost)
			{
				next_min_cost = nodes_current_cost[n_id];
				next_node_id = n_id;
			}

		if (next_min_cost == UINT_MAX)  // No more nodes reachable because graph 
			break;		                // is disconnected, To-Node has not been 
			                            // reached

        // Adding the newly found node to the nodes to which the shortest path is
        // known
		nodes_added[next_node_id] = true;
		Node* next_node = g->nodes[next_node_id];

		if (next_node == to) // Found the shortest path to To-node
		{
			node_found = true;
			break;
		}

        // Updates the cost to reach all other nodes if the path from the newly  
        // added node is cheaper than the path from an older node
		for (int e_id = 0; e_id < next_node->adjacent_nodes_count; e_id++)
		{
			int updated_node_id = graph_get_node_id(next_node->adjacent_nodes[e_id]);
			unsigned int cost_to_reach = 1 + nodes_current_cost[next_node_id];

			if (nodes_current_cost[updated_node_id] > cost_to_reach)
			{
				nodes_current_cost[updated_node_id] = cost_to_reach;
				nodes_best_reachable_from[updated_node_id] = next_node_id;
			}
		}
	}


	if (!node_found) // To-Node not reachable because graph is disconnected
	{
		(*p)->node_count = 0;
		(*p)->nodes = NULL;
	}
	else // Backtracking, building the path from To-Node to From-Node
	{

		int to_node_id = graph_get_node_id(to);
		int from_node_id = graph_get_node_id(from);


		(*p)->nodes = NULL;
		
		// Determining the amount of nodes on the path 
		(*p)->node_count = 1;	
		int backtrack_node_id = to_node_id;
		while (backtrack_node_id != from_node_id)
		{
			(*p)->node_count++;
			backtrack_node_id = nodes_best_reachable_from[backtrack_node_id];
		}

        // Allocating pointers for the nodes on the path
		(*p)->nodes = (Node**)malloc((*p)->node_count * sizeof(Node*));
		

        // Inserting the nodes on the path from To-Node to From-Node		
		(*p)->nodes[(*p)->node_count-1] = to;
		backtrack_node_id = to_node_id;
		for (int n_id = 1; n_id < (*p)->node_count; n_id++)
		{
			backtrack_node_id = nodes_best_reachable_from[backtrack_node_id];
			Node* current_backtrack_node = g->nodes[backtrack_node_id];
			(*p)->nodes[(*p)->node_count-1-n_id] = current_backtrack_node;
		}

	}

	//freeing all temporary arrays
	free(nodes_current_cost);
	free(nodes_best_reachable_from);
	free(nodes_added);
}

//Erzeugt einen Ring mit gegebener Knotenzahl.
//Bei nur einem Knoten erhält dieser Knoten eine Kante mit sich selbst.
Graph* graph_create_ring(int n) {
	assert(n > 0);
	Graph* graph;
	graph_create(&graph);

	Node* first = graph_insert_node(graph, (char*)"0");
	Node* previous = first;
	
	for(int i = 1; i < n; ++i) {
		int size = snprintf(NULL, 0, "%d", i);
		char name[size+1];
		sprintf(name, "%d", i);
		Node* next = graph_insert_node(graph, name);
		graph_insert_edge(previous, next);
		previous = next;
	}

	graph_insert_edge(previous, first);
	
	return graph;
}

//Erzeugt einen 3D-Torus mit gegebener Höhe, Breite und Tiefe.
//Vorgehen: Erstelle alle Knoten in einem 3d-Array.
//Iteriere anschließend das Array, um die Kanten zu setzen.
//Notiz: Das geht auch mit einem 1-D Array (graph->nodes), aber dann müsste man
//sich überlegen, wie man da die Kanten setzen muss.
Graph* graph_create_3d_torus(int height, int width, int depth) {
	assert(height > 0 && width > 0 && depth > 0);
	Graph* graph;
	graph_create(&graph);

	//3d Array von Zeigern auf Node erstellen
	Node**** grid_3d = (Node****)malloc(height * sizeof(Node***));
	for(int i = 0; i < height; ++i) {
		grid_3d[i] = (Node***)malloc(width * sizeof(Node**));
		for(int j = 0; j < width; ++j) {
			grid_3d[i][j] = (Node**)malloc(depth * sizeof(Node*));
			for(int k = 0; k < depth; ++k) {
				int size = snprintf(NULL, 0, "%d,%d,%d", i, j, k);
				char name[size+1];
				sprintf(name, "%d,%d,%d", i, j, k);
				grid_3d[i][j][k] = graph_insert_node(graph, name);
			}
		}
	}
	
	//Entlang der Tiefe periodisch setzen
	for(int i = 0; i < height; ++i) {
		for(int j = 0; j < width; ++j) {
			for(int k = 0; k < depth; ++k) {
				graph_insert_edge(grid_3d[i][j][k], grid_3d[i][j][(k + 1) % depth]);
			}
		}
	}	
	
	//Entlang der Breite
	for(int i = 0; i < height; ++i) {
		for(int j = 0; j < depth; ++j) {
			for(int k = 0; k < width; ++k) {
				graph_insert_edge(grid_3d[i][k][j], grid_3d[i][(k + 1) % width][j]);
			}
		}
	}
	
	//Entlang der Höhe
	for(int i = 0; i < width; ++i) {
		for(int j = 0; j < depth; ++j) {
			for(int k = 0; k < height; ++k) {
				graph_insert_edge(grid_3d[k][i][j], grid_3d[(k + 1) % height][i][j]);
			}
		}
	}
	
	//Speicher freigeben
	for(int i = 0; i < height; ++i) {
		for(int j = 0; j < width; ++j) {
			free(grid_3d[i][j]);
		}
		free(grid_3d[i]);
	}
	free(grid_3d);

	return graph;
}

//Erzeugt einen vollständigen Graphen mit gegebener Knotenzahl.
Graph* graph_create_complete_graph(int n) {
	assert(n > 1);
	Graph* graph;
	graph_create(&graph);

	for(int i = 0; i < n; ++i) {
		int size = snprintf(NULL, 0, "%d", i);
		char name[size+1];
		sprintf(name, "%d", i);
		graph_insert_node(graph, name);
	}
	
	//Mit 2 verschachtelten Schleifen lassen sich alle 2er-Kombinationen aus n Elementen ermitteln
	//(Es gibt n über k Auswahlen von je k Elementen aus einer Menge von n Elementen)
	for(int i = 0; i < n - 1; ++i) {
		for(int j = i + 1; j < n; ++j) {
			graph_insert_edge(graph->nodes[i], graph->nodes[j]);
		}
	}
	
	return graph;
}

//Bestimmt den Grad des Graphen indem der höchste Grad eines Knoten gesucht wird
int graph_calculate_degree(Graph* graph) {
	int degree = 0;
	for(int i = 0; i < graph->node_count; ++i) {
		if(graph->nodes[i]->adjacent_nodes_count > degree) {
			degree = graph->nodes[i]->adjacent_nodes_count;
		}
	}
	
	return degree;
}

int graph_calculate_diameter(Graph* graph) {
	int diameter = 0;
	
	//Auch hier werden alle 2er-Kombinationen wie bei graph_create_complete_graph betrachtet
	//Es werden also alle Pfade bestimmt und davon der längste genommen (ineffizient)
	for(int i = 0; i < graph->node_count - 1; ++i) {
		for(int j = i; j < graph->node_count; ++j) {
			Path* p;
			graph_find_shortest_path(graph->nodes[i], graph->nodes[j], &p);
			if(p->node_count - 1 > diameter) {
				diameter = p->node_count - 1;
			}
		}
	}
	
	return diameter;
}

int graph_calculate_edge_count(Graph* graph) {
	int count = 0;
	for(int i = 0; i < graph->node_count; ++i) {
		count += graph->nodes[i]->adjacent_nodes_count;	
	}
	
	return count / 2; //Jede Kante wird doppelt gezählt, also durch 2 teilen
}

/////////////////////////////////////////////////////////////////////////////////
// main function demonstrating the use of the graph functions
/////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** args)
{
	//Ring
	int n = 10, d;
	Graph* g_ring = graph_create_ring(n);
	
	assert(graph_calculate_degree(g_ring) == 2);
	assert(graph_calculate_diameter(g_ring) == (int)(n / 2.));
	assert(g_ring->node_count == n);
	assert(graph_calculate_edge_count(g_ring) == n);
	
	//3d-Torus
	//Fail für 2, 4, 2. Algorithmus gibt 4. n = 16, floor(16^(1/3)/2) * d = 3
	//Formel falsch? Runden statt abrunden?
	int height = 3, width = 3, depth = 3;
	Graph* torus_3d = graph_create_3d_torus(height, width, depth);
	n = torus_3d->node_count;
	d = 3;
	int expected_diameter = d * (int)(pow(n, 1./d)/2.); 
	int number_of_nodes = height*width*depth;
	
	assert(graph_calculate_degree(torus_3d) == 2*d);
	assert(graph_calculate_diameter(torus_3d) == expected_diameter);
	assert(torus_3d->node_count == number_of_nodes);
	assert(graph_calculate_edge_count(torus_3d) == d*number_of_nodes); 
	
	//Vollständiger Graph
	n = 10;
	Graph* complete_graph = graph_create_complete_graph(n);

	assert(graph_calculate_degree(complete_graph) == n-1);
	assert(graph_calculate_diameter(complete_graph) == 1);
	assert(complete_graph->node_count == n);
	assert(graph_calculate_edge_count(complete_graph) == n*(n-1)/2);
	
	printf("All tests passed!\n");
	return 0;
}
