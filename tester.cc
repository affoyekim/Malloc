#include <stdlib.h>

int main() {
  void *slots[100];

  for(int i = 0; i < 100; i++) {
    slots[i] = 0;
  }
  
  for(int i = 0; i < 10000; i++) {
    int rando = rand() % 100;
	if(slots[rando] == 0) {
	  int newrando = rand() % 1050;
	  if(newrando > 1000) {
	    newrando = rand() % 5000 + 1200;
	  }
	  void * ptr = malloc(newrando);
	  slots[rando] = ptr;
	}
	else {
	  free(slots[rando]);
	  slots[rando] = 0;
	}
  }
  
  return 0;
}
