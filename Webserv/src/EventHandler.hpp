#ifndef EVENTHANDLER_HPP
#define EVENTHANDLER_HPP

#include <sys/event.h>

#include <iostream>
#include <map>
#include <vector>

#define MAX_EVENTS 64
#define TIMER_TIME_OUT_SEC 600
#define TIMER_KEEP_ALIVE_SEC 5

class EventHandler
{

private:
  int _kq;
  int _nevents;
  std::vector<struct kevent> _changeList;
  struct kevent _eventList[MAX_EVENTS];

public:
  EventHandler(void);
  virtual ~EventHandler(void);

  void eventHandlerInit(void);

public:
  void addKeventToChangeList(uintptr_t ident, int16_t filter, uint16_t flags,
                             uint32_t fflags, intptr_t data, void *udata);
  int newEvents(void);

public:
  void registerReadEvent(int serverSocket);
  void unregisterReadEvent(int serverSocket);

  void registerReadWriteEvents(int clientSocket);
  void unregisterReadWriteEvents(int clientSocket);
  
  void switchToWriteState(int clientSocket);
  void switchToReadState(int clientSocket);

  void turnOnRead(int clientSocket);
  void turnOffRead(int clientSocket);
  void turnOnWrite(int clientSocket);
  void turnOffWrite(int clientSocket);

  void registerTimerEvent(int clientSocket, intptr_t sec);
  void unregisterTimerEvent(int clientSocket);

public:
  const struct kevent &getEvent(int index) const;
};

#endif
