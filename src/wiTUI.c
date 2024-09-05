#include "../include/wiTUI.h"
#include "../../WiEnrich/include/enrich.h"
#include <stdlib.h>

wiWindow* wiMakeWindow() {
	wiWindow* window = (wiWindow*) malloc(sizeof(wiWindow));

	window->width = 10;
	window->height = 10;
	window->title = "Test window";
	window->footer = "q: quit";

	// Make 1x1 array initially
	window->contents = (char***) malloc(sizeof(char**));
	window->contents[0] = (char**) malloc(sizeof(char*));
	window->contents[0][0] = NULL;

	wiBorder border = { 
		"\u256D", "\u256E", "\u256F", "\u2570", 
		"\u2500", "\u2500", "\u2502", "\u2502",
		wiEnrich("[WHITE]"), wiEnrich("[DIM][WHITE]")
	};
	window->border = border;

	return window;
}
