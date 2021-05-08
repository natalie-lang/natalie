#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <setjmp.h>
#include <vector>

/*
 * TODO:
 * - add destructors on every object
 */

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/cell.hpp"
#include "natalie/gc/heap.hpp"
#include "natalie/gc/heap_block.hpp"
#include "natalie/gc/marking_visitor.hpp"
