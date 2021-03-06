#include"Board.h"
#include"simulater/MeglimathCore/GameLogic/Field.h"
#include"simulater/MeglimathCore/GameLogic/Transform.h"
#define BOOST_PYTHON_STATIC_LIB    
#include<boost\python\tuple.hpp>
#include<boost\range\irange.hpp>
#include<iostream>
using boost::irange;
void Board::init_board(int turn, int start_player, int width, int height) {
	gamelogic->InitializeRandom(turn, height, width);
	first = !start_player ? TeamType::A : TeamType::B;
	this->turn = first;
	this->_thinks.clear();
}
void Board::do_move(int move) {
	/*move1,move2は[0,12*12*2)*/
	int move1 = move % (12 * 12 * 2);
	int move2 = move / (12 * 12 * 2);
	Action act1{ move1 / (12 * 12) };
	Action act2{ move2 / (12 * 12) };
	int to1 = move1 % (12 * 12);
	int to2 = move2 % (12 * 12);
	auto agents = gamelogic->GetTeams()[turn].agents;
	_Point<int> p0 = agents[0].position;
	_Point<int> p1 = agents[1].position;
	_Point<int> moveto1 = { to1 / 12,to1 % 12 };
	_Point<int> moveto2 = { to2 / 12,to2 % 12 };
	Direction dir1 = Transform::DeltaToDir(moveto1 - p0);
	Direction dir2 = Transform::DeltaToDir(moveto2 - p1);
	
	Think think{ { {{act1,dir1},{act2,dir2} } } };
	_thinks[turn] = think;
	if (turn != first) {
		gamelogic->NextTurn(_thinks);
	}
	turn = turn == TeamType::A ? TeamType::B : TeamType::A;
}
py::tuple Board::has_a_winner()const {
	return py::make_tuple(gamelogic->GetWinner() != -1, gamelogic->GetWinner());
}
py::tuple Board::game_end()const {
	return py::make_tuple(gamelogic->GetGameEnd(), gamelogic->GetWinner());
}
int Board::get_current_player()const {
	return turn == TeamType::A ? 0 : 1;
}
np::ndarray Board::get_current_state()const {
	auto& cells = gamelogic->GetField().cells;
	py::tuple shape = py::make_tuple(cells.width(), cells.height());
	np::ndarray ret = np::zeros(shape, np::dtype::get_builtin<long long>());
	for (int x = 0; x < cells.width(); x++) {
		for (int y = 0; y < cells.height(); y++) {
			int r;
			switch (cells[y][x].tile)
			{
			case TileType::A:
				r = 0;
				break;
			case TileType::B:
				r = 1;
				break;
			case TileType::None:
				r = -1;
				break;
			}
			ret[x][y] = r;
		}
	}
	return ret;
}
np::ndarray Board::get_board_state()const {
	const auto& cells = gamelogic->GetField().cells;
	py::tuple shape = py::make_tuple(cells.width(), cells.height());
	np::ndarray ret = np::zeros(shape, np::dtype::get_builtin<long long>());
	for (int x = 0; x < cells.width(); x++) {
		for (int y = 0; y < cells.height(); y++) {
			ret[x][y] = cells[y][x].point;
		}
	}
	return ret;
}
np::ndarray Board::get_player_state()const {
	const auto& cells = gamelogic->GetField().cells;
	py::tuple shape = py::make_tuple(cells.width(), cells.height());
	np::ndarray ret = np::zeros(shape, np::dtype::get_builtin<long long>());
	auto ag = gamelogic->GetAgents();
	for (int i = 0; i < ag.size(); i++) {
		auto p = ag[i].position;
		ret[p.x][p.y] = i + 1;
	}
	return ret;
}
int Board::get_remain_turn() const {
	return this->gamelogic->GetTurn();
}
int Board::get_point(int player) const {
	auto pl = !player ? TeamType::A : TeamType::B;
	return this->gamelogic->GetField().GetTotalPoints()[static_cast<int>(pl)];
}
py::list Board::get_availables()const {
	py::list ret;
	for (int move = 0; move < 12 * 12 * 2 * 12 * 12 * 2; move++) {
		int move1 = move % (12 * 12 * 2);
		int move2 = move / (12 * 12 * 2);
		Action act1{ move1 / (12 * 12) };
		Action act2{ move2 / (12 * 12) };
		int to1 = move1 % (12 * 12);
		int to2 = move2 % (12 * 12);
		auto agents = gamelogic->GetTeams()[turn].agents;
		_Point<int> p0 = agents[0].position;
		_Point<int> p1 = agents[1].position;
		_Point<int> moveto1 = { to1 / 12,to1 % 12 };
		_Point<int> moveto2 = { to2 / 12,to2 % 12 };
		Direction dir1 = Transform::DeltaToDir(moveto1 - p0);
		Direction dir2 = Transform::DeltaToDir(moveto2 - p1);
		if (dir1 != Direction::Stop && dir2 != Direction::Stop) {
			Think think{ { {{act1,dir1},{act2,dir2} } } };
			if (gamelogic->IsThinkAble(turn, think))ret.append(move);
		}
	}
	return ret;
}
Board::Board() :gamelogic(new GameLogic()) {
	gamelogic->InitializeRandom(60, 6, 6);
}

