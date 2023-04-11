#pragma once

namespace trc {

struct program {
    virtual constexpr ~program() = default;

    virtual constexpr auto run() -> int = 0;
};

}
