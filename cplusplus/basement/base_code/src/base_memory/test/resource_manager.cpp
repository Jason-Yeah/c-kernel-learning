#include "resource_manager.h"

resource_manager::resource_manager(): _dintp(nullptr) {}
resource_manager::resource_manager(int val)
{
    _dintp = std::make_unique<int>(val);
}

resource_manager::resource_manager(resource_manager&& other) noexcept
: _dintp(std::move(other._dintp)) {}
resource_manager& resource_manager::operator= (resource_manager&& other) noexcept
{
    if (this != &other) _dintp = std::move(other._dintp);
    return *this;
}

int resource_manager::get_value() const
{
    if (!_dintp) throw std::runtime_error("Resource is not initialized!");
    return *_dintp;
}

void resource_manager::set_value(int new_val)
{
    // Plan A
    // if (!_dintp) throw std::runtime_error("Resource is not initialized!");
    // // _dintp = std::make_unique<int>(new_val);
    // *_dintp = new_val;
    // Plan B
    if (_dintp) *_dintp = new_val;
    else _dintp = std::make_unique<int>(new_val);
}




