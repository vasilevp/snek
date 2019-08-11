#pragma once
#include <stdexcept>
#include "direction.hpp"

// Position is a 2D vector with some basic vector operations
struct Position
{
    int x = 0;
    int y = 0;

    bool operator==(const Position &other) const
    {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Position &other) const
    {
        return !(*this == other);
    }

    Position operator+(const Position &other) const
    {
        return Position{x + other.x, y + other.y};
    }

    Position &operator+=(const Position &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Position Translated(Direction dir) const
    {
        switch (dir)
        {
        case Direction::Up:
            return *this + Position{0, -1};

        case Direction::Down:
            return *this + Position{0, 1};

        case Direction::Left:
            return *this + Position{-1, 0};

        case Direction::Right:
            return *this + Position{1, 0};

        default:
            throw std::invalid_argument("invalid direction");
        }
    }

    static Position Random(int x0, int xmax, int y0, int ymax)
    {
        return Position{
            rand() % (xmax - x0) + x0,
            rand() % (ymax - y0) + y0,
        };
    }
};
