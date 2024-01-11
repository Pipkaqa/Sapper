#pragma once

#include <vector>
#include "CellType.h"

class Cell
{
public:
	Cell(int x, int y);

public:
	bool visible = false;
	bool flagged = false;
	bool blowed = false;

	CellType type = AIR;

public:
	int check_mines_around(std::vector<std::vector<Cell*>> map) const;

private:
	int x_pos;
	int y_pos;
};