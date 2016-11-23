# pragma once

#include <string>

extern size_t get_content(std::string binary, size_t offset, void* buf, size_t size);
extern uint64_t get_entry(std::string binary);