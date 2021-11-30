#pragma once
#include <algorithm>
#include <curses.h>

using WINDOW = struct _win_st;

// Entity is a render node
class Entity
{
    Position pos = {0};
    char icon = ' ';
    WINDOW *wnd = nullptr;

public:
    Entity() = default;

    Entity(Position pos, char icon, WINDOW *wnd) : pos(pos), icon(icon), wnd(wnd)
    {
        mvwaddch(wnd, pos.y, pos.x, icon);
    }

    ~Entity()
    {
        if (wnd == nullptr)
        {
            return;
        }

        mvwaddch(wnd, pos.y, pos.x, ' ');
    }

    Entity(const Entity &other) = delete;
    Entity(Entity &other) = delete;
    Entity(Entity &&other)
    {
        std::swap(pos, other.pos);
        std::swap(icon, other.icon);
        std::swap(wnd, other.wnd);
    }

    Entity &operator=(Entity &&other)
    {
        std::swap(pos, other.pos);
        std::swap(icon, other.icon);
        std::swap(wnd, other.wnd);
        return *this;
    }

    void SetIcon(char icon)
    {
        this->icon = icon;
        mvwaddch(wnd, pos.y, pos.x, icon);
    }

    const Position &GetPosition() const
    {
        return pos;
    }
};