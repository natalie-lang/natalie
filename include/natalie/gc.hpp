#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/cell.hpp"
#include "natalie/gc/heap.hpp"
#include "natalie/gc/heap_block.hpp"
#include "natalie/gc/marking_visitor.hpp"
