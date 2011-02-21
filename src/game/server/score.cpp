/* copyright (c) 2008 rajh and gregwar. Score stuff */

#include "score.hpp"
#include <engine/e_config.h>
#include <sstream>
#include <fstream>
#include <list>
#include <string>
#include <string.h>
#include "gamecontext.hpp"

static LOCK score_lock = 0;

PLAYER_SCORE::PLAYER_SCORE(const char *name, int points, const char *ip)
{
	str_copy(this->name, name, sizeof(this->name));
	this->points = points;
	str_copy(this->ip, ip, sizeof(this->ip));
}

std::list<PLAYER_SCORE> top;

SCORE::SCORE()
{
	if(score_lock == 0)
		score_lock = lock_create();
	load();
}

std::string save_file()
{
	std::ostringstream oss;
	oss << config.sv_map << "_record.dtb";
	return oss.str();
}

static void save_score_thread(void *)
{
	lock_wait(score_lock);
	std::fstream f;
	f.open(save_file().c_str(), std::ios::out);
	if(!f.fail())
	{
		int t = 0;
		for(std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end(); i++)
		{
			f << i->name << std::endl << i->points << std::endl  << i->ip << std::endl;
			t++;
			if(t%50 == 0)
				thread_sleep(1);
		}
	}
	f.close();
	lock_release(score_lock);
}

void SCORE::save()
{
	void *save_thread = thread_create(save_score_thread, 0);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)save_thread);
#endif
}

void SCORE::load()
{
	lock_wait(score_lock);
	std::fstream f;
	f.open(save_file().c_str(), std::ios::in);
	top.clear();
	while (!f.eof() && !f.fail())
	{
		std::string tmpname, tmpscore, tmpip, tmpcpline;
		std::getline(f, tmpname);
		if(!f.eof() && tmpname != "")
		{
			std::getline(f, tmpscore);
			std::getline(f, tmpip);
			top.push_back(*new PLAYER_SCORE(tmpname.c_str(), atof(tmpscore.c_str()), tmpip.c_str()));
		}
	}
	f.close();
	lock_release(score_lock);
}

PLAYER_SCORE *SCORE::search_score(int id, bool score_ip, int *position)
{
	char ip[16];
	server_getip(id, ip);
	
	int pos = 1;
	for(std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end(); i++)
	{
		if(!strcmp(i->ip, ip) && config.sv_score_ip && score_ip)
		{
			if(position)
				*position = pos;
			return & (*i);
		}
		pos++;
	}
	
	return search_name(server_clientname(id), position, 0);
}

PLAYER_SCORE *SCORE::search_name(const char *name, int *position, bool nocase)
{
	PLAYER_SCORE *player = 0;
	int pos = 1;
	int found = 0;
	for (std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end(); i++)
	{
		if(str_find_nocase(i->name, name))
		{
			if(position)
				*position = pos;
			if(nocase)
			{
				found++;
				player = & (*i);
			}
			if(!strcmp(i->name, name))
				return & (*i);
		}
		pos++;
	}
	if(found > 1)
	{
		if(position)
			*position = -1;
		return 0;
	}
	return player;
}

void SCORE::parsePlayer(int id, int point)
{
	const char *name = server_clientname(id);
	char ip[16];
	server_getip(id, ip);
	
	lock_wait(score_lock);
	PLAYER_SCORE *player = search_score(id, 1, 0);
	if(player)
	{
		if(player->points < point)
		{
			player->points = point;
			str_copy(player->name, name, sizeof(player->name));
		}
	}
	else
		top.push_back(*new PLAYER_SCORE(name, point, ip));
	
	top.sort();
	top.reverse();
	lock_release(score_lock);
	save();
}

void SCORE::initPlayer(int id)
{
	char ip[16];
	server_getip(id, ip);
	PLAYER_SCORE *player = search_score(id, 0, 0);
	if(player)
	{
		lock_wait(score_lock);
		str_copy(player->ip, ip, sizeof(player->ip));
		lock_release(score_lock);
		save();
	}
}

void SCORE::top5_draw(int id, int debut)
{
	int pos = 1;
	char buf[512];
	game.send_chat_target(id, "----------- Top 5 -----------");
	for (std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end() && pos <= 5+debut; i++)
	{
		if(pos >= debut)
		{
			str_format(buf, sizeof(buf), "%d. %s Points: %d", pos, i->name, i->points);
			game.send_chat_target(id, buf);
		}
		pos++;
	}
	game.send_chat_target(id, "-----------------------------");
}
