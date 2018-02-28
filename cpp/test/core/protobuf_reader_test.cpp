#include "precompiled.h"

#include <bond/protocol/random_protocol.h>

#include <boost/test/debug.hpp>


bool init_unit_test()
{
    boost::debug::detect_memory_leaks(false);
    bond::RandomProtocolReader::Seed(time(nullptr), time(nullptr) / 1000);
    return true;
}
