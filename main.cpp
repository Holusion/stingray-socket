#include "socket.cpp"
#include <unistd.h>

int main(int argc, char**argv){
  EventListener *c = create();
  while(!c->getQuit()){
    c->update();
    usleep(40000);
  }
  destroy(c);
}
