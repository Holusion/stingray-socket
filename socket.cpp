#include "EventListener.hpp"
#include <iostream>
#include <cstring>
#include <cmath>
#include  <errno.h>
#include  <unistd.h>
#include  <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/un.h>
#include <csignal>
#include <vector>
#include <algorithm> // remove and remove_if

class  Controller: public EventListener {
  public:
    Controller(int);
    ~Controller();
    void update();
    static void serve(int signum);
    static bool handle(int);
    static bool configure(int f);
    static std::string get_request(int);
    static void parse_request(std::string str);
    virtual int getAxis() const{return axis;};
    virtual int getQuit() const{return quit;};
  private:
    std::string sock_addr;
    static std::vector<int>clients;
    static int fd;
    static int angle;
    static int axis;
    static int quit;
    static const size_t buflen_ = 1024;
    static char buf_[buflen_];
    static Controller *instance;
};

int Controller::fd = -1;
int Controller::angle = 0;
int Controller::axis = 4;
int Controller::quit = 0;
std::vector<int>Controller::clients = std::vector<int>();

Controller::Controller(int port=3004){
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(addr.sin_zero), 8);
  int err, flags, one=1;
  if (fd ==-1){
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0){
      std::cout<<"Failed to open Socket : "<<strerror(-fd)<<std::endl;
      return;
    }
    // set socket options

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0){
      std::cout<<"Error binding "<<port<<": "<<strerror(-err)<<std::endl;
    }
    //set socket flags
    configure(fd);

    err =listen(fd,5);
    if (err == -1){
      std::cout<<"Error listening to  "<<port<<": "<<strerror(errno)<<std::endl;
    }
  }
  std::cout<<"Listening on port "<<port<<std::endl;
}

Controller::~Controller() {
  std::cout<<"Destroy"<<std::endl;
  quit = 1;
  if(0 < fd){
    close(fd);
    fd = -1;
  }
  for(auto client: clients){
    close(client);
  }
  //t.join();
}

bool Controller::configure(int f){
  int flags;
  flags = fcntl (f, F_GETFL);
  if(fcntl (f, F_SETFL, flags|O_ASYNC|O_NONBLOCK)<0){
    std::cout<<"F_SETFL failed"<<std::endl;
    return false;
  }
  if(fcntl(f, F_SETSIG, SIGIO)<0){
    std::cout<<"F_SETSIG failed"<<std::endl;
    return false;
  }
  struct sigaction handler;
  handler.sa_handler = Controller::serve;
  if (sigfillset(&handler.sa_mask)< 0){
    std::cout<<"failed to set fillset"<<std::endl;
    return false;
  }
  if(sigaction(SIGIO,&handler,0) < 0){
    std::cout<<"sigaction() failed"<<std::endl;
    return false;
  }
  if(fcntl(f, F_SETOWN, getpid())<0){
    std::cout<<"F_SETOWN failed"<<std::endl;
    return false;
  }
  return true;
}

void Controller::serve(int signum){
  // setup client
  int client, flags;
  struct sockaddr client_addr;
  socklen_t clientlen = sizeof(client_addr);
  if ((client = accept(fd, &client_addr,&clientlen)) < 0) {
    if (errno != EWOULDBLOCK && errno != EAGAIN){
      std::cout<<"Accept error : "<<strerror(errno)<<std::endl;
    }
  }else{
    std::cout << "Got a client!!"<<std::endl;
    //Configure the new client
    if(configure(client)){
      clients.push_back(client); //Append to clients
    }
  }
  //Then we handle accepted clients.
  //If handle() returns true, client is closed and should be deleted
  clients.erase(
    std::remove_if(clients.begin(), clients.end(), handle),
     clients.end()
  );
}

//return true if socket is closed
bool Controller::handle(int client){
  int rc;
  char buf [1024];
  rc= read(client, buf, 1024);
  if(rc == 0){
    close(client);
    return true;
  }else if (rc == -1){
    if (errno == EWOULDBLOCK || errno == EAGAIN){
      return false;
    }else{
      std::cout<<"read error on "<<client<<" : "<<strerror(errno)<<std::endl;
      return true;
    }
  }else{
    parse_request(std::string(buf));
  }
  return false;
}

void Controller::parse_request(std::string str){
  int na;
  str = str.substr(0, str.find("\n"));
  if (str == "QUIT"){
    quit = 1;
    return;
  }
  try{
    if (str[0] == 'd'){
      axis = 0;
      na = std::stoi(str.substr(1));
      if (std::signbit(na) == std::signbit(angle)){
        angle += na;
      }else{
        angle = na;
      }
    }else{
      axis = std::stoi(str);
    }
  }catch (const std::invalid_argument& ia) {
	  std::cerr << "Invalid argument: " << ia.what()<<": "<<str << '\n';
  }

}

void Controller::update(){
  if (angle == 0){
    //We work in angle mode only when axis = 0.
    return;
  }
  if( 4 < abs(angle)){
    axis = sqrt(abs(angle)*4)*abs(angle)/angle;
  }else{
    axis = 4*angle/abs(angle);
  }
  (angle<0)?angle++:angle--;

}



extern "C"{
  // the class factories
  extern EventListener* create() {
   return new Controller;
  }
  extern void destroy(EventListener* p) {
   delete p;
  }
  extern void update(EventListener* p){
    ((Controller*)p)->update();
  }
}
