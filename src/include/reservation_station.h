#ifndef ISM_RESERVATION_STATION_H
#define ISM_RESERVATION_STATION_H

namespace insomnia {

template <std::size_t StnSize>
class ReservationStation : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};


}

#endif // ISM_RESERVATION_STATION_H