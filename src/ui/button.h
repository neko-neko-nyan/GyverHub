#pragma once

class HubBuilder;

namespace gyverhub {
    class Button {
        char value = '0';

    public:
        constexpr Button() {}
        constexpr Button(const Button &) = default;

        operator bool() {
            return this->value == '1';
        }

        bool clicked() {
            return this->value == '2';
        }

        friend class ::HubBuilder;
    };
}
