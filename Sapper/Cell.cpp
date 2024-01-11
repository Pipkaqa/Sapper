#include "Cell.h"

Cell::Cell(int x, int y)
{
	x_pos = x;
	y_pos = y;
}

int Cell::check_mines_around(std::vector<std::vector<Cell*>> map) const
{
	int mines_around = 0;

	int x_start = x_pos - 1;
	int x_end = x_pos + 2;

	int y_start = y_pos - 1;
	int y_end = y_pos + 2;

	for (int y = y_start; y < y_end; y++)
	{
		for (int x = x_start; x < x_end; x++)
		{
			if (map[x][y]->type == MINE)
			{
				mines_around++;
			}
		}
	}

	return mines_around;
}