#ifndef ISM_CIRCULAR_QUEUE_H
#define ISM_CIRCULAR_QUEUE_H

#include <stdexcept>
namespace insomnia {

template <class T, int Len>
class circular_queue {
  static_assert(std::is_default_constructible_v<T> && Len > 0);
public:
  circular_queue() = default;
  void clear() { _rear = _size = 0; }
  void push(const T &t) {
    if(full()) throw std::runtime_error("push in full circular queue.");
    _size++;
    back() = t;
  }
  void pop() {
    if(empty()) throw std::runtime_error("pop in empty circular queue.");
    _rear++; _size--;
    if(_rear == Len) _rear = 0;
  }
  T& front() {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    return _data[_rear];
  }
  const T& front() const {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    return _data[_rear];
  }
  T& back() {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    int f = _rear + _size - 1;
    if(f >= Len) f -= Len;
    return _data[f];
  }
  const T& back() const {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    int f = _rear + _size - 1;
    if(f >= Len) f -= Len;
    return _data[f];
  }
  [[nodiscard]] int size() const { return _size; }
  [[nodiscard]] bool empty() const { return _size == 0; }
  [[nodiscard]] bool full() const { return _size == Len; }

private:
  T _data[Len]{};
  int _rear = 0, _size = 0;
};

}

#endif // ISM_CIRCULAR_QUEUE_H