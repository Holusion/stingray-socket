#include "EventListener.hpp"
#include <iostream>
#include <thread>
#include <cstring>
#include  <errno.h>
#include  <unistd.h>
#include  <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>


#include <string>

class  Controller: public EventListener {
  public:
    Controller(std::string);
    ~Controller();
    void update();
    void serve();
    void handle(int);
    std::string get_request(int);
    void parse_request(std::string str);
    virtual int getAxis() const{return axis;};
    virtual int getQuit() const{return quit;};
  private:
    std::string sock_addr;
    std::thread t;
    int fd = -1;
    int angle = 0;
    int axis = 4;
    int quit = 0;
    int buflen_;
    char* buf_;

};
Controller::Controller(std::string str="/tmp/stingray.sock"):sock_addr(str) {
  struct sockaddr_un addr;
  //Create buffer
  buflen_ = 1024;
  buf_ = new char[buflen_+1];
  int err;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_addr.c_str(), sizeof(addr.sun_path)-1);
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0){
    std::cout<<"Failed to open Socket : "<<strerror(-fd)<<std::endl;
    return;
  }
  err = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (err < 0){
    std::cout<<"Error binding /tmp/stingray.sock :"<<strerror(-err)<<std::endl;
  }
  err =listen(fd,5);
  if (err == -1){
    std::cout<<"Error listening to /tmp/stingray.sock :"<<strerror(errno)<<std::endl;
  }
  t = std::thread(&Controller::serve,this);
}

Controller::~Controller() {
  if(fd >0){
    close(fd);
    unlink("/tmp/stingray.sock");
  }
  delete buf_;
}

void Controller::serve(){
  // setup client
  int client;
  struct sockaddr_un client_addr;
  socklen_t clientlen = sizeof(client_addr);
    // accept clients
  while ((client = accept(fd,(struct sockaddr *)&client_addr,&clientlen)) > 0) {
      handle(client);
  }
}

void Controller::handle(int client){
  // loop to handle all requests
  while (1) {
      // get a request
      std::string request = get_request(client);
      // break if client is done or an error occurred
      if (request.empty())
          break;
      parse_request(request);
  }
  close(client);
}
void Controller::parse_request(std::string str){
  if (str == "QUIT"){
    quit = 1;
    return;
  }
  try{
    if (str[0] == 'd'){
      axis = 0;
      angle += std::stoi(str.substr(1));
    }else{
      axis = std::stoi(str);
    }
  }catch (const std::invalid_argument& ia) {
	  std::cerr << "Invalid argument: " << ia.what() << '\n';
  }

}
std::string Controller::get_request(int client) {
    std::string request = "";
    // read until we get a newline
    while (request.find("\n") == std::string::npos) {
        int nread = recv(client,buf_,1024,0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } else if (nread == 0) {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        request.append(buf_,nread);
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    return request;
}

void Controller::update(){
  if (angle == 0){
    //We work in angle mode only when axis = 0.
    return;
  }

  axis = (angle<0)?++angle:--angle;
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
