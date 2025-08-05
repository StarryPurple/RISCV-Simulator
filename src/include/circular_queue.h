#ifndef ISM_CIRCULAR_QUEUE_H
#define ISM_CIRCULAR_QUEUE_H

#include <stdexcept>
namespace insomnia {

template <class T, std::size_t Len>
class circular_queue {
  static_assert(std::is_default_constructible_v<T> && Len > 0);
public:
  circular_queue() = default;
  void clear() { _rear = _size = 0; }
  void push(const T &t) {
    if(full()) throw std::runtime_error("push in full circular queue.");
    _size++;
    std::size_t f = _rear + _size - 1;
    if(f >= Len) f -= Len;
    _data[f] = t;
  }
  template <class ...Args>
  void emplace(Args &&...args) {
    if(full()) throw std::runtime_error("emplace in full circular queue.");
    _size++;
    std::size_t f = _rear + _size - 1;
    if(f >= Len) f -= Len;
    new (&_data[f]) T(std::forward<Args>(args)...);
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
    std::size_t f = _rear + _size - 1;
    if(f >= Len) f -= Len;
    return _data[f];
  }
  const T& back() const {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    std::size_t f = _rear + _size - 1;
    if(f >= Len) f -= Len;
    return _data[f];
  }
  [[nodiscard]] std::size_t size() const { return _size; }
  [[nodiscard]] bool empty() const { return _size == 0; }
  [[nodiscard]] bool full() const { return _size == Len; }
  [[nodiscard]] std::size_t front_index() const {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    return _rear;
  }
  [[nodiscard]] std::size_t back_index() const {
    if(empty()) throw std::runtime_error("read in empty circular queue.");
    std::size_t f = _rear + _size - 1;
    return f >= Len ? f - Len : f;
  }
  // what index will it be if a new entry is pushed.
  [[nodiscard]] std::size_t next_index() const {
    if(full()) throw std::runtime_error("query next index in full circular queue.");
    std::size_t res = _rear + _size;
    return res > Len ? res - Len : res;
  }

  // You'd better know what you're doing.
  const T& at(std::size_t index) const {
    std::size_t f = _rear + _size - 1;
    if(f > Len) {
      if(f - Len < index && index < _rear)
        throw std::runtime_error("read in invalid place.");
    } else {
      if(index < _rear || index > f)
        throw std::runtime_error("read in invalid place.");
    }
    return _data[index];
  }

  // You'd better know what you're doing.
  T& at(std::size_t index) {
    std::size_t f = _rear + _size - 1;
    if(f > Len) {
      if(f - Len < index && index < _rear)
        throw std::runtime_error("read in invalid place.");
    } else {
      if(index < _rear || index > f)
        throw std::runtime_error("read in invalid place.");
    }
    return _data[index];
  }

  bool index_valid(std::size_t index) const {
    std::size_t f = _rear + _size - 1;
    if(f > Len) {
      if(f - Len < index && index < _rear) return false;
    } else {
      if(index < _rear || index > f) return false;
    }
    return true;
  }

  // be careful using this.
  void pop_back() {
    if(empty())
      throw std::runtime_error("popping empty queue.");
    --_size;
  }

private:
  T _data[Len]{};
  std::size_t _rear = 0, _size = 0;
};

}

#endif // ISM_CIRCULAR_QUEUE_H