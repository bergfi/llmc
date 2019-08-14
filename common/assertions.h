#pragma once

#include <cassert>
#include <iostream>

#ifndef LLMC_DEBUG_ASSERT
#   define LLMC_DEBUG_ASSERT assert
#endif

#ifndef LLMC_DEBUG_LOG
#   define LLMC_DEBUG_LOG() (std::cerr)
#endif