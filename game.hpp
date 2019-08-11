#pragma once
#include <list>
#include <curses.h>
#include "entity.hpp"

class Game
{
public:
    bool lost = false;
    int snek_length = 3;
    Direction dir = Direction::Down;
    Entity food = Entity(Position{-1, -1}, ' ', nullptr);
    std::list<Entity> snek;
    WINDOW *wnd = nullptr;
    const char *header = " SNEK v%s for vasilev.io SCORE: %d PRESS Q TO EXIT";
    const char *version = "0.1a";

    Game()
    {
        // initialize & set up ncurses
        initscr();
        wnd = newwin(LINES, COLS, 0, 0);
        wborder(wnd, 0, 0, 0, 0, 0, 0, 0, 0);
        nodelay(stdscr, true);
        cbreak();
        noecho();
        curs_set(0);

        // seed rand()
        srand(time(nullptr));

        // create starting food
        food = Entity(Position::Random(1, COLS - 1, 1, LINES - 1), '@', wnd);

        // create starting snek body
        snek.emplace_back(Position{40, 12}, 'O', wnd);
        snek.emplace_back(Position{39, 12}, 'o', wnd);
        snek.emplace_back(Position{38, 12}, 'o', wnd);
    }

    void Run()
    {
        for (;;)
        {
            auto input = getch();
            switch (input)
            {
            case '\033':
                if (getch() != '[')
                {
                    break;
                }
                switch (getch())
                {
                case 'A':
                    dir = Direction::Up;
                    break;
                case 'B':
                    dir = Direction::Down;
                    break;
                case 'D':
                    dir = Direction::Left;
                    break;
                case 'C':
                    dir = Direction::Right;
                    break;
                }
                break;

            case 'q':
                exit(EXIT_SUCCESS);

            case ERR:
            default:
                break;
            }

            Tick();

            wrefresh(wnd);

            usleep(100'000);
            if (lost)
            {
                break;
            }
        }
        wclear(wnd);
    }

    void Lose()
    {
        lost = true;
        snek.front().SetIcon('X');

        // create a window with game over text
        auto wnd2 = newwin(4, 18, LINES / 2 - 1, COLS / 2 - 9);
        wborder(wnd2, 0, 0, 0, 0, 0, 0, 0, 0);
        mvwaddstr(wnd2, 1, 1, "   GAME OVER");

        mvwaddstr(wnd2, 2, 1, "DISCONNECTING.");
        wrefresh(wnd2);
        usleep(1'000'000);
        mvwaddstr(wnd2, 2, 1, "DISCONNECTING..");
        wrefresh(wnd2);
        usleep(1'000'000);
        mvwaddstr(wnd2, 2, 1, "DISCONNECTING...");
        wrefresh(wnd2);
        usleep(1'000'000);

        delwin(wnd2);
    };

    void Tick()
    {
        auto &head = snek.front();
        auto newpos = head.GetPosition().Translated(dir);

        // check that newpos is in bounds
        if (newpos.x == 0 || newpos.y == 0 || newpos.x >= COLS - 1 || newpos.y >= LINES - 1)
        {
            Lose();
            return;
        }

        // check that newpos doesn't intersect snek
        for (auto &&s : snek)
        {
            if (&s != &head && newpos == s.GetPosition())
            {
                Lose();
                return;
            }
        }

        // check for food at newpos
        if (newpos == food.GetPosition())
        {
            snek_length++;
            auto newfood = Entity(Position::Random(1, COLS - 1, 1, LINES - 1), '@', wnd);
            food = std::move(newfood);
        }
        else
        {
            head.SetIcon('o');
        }

        // grow snek forwards
        snek.push_front(Entity(newpos, 'O', wnd));

        // shrink snek from backwards
        while (snek.size() > snek_length)
        {
            snek.pop_back();
        }

        // print score info
        mvwprintw(wnd, 0, 2, header, version, snek_length - 3);
    };
};
