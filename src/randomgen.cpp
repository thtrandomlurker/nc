#include <random>

static std::mt19937 mt(time(0));
std::uniform_int_distribution<int32_t> angleDist(-180000, 180000);
std::uniform_int_distribution<int32_t> distanceDist(100000, 900000);
std::uniform_int_distribution<int32_t> amplitudeDist(100, 1000);
std::uniform_int_distribution<int32_t> frequencyDist(-8, 8);

int32_t get_random_angle() {
	return angleDist(mt);
}
int32_t get_random_distance() {
	return distanceDist(mt);
}
int32_t get_random_amplitude() {
	return amplitudeDist(mt);
}
int32_t get_random_frequency() {
	return frequencyDist(mt);
}