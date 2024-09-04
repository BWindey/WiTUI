#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../include/wiTablesIO.h"

void setDefaultShortcuts(wiShortcuts* shortcuts) {
	assert(shortcuts != NULL);

	shortcuts->quit = 'q';
	shortcuts->up = 'k';
	shortcuts->down = 'j';
	shortcuts->left = 'h';
	shortcuts->right = 'l';
}

wiTable* wiCreateTable(char* title) {
	wiTable* table = (wiTable*) malloc(sizeof(wiTable));
	table->title = strdup(title);
	table->rows = NULL;
	table->shortcuts = (wiShortcuts*) malloc(sizeof(wiShortcuts));
	
	setDefaultShortcuts(table->shortcuts);

	return table;
}

void wiFreeTable(wiTable* table) {
	free(table->title);
	free(table->shortcuts);
	// TODO: free the actual rows too when they are implemented
	free(table);
}
