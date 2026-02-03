#include <chrono>
#include <deque>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

namespace {
constexpr int kWidth = 40;
constexpr int kHeight = 20;
constexpr char kWallChar = '#';
constexpr char kSnakeChar = 'O';
constexpr char kFoodChar = '*';

struct Point {
  int x;
  int y;

  bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

enum class Direction { kUp, kDown, kLeft, kRight };

#ifndef _WIN32
class TerminalRawMode {
 public:
  TerminalRawMode() {
    tcgetattr(STDIN_FILENO, &original_);
    termios raw = original_;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }

  ~TerminalRawMode() { tcsetattr(STDIN_FILENO, TCSANOW, &original_); }

 private:
  termios original_{};
};

bool KeyPressed() {
  timeval timeout{0, 0};
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(STDIN_FILENO, &read_fds);
  return select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout) > 0;
}

char ReadChar() {
  char ch = 0;
  if (read(STDIN_FILENO, &ch, 1) < 0) {
    return 0;
  }
  return ch;
}
#endif

void ClearScreen() {
#ifdef _WIN32
  std::system("cls");
#else
  std::system("clear");
#endif
}

Point RandomEmptyCell(const std::deque<Point>& snake, std::mt19937& rng) {
  std::uniform_int_distribution<int> x_dist(1, kWidth - 2);
  std::uniform_int_distribution<int> y_dist(1, kHeight - 2);

  while (true) {
    Point candidate{x_dist(rng), y_dist(rng)};
    bool on_snake = false;
    for (const auto& segment : snake) {
      if (segment == candidate) {
        on_snake = true;
        break;
      }
    }
    if (!on_snake) {
      return candidate;
    }
  }
}

void Render(const std::deque<Point>& snake, const Point& food, int score) {
  std::vector<std::string> board(kHeight, std::string(kWidth, ' '));

  for (int x = 0; x < kWidth; ++x) {
    board[0][x] = kWallChar;
    board[kHeight - 1][x] = kWallChar;
  }
  for (int y = 0; y < kHeight; ++y) {
    board[y][0] = kWallChar;
    board[y][kWidth - 1] = kWallChar;
  }

  for (const auto& segment : snake) {
    board[segment.y][segment.x] = kSnakeChar;
  }
  board[food.y][food.x] = kFoodChar;

  ClearScreen();
  for (const auto& row : board) {
    std::cout << row << '\n';
  }
  std::cout << "Score: " << score << "\n";
  std::cout << "Use W/A/S/D to move, Q to quit." << std::endl;
}

bool IsOpposite(Direction dir, Direction next) {
  return (dir == Direction::kUp && next == Direction::kDown) ||
         (dir == Direction::kDown && next == Direction::kUp) ||
         (dir == Direction::kLeft && next == Direction::kRight) ||
         (dir == Direction::kRight && next == Direction::kLeft);
}
void WaitForExitKey() {
#ifdef _WIN32
  while (true) {
    char key = static_cast<char>(_getch());
    if (key == 'k' || key == 'K') {
      break;
    }
  }
#else
  while (true) {
    char key = ReadChar();
    if (key == 'k' || key == 'K') {
      break;
    }
  }
#endif
}
}  // namespace

int main() {
#ifndef _WIN32
  TerminalRawMode raw_mode;
#endif

  std::deque<Point> snake{{kWidth / 2, kHeight / 2}, {kWidth / 2 - 1, kHeight / 2}};
  Direction direction = Direction::kRight;

  std::random_device rd;
  std::mt19937 rng(rd());
  Point food = RandomEmptyCell(snake, rng);

  bool running = true;
  int score = 0;
  int beans = 0;
  auto start_time = std::chrono::steady_clock::now();

  while (running) {
#ifdef _WIN32
    if (_kbhit()) {
      char input = static_cast<char>(_getch());
#else
    if (KeyPressed()) {
      char input = ReadChar();
#endif
      Direction next_direction = direction;
      switch (input) {
        case 'w':
        case 'W':
          next_direction = Direction::kUp;
          break;
        case 's':
        case 'S':
          next_direction = Direction::kDown;
          break;
        case 'a':
        case 'A':
          next_direction = Direction::kLeft;
          break;
        case 'd':
        case 'D':
          next_direction = Direction::kRight;
          break;
        case 'q':
        case 'Q':
          running = false;
          break;
        default:
          break;
      }

      if (!IsOpposite(direction, next_direction)) {
        direction = next_direction;
      }
    }

    if (!running) {
      break;
    }

    Point head = snake.front();
    switch (direction) {
      case Direction::kUp:
        head.y -= 1;
        break;
      case Direction::kDown:
        head.y += 1;
        break;
      case Direction::kLeft:
        head.x -= 1;
        break;
      case Direction::kRight:
        head.x += 1;
        break;
    }

    bool hit_wall = head.x <= 0 || head.x >= kWidth - 1 || head.y <= 0 || head.y >= kHeight - 1;
    bool hit_self = false;
    for (const auto& segment : snake) {
      if (segment == head) {
        hit_self = true;
        break;
      }
    }

    if (hit_wall || hit_self) {
      auto end_time = std::chrono::steady_clock::now();
      auto elapsed_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
      ClearScreen();
      std::cout << "Game Over!\n";
      std::cout << "Beans collected: " << beans << "\n";
      std::cout << "Survival time: " << elapsed_seconds << " seconds\n";
      std::cout << "Press K to exit.\n";
      WaitForExitKey();
      break;
    }

    snake.push_front(head);

    if (head == food) {
      score += 10;
      beans += 1;
      food = RandomEmptyCell(snake, rng);
    } else {
      snake.pop_back();
    }

    Render(snake, food, score);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
  }

  return 0;
}