Board::Board(const Board &b) : gamelogic(std::make_unique<GameLogic>(*b.gamelogic)), _thinks(b._thinks), turn(b.turn), first(b.first) {}

void Board::make_board(py::tuple size, np::ndarray points,
	py::tuple agent1, py::tuple agent2, py::tuple agent3, py::tuple agent4,
	np::ndarray tile, int rturn) {
	using std::array;
	using std::vector;
	using size_itr = boost::python::stl_input_iterator<size_t>;
	size_itr begin(size), end; vector<size_t> fieldsize(begin, end);

	using point_itr = boost::python::stl_input_iterator<int>;
	vector<vector<int>> poss{};
	for (auto p : { agent1,agent2,agent3,agent4 }) {
		point_itr begin(p), end;
		//各tupleの内容をvectorに詰める
		poss.emplace_back(vector<int>(begin, end));
	}

	array<TeamLogic, 2> teamlogic{
		TeamLogic{ array<Agent,2>{Agent{{ poss[0][0],poss[0][1] }} , Agent{{ poss[1][0],poss[1][1] }} } },
		TeamLogic{ array<Agent,2>{Agent{{ poss[2][0],poss[2][1] }} , Agent{{ poss[3][0],poss[3][1] }} } }
	};

	_Size field_size{ fieldsize[0],fieldsize[1] };
	_Size data_size = _Size((field_size.x + 1) / 2, (field_size.y + 1) / 2);
	_Grid<Cell> gcell{ field_size };
	// ポイントをグリッド状に成型して入力
	int idx = 0;
	auto data = reinterpret_cast<int *>(points.get_data());
	for (int i : irange(0, (int)data_size.y))
	{
		for (int k : irange(0, (int)data_size.x))
		{
			gcell[i][k].point = data[idx];
			// データをコピー
			gcell[field_size.y - 1 - i][field_size.x - 1 - k] = gcell[i][k];
			gcell[field_size.y - 1 - i][k] = gcell[i][k];
			gcell[i][field_size.x - 1 - k] = gcell[i][k];

			idx++;
		}
	}
	// タイルをグリッド状に成型して入力
	idx = 0;
	auto tile_data = reinterpret_cast<int *>(tile.get_data());
	for (int i : irange(0, (int)field_size.y))
	{
		for (int k : irange(0, (int)field_size.x))
		{
			switch (tile_data[idx]) {
			case 'a':
				gcell[i][k].PaintedBy(TeamType::A);
				break;
			case 'b':
				gcell[i][k].PaintedBy(TeamType::B);
				break;
			case '-':
				break;
			}
			idx++;
		}
	}
	Field field{ gcell };
	gamelogic->InitializeVariable(rturn, field, teamlogic);
	first = TeamType::A;
	this->turn = first;
	this->_thinks.clear();
}
