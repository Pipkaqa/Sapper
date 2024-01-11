#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <future>
#include <thread>
#include <chrono>
#include <string>
#include <queue>
#include <stack>
#include <vector>
#include <random>

#include "InputKeys.h"
#include "Cell.h"
#include "MoveDirection.h"

std::promise<void> update_exit_signal;
std::promise<void> timer_exit_signal;

std::thread timer_thread;
std::thread update_thread;

const int OFFSET_Y = 1;
const int MILLISECONDS_PAUSE = 500;
const int MAP_SIZE = 20;

const int dRow[] = { -1, 0, 1, 0 };
const int dCol[] = { 0, 1, 0, -1 };

int total_mines_count;

int mines_remain;
int flags_remain;

COORD mouse_pos;

std::string time_str = "00:00";

std::vector<std::vector<Cell*>> map(MAP_SIZE);

void choose_difficulty();
void generate_map();

void draw_tick(std::future<void> future);
void timer_tick(std::future<void> future);

bool get_input();

void try_move_cursor(MoveDirection move_dir);
void try_change_cell_flagged_status(int x, int y);
bool try_open_cell(int x, int y);
void try_secure_cells_around(std::pair<int, int> start_pos);

void check_cells_flagged_status();
void finish_game(std::string message);

void gotoxy(int x, int y);

int main()
{
	choose_difficulty();
	generate_map();

	update_thread = std::thread(&draw_tick, std::move(update_exit_signal.get_future()));

	while (true)
	{
		if (get_input())
		{
			break;
		}
	}

	update_thread.join();
	timer_thread.join();

	return 0;
}

void choose_difficulty()
{
	int difficulty;

	std::string input;

	while (true)
	{
	while_mark:
		system("cls");

		std::cout << "Choose difficulty: \n";
		std::cout << "1 - Easy\n";
		std::cout << "2 - Medium\n";
		std::cout << "3 - Hard\n";
		std::cout << "4 - Professional\n";

		gotoxy(19, 0);

		std::getline(std::cin, input);

		if (std::empty(input) || input.length() != 1 || !isdigit(input[0]))
		{
			continue;
		}

		break;
	}

	difficulty = std::stoi(input);

	switch (std::stoi(input))
	{
	case 1:
		total_mines_count = 10;
		break;
	case 2:
		total_mines_count = 30;
		break;
	case 3:
		total_mines_count = 60;
		break;
	case 4:
		total_mines_count = 99;
		break;
	default:
		goto while_mark;
	}

	mines_remain = total_mines_count;
	flags_remain = total_mines_count;

	gotoxy(0, 6);

	std::cout << "Game difficulty - " << difficulty;

	Sleep(MILLISECONDS_PAUSE);

}

void generate_map()
{
	gotoxy(2, 1 + OFFSET_Y);

	for (int y = 0; y < MAP_SIZE; y++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			map[y].push_back(new Cell(y, x));
		}
	}

	for (auto& cell : map[0])
	{
		cell->type = WALL;
		cell->visible = true;
	}

	for (auto& cell : map[MAP_SIZE - 1])
	{
		cell->type = WALL;
		cell->visible = true;
	}

	for (auto& str : map)
	{
		str[0]->type = WALL;
		str[0]->visible = true;
	}

	for (auto& str : map)
	{
		str[MAP_SIZE - 1]->type = WALL;
		str[MAP_SIZE - 1]->visible = true;
	}

	std::random_device random_device;
	std::mt19937 generator(random_device());
	std::uniform_int_distribution<> distribution(1, MAP_SIZE - 2);

	for (int i = 0; i < total_mines_count; i++)
	{
		while (true)
		{
			int mine_position_x = distribution(generator);
			int mine_position_y = distribution(generator);

			if (map[mine_position_x][mine_position_y]->type != MINE)
			{
				map[mine_position_x][mine_position_y]->type = MINE;
				break;
			}
		}
	}
}

