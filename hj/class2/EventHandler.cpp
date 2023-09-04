
#include "EventHandler.hpp"
#include <unistd.h> // for close

EventHandler::EventHandler(void)
  : _kq(-1), _nevents(0), _changeList()
{
  // _changeList는 벡터라 기본 생성자에 의해 빈 상태로 초기화
  // 따라서, 불필요하지만 초기화 문법이 궁금해서 해보기.
  // _eventList는 사용 용도상 초기화가 불필요하다고 판단하지만... 인심 쓰기
  memset(_eventList, 0, sizeof(_eventList));
}

EventHandler::~EventHandler(void)
{
  if (_kq != -1)
  {
    close(_kq);
  }
}

void EventHandler::eventHandlerInit(void)
{
  if ((_kq = kqueue()) == -1)
  {
    throw "Fail to kqueue()" + std::string(strerror(errno));
  }
}

void EventHandler::addKeventToChangeList(uintptr_t ident, int16_t filter, uint16_t flags,
                                         uint32_t fflags, intptr_t data, void *udata)
{
  struct kevent e;
  EV_SET(&e, ident, filter, flags, fflags, data, udata);
  _changeList.push_back(e);
}

int EventHandler::newEvents()
{
  _nevents = kevent(_kq,
                    _changeList.data(), _changeList.size(),
                    _eventList, MAX_EVENTS,
                    NULL);
  if (_nevents == -1)
    throw "kevent error on newEvents" + std::string(strerror(errno));
  _changeList.clear();

  return (_nevents);
}

void EventHandler::registerReadEvent(int serverSocket)
{
  addKeventToChangeList(serverSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
}

void EventHandler::unregisterReadEvent(int serverSocket)
{
  addKeventToChangeList(serverSocket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
}

void EventHandler::registerReadWriteEvents(int clientSocket)
{
  addKeventToChangeList(clientSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
  addKeventToChangeList(clientSocket, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);
}

/**
 * unregist시 주의사항
 * 1. close후, fd 접근하면 안되므로, 하고 싶으면 close전에 호출해야
 * 2. close된 fd에 대해서 자동으로 kqueue가 항목에서 제거함으로 명시적 호출 불필요.
 */
void EventHandler::unregisterReadWriteEvents(int clientSocket)
{
  addKeventToChangeList(clientSocket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  addKeventToChangeList(clientSocket, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
}

void EventHandler::switchToWriteState(int clientSocket)
{
  addKeventToChangeList(clientSocket, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
  addKeventToChangeList(clientSocket, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
}

void EventHandler::switchToReadState(int clientSocket)
{
  addKeventToChangeList(clientSocket, EVFILT_READ, EV_ENABLE, 0, 0, NULL);
  addKeventToChangeList(clientSocket, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
}

const struct kevent &EventHandler::getEvent(int index) const
{
  return (_eventList[index]);
}
