#include <list>
#include <random>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <curses.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

using namespace std;

auto Log = fstream("output.log", ios_base::out);

enum class Direction
{
    Up,
    Down,
    Left,
    Right,
};

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
            throw invalid_argument("invalid direction");
        }
    }
};

ostream &operator<<(ostream &os, Position p)
{
    return os << "{ " << p.x << ", " << p.y << " }";
}

// Entity is a render node
class Entity
{
    Position pos = {0};
    char icon = ' ';
    WINDOW *wnd;

public:
    Entity(Position pos, char icon, WINDOW *wnd) : pos(pos), icon(icon), wnd(wnd)
    {
        mvwaddch(wnd, pos.y, pos.x, icon);
        Log << "Entity(" << pos << ", '" << icon << "', wnd)" << endl;
    }

    Entity(const Entity &other) = delete;
    Entity(Entity &other) = delete;
    Entity(Entity &&other) = default;

    Entity &operator=(Entity &&other)
    {
        swap(pos, other.pos);
        swap(icon, other.icon);
        wnd = other.wnd;
    }

    ~Entity()
    {
        mvwaddch(wnd, pos.y, pos.x, ' ');
        Log << "~Entity(" << pos << ", '" << icon << "', wnd)" << endl;
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

auto header = " SNEK v%s for vasilev.io SCORE: %d PRESS Q TO EXIT";
auto version = "0.1a";

auto randpos(int x0, int xmax, int y0, int ymax)
{
    return Position{
        rand() % (xmax - x0) + x0,
        rand() % (ymax - y0) + y0,
    };
}

struct sockaddr_in address;
socklen_t addrlen = sizeof(address);
int create_and_bind()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

int main(int argc, char *argv[])
{
    int server_fd = create_and_bind();
    int client_fd = 0;
    if (client_fd = accept(server_fd, (sockaddr *)&address, &addrlen); client_fd < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    auto out = fdopen(client_fd, "wb");
    string socket = "/dev/fd/" + to_string(client_fd);
    dup2(client_fd, STDOUT_FILENO);
    dup2(client_fd, STDIN_FILENO);

    write(client_fd, "\377\375\042\377\373\001", 6);
    for (int i = 0; i < 56; i++)
    {
        getchar();
    }

    initscr();
    auto wnd = newwin(LINES, COLS, 0, 0);
    wborder(wnd, 0, 0, 0, 0, 0, 0, 0, 0);
    nodelay(stdscr, true);
    cbreak();
    noecho();
    curs_set(0);
    srand(time(nullptr));

    Direction dir = Direction::Down;
    Entity food = Entity(randpos(1, COLS - 1, 1, LINES - 1), '@', wnd);

    list<Entity> snek;
    snek.emplace_back(Position{40, 12}, 'O', wnd);
    snek.emplace_back(Position{39, 12}, 'o', wnd);
    snek.emplace_back(Position{38, 12}, 'o', wnd);

    int snek_length = 3;

    bool lost = false;

    auto lose = [&] {
        lost = true;
        snek.front().SetIcon('X');

        auto wnd2 = newwin(4, 18, LINES / 2 - 1, COLS / 2 - 9);
        wborder(wnd2, 0, 0, 0, 0, 0, 0, 0, 0);
        mvwaddstr(wnd2, 1, 1, "   YOU DIED");
        mvwaddstr(wnd2, 2, 1, "DISCONNECTING...");
        wrefresh(wnd2);
        delwin(wnd2);
    };

    auto move_snek = [&] {
        auto &head = snek.front();
        auto newpos = head.GetPosition().Translated(dir);
        if (newpos.x == 0 || newpos.y == 0 || newpos.x >= COLS - 1 || newpos.y >= LINES - 1)
        {
            lose();
            return;
        }

        for (auto &&s : snek)
        {
            if (&s != &head && newpos == s.GetPosition())
            {
                lose();
                return;
            }
        }

        if (newpos == food.GetPosition())
        {
            snek_length++;
            auto newfood = Entity(randpos(1, COLS - 1, 1, LINES - 1), '@', wnd);
            food = move(newfood);
        }
        else
        {
            head.SetIcon('o');
        }

        snek.emplace_front(newpos, 'O', wnd);
        while (snek.size() > snek_length)
        {
            snek.pop_back();
        }
    };

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

        move_snek();
        mvwprintw(wnd, 0, 2, header, version, snek_length - 3);
        wrefresh(wnd);

        usleep(50'000);
        if (lost)
        {
            usleep(3'000'000);
            exit(EXIT_SUCCESS);
        }
    }
}