void draw_tick(std::future<void> future)
{
	while (future.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
	{
		system("cls");

		std::string flags = std::to_string(flags_remain);
		flags = std::string(2 - min(2, (int)flags.length()), '0') + flags;

		std::string info_block_1 = flags_remain < 0 ? "[Mines: 0!]" : "[Mines: " + flags + "]";
		std::string info_block_2 = "[.-.]";
		std::string info_block_3 = "[Time: " + time_str + "]\n";

		std::cout << info_block_1;

		int space_count = MAP_SIZE * 2 - static_cast<int>(info_block_1.length()) - static_cast<int>(info_block_2.length()) - static_cast<int>(info_block_3.length());

		for (int i = 0; i < space_count / 2; i++)
		{
			std::cout << " ";
		}

		std::cout << info_block_2;

		for (int i = 0; i < space_count / 2; i++)
		{
			std::cout << " ";
		}

		std::cout << info_block_3;

		for (int y = 0; y < MAP_SIZE; y++)
		{
			for (int x = 0; x < MAP_SIZE; x++)
			{
				std::string sym;

				if (map[x][y]->visible)
				{
					if (map[x][y]->type == WALL)
					{
						sym = "#";
					}
					else
					{
						int mines_count_around = map[x][y]->check_mines_around(map);

						if (mines_count_around == 0) sym = "-";
						else sym = std::to_string(mines_count_around);
					}
				}
				else if (map[x][y]->flagged)
				{
					sym = "P";
				}
				else
				{
					sym = ".";
				}

				std::cout << sym << " ";
			}

			std::cout << std::endl;
		}

		gotoxy(mouse_pos.X, mouse_pos.Y);

		Sleep(1000);
	}
}

void timer_tick(std::future<void> future)
{
	int seconds = 0;
	int minutes = 0;

	while (future.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
	{
		if (++seconds > 59)
		{
			seconds = 0;

			if (++minutes > 59)
			{
				finish_game("Game over!");
			}
		}

		std::string seconds_str = std::to_string(seconds);
		std::string minutes_str = std::to_string(minutes);

		seconds_str = std::string(2 - min(2, (int)seconds_str.length()), '0') + seconds_str;
		minutes_str = std::string(2 - min(2, (int)minutes_str.length()), '0') + minutes_str;

		time_str = minutes_str + ":" + seconds_str;

		Sleep(1000);
	}
}

bool get_input()
{
	if (_kbhit())
	{
		switch (_getch())
		{
		case KEY_ESC:
			finish_game("Game over!");
			return true;
		case KEY_ENTER:
		case KEY_SPACE:
			return try_open_cell(mouse_pos.X / 2, mouse_pos.Y - 1);
			break;
		case KEY_F_ENG:
		case KEY_F_ENG_CAPS:
		case KEY_F_RUS:
		case KEY_F_RUS_CAPS:
			try_change_cell_flagged_status(mouse_pos.X / 2, mouse_pos.Y - 1);
			break;
		case KEY_W_ENG:
		case KEY_W_ENG_CAPS:
		case KEY_W_RUS:
		case KEY_W_RUS_CAPS:
		case KEY_UP:
			try_move_cursor(UP);
			break;
		case KEY_A_ENG:
		case KEY_A_ENG_CAPS:
		case KEY_A_RUS:
		case KEY_A_RUS_CAPS:
		case KEY_LEFT:
			try_move_cursor(LEFT);
			break;
		case KEY_S_ENG:
		case KEY_S_ENG_CAPS:
		case KEY_S_RUS:
		case KEY_S_RUS_CAPS:
		case KEY_DOWN:
			try_move_cursor(DOWN);
			break;
		case KEY_D_ENG:
		case KEY_D_ENG_CAPS:
		case KEY_D_RUS:
		case KEY_D_RUS_CAPS:
		case KEY_RIGHT:
			try_move_cursor(RIGHT);
			break;
		default:
			std::cout << _getch();
			break;
		}
	}

	return false;
}

void try_move_cursor(MoveDirection move_dir)
{
	COORD initial_pos = mouse_pos;

	switch (move_dir)
	{
	case UP:
		if (--initial_pos.Y > 0 + OFFSET_Y) gotoxy(initial_pos.X, initial_pos.Y);
		break;
	case RIGHT:
		if (++++initial_pos.X < (MAP_SIZE - 1) * 2) gotoxy(initial_pos.X, initial_pos.Y);
		break;
	case DOWN:
		if (++initial_pos.Y < MAP_SIZE - 1 + OFFSET_Y) gotoxy(initial_pos.X, initial_pos.Y);
		break;
	case LEFT:
		if (----initial_pos.X > 0) gotoxy(initial_pos.X, initial_pos.Y);
		break;
	}
}

void try_change_cell_flagged_status(int x, int y)
{
	if (!map[x][y]->visible)
	{
		map[x][y]->flagged = !map[x][y]->flagged;
	}

	check_cells_flagged_status();
}

bool try_open_cell(int x, int y)
{
	if (map[x][y]->flagged)
	{
		return false;
	}
	else if (map[x][y]->type == MINE)
	{
		map[x][y]->blowed = true;
		finish_game("Game over!");
		return true;
	}
	else if (time_str == "00:00")
	{
		try_secure_cells_around(std::make_pair(x, y));

		timer_thread = std::thread(&timer_tick, std::move(timer_exit_signal.get_future()));
	}
	else if (!map[x][y]->visible)
	{
		try_secure_cells_around(std::make_pair(x, y));
	}

	return false;
}

void try_secure_cells_around(std::pair<int, int> start_pos)
{
	if (!map[start_pos.first][start_pos.second]->visible)
	{
		if (map[start_pos.first][start_pos.second]->check_mines_around(map) != 0)
		{
			map[start_pos.first][start_pos.second]->visible = true;
			return;
		}
	}

	std::vector<std::vector<bool>> visited(MAP_SIZE);

	for (auto& str : visited)
	{
		str = std::vector<bool>(MAP_SIZE);
	}

	std::queue<std::pair<int, int>> queue;

	queue.push(std::make_pair(start_pos.first, start_pos.second));

	while (!queue.empty())
	{
		std::pair<int, int> cell = queue.front();

		queue.pop();
		visited[cell.first][cell.second] = true;

		for (int y = cell.second - 1; y < cell.second + 2; y++)
		{
			for (int x = cell.first - 1; x < cell.first + 2; x++)
			{
				if (map[x][y]->type != MINE)
				{
					map[x][y]->visible = true;
				}

				if (!visited[x][y] && map[x][y]->type != WALL && map[x][y]->check_mines_around(map) == 0)
				{
					queue.push(std::make_pair(x, y));

					if (map[x][y]->flagged)
					{
						map[x][y]->flagged = false;
						check_cells_flagged_status();
					}
				}
			}
		}
	}
}

void check_cells_flagged_status()
{
	int flags = total_mines_count;
	int flagged_mines = total_mines_count;

	for (const auto& str : map)
	{
		for (const auto& cell : str)
		{
			if (cell->flagged)
			{
				if (cell->type == MINE)
				{
					flagged_mines--;
				}

				flags--;
			}
		}
	}

	flags_remain = flags;
	mines_remain = flagged_mines;

	if (!flags_remain && !mines_remain)
	{
		finish_game("You win!");
	}
}

void finish_game(std::string message)
{
	system("cls");

	std::cout << message << "\n\n";

	std::string flags = std::to_string(flags_remain);
	flags = std::string(2 - min(2, (int)flags.length()), '0') + flags;

	std::string info_block_1 = flags_remain < 0 ? "[Mines: 0!]" : "[Mines: " + flags + "]";
	std::string info_block_2 = "[.-.]";
	std::string info_block_3 = "[Time: " + time_str + "]\n";

	std::cout << info_block_1;

	int space_count = MAP_SIZE * 2 - static_cast<int>(info_block_1.length()) - static_cast<int>(info_block_2.length()) - static_cast<int>(info_block_3.length());

	for (int i = 0; i < space_count / 2; i++)
	{
		std::cout << " ";
	}

	std::cout << info_block_2;

	for (int i = 0; i < space_count / 2; i++)
	{
		std::cout << " ";
	}

	std::cout << info_block_3;

	for (auto& str : map)
	{
		for (auto& cell : str)
		{
			cell->visible = true;
		}
	}

	for (int y = 0; y < MAP_SIZE; y++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			std::string sym;

			if (map[x][y]->visible)
			{
				if (map[x][y]->type == WALL)
				{
					sym = "#";
				}
				else if (map[x][y]->blowed)
				{
					sym = "X";
				}
				else if (map[x][y]->flagged)
				{
					sym = "P";
				}
				else if (map[x][y]->type == MINE)
				{
					sym = "*";
				}
				else
				{
					int mines_count_around = map[x][y]->check_mines_around(map);

					if (mines_count_around == 0) sym = "-";
					else sym = std::to_string(mines_count_around);
				}
			}
			else
			{
				sym = ".";
			}

			std::cout << sym << " ";
		}

		std::cout << std::endl;
	}

	update_exit_signal.set_value();
	timer_exit_signal.set_value();
}

void gotoxy(int x, int y)
{
	COORD coords = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coords);
	mouse_pos.X = coords.X;
	mouse_pos.Y = coords.Y;
}