#pragma once


namespace gyverhub {
    class Builder;

    class Button {
        uint8_t value = 0;

    public:
        constexpr Button() {}
        constexpr Button(const Button &) = default;

        operator bool() {
            return this->value == 1;
        }

        bool clicked() {
            return this->value == 2;
        }

        friend class Builder;
    };
}
