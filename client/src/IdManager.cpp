#include "IdManager.hpp"

namespace hippo
{
std::atomic<uint32_t> IdGen::resman_cnt;
std::atomic<uint32_t> IdGen::sess_cnt;
}

