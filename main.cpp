#include <list>
#include <random>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstring>

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

#define CSI "\e["

void setcursor(int fd, int x, int y)
{
    auto str = CSI + to_string(y) + ';' + to_string(x) + "f";
    write(fd, str.c_str(), str.size());
}

void showCursor(bool show)
{
    if (show)
    {
        puts(CSI "?25h");
    }
    else
    {
        fputs(CSI "?25l");
    }
}
#undef CSI

int getinput(int fd)
{
    int byte = 0;
    recv(fd, &byte, 1, 0);
    return byte;
}

int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(0, &fds));
}

// Entity is a render node
class Entity
{
    Position pos = {0};
    char icon = ' ';
    int fd;

public:
    Entity(Position pos, char icon, int fd) : pos(pos), icon(icon), fd(fd)
    {
        setcursor(fd, pos.x, pos.y);
        write(fd, &icon, 1);
        Log << "Entity(" << pos << ", '" << icon << "', wnd)" << endl;
    }

    Entity(const Entity &other) = delete;
    Entity(Entity &other) = delete;
    Entity(Entity &&other) = default;

    Entity &operator=(Entity &&other)
    {
        swap(pos, other.pos);
        swap(icon, other.icon);
        fd = other.fd;
    }

    ~Entity()
    {
        char empty = ' ';
        setcursor(fd, pos.x, pos.y);
        write(fd, &empty, 1);
        Log << "~Entity(" << pos << ", '" << icon << "', wnd)" << endl;
    }

    void SetIcon(char icon)
    {
        this->icon = icon;
        setcursor(fd, pos.x, pos.y);
        write(fd, &icon, 1);
    }

    const Position &GetPosition() const
    {
        return pos;
    }
};

auto header = " SNEK v%s for vasilev.io SCORE: %d ";
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

#define COLS 80
#define LINES 25

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
    write(client_fd, CSI "2J", 4);
    write(client_fd, "\377\375\042\377\373\001", 6);
    srand(time(nullptr));
    fcntl(STDIN_FILENO, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    for (int i = 0; i < 56; i++)
    {
        getchar();
    }

    Direction dir = Direction::Down;
    Entity food = Entity(randpos(1, COLS - 1, 1, LINES - 1), '@', client_fd);

    list<Entity> snek;
    snek.emplace_back(Position{40, 12}, 'O', client_fd);
    snek.emplace_back(Position{39, 12}, 'o', client_fd);
    snek.emplace_back(Position{38, 12}, 'o', client_fd);

    int snek_length = 3;

    bool lost = false;

    auto lose = [&] {
        lost = true;
        snek.front().SetIcon('X');

        // auto wnd2 = newwin(4, 16, LINES / 2 - 1, COLS / 2 - 8);
        // wborder(wnd2, 0, 0, 0, 0, 0, 0, 0, 0);
        // mvwaddstr(wnd2, 1, 1, "   YOU DIED");
        // mvwaddstr(wnd2, 2, 1, "CTRL+C TO EXIT");
        // wrefresh(wnd2);
        // delwin(wnd2);
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
            auto newfood = Entity(randpos(1, COLS - 1, 1, LINES - 1), '@', client_fd);
            food = move(newfood);
        }
        else
        {
            head.SetIcon('o');
        }

        snek.emplace_front(newpos, 'O', client_fd);
        while (snek.size() > snek_length)
        {
            snek.pop_back();
        }
    };

    for (;;)
    {
        auto input = getchar();
        switch (input)
        {
        case '\033':
            if (getchar() != '[')
            {
                break;
            }
            switch (getchar())
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

        default:
            break;
        }

        move_snek();
        setcursor(client_fd, 2, 0);

        char buf[1024];
        sprintf(buf, header, version, snek_length - 3);
        write(client_fd, buf, strlen(buf));

        // do
        {
            usleep(50'000);
        }
        //  while (lost);
        cerr << "'" << char(input) << "'" << endl;
    }
}