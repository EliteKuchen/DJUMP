/* copyright (c) 2008 rajh and gregwar. Score stuff */

#ifndef SCORE_H_RACE
#define SCORE_H_RACE
#include <engine/e_server_interface.h>

class PLAYER_SCORE
{
public:
	char name[MAX_NAME_LENGTH];
	int points;
	char ip[16];
	float cp_time[42];
	
	PLAYER_SCORE(const char *name, int points, const char *ip);
	
	bool operator==(const PLAYER_SCORE& other) { return (this->points == other.points); }
	bool operator<(const PLAYER_SCORE& other) { return (this->points < other.points); }
};

class SCORE
{
public:
	SCORE();
	
	void save();
	void load();
	PLAYER_SCORE *search_score(int id, bool score_ip, int *position);
	PLAYER_SCORE *search_name(const char *name, int *position, bool match_case);
	void parsePlayer(int id, int point);
	void initPlayer(int id);
	void top5_draw(int id, int debut);
};

#endif